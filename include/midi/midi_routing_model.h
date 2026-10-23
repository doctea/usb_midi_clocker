#pragma once

#include "midi/midi_mapper_matrix_manager.h"

// UI-free queries and edits over matrix connections, shared by the matrix page
// popup and per-owner routing panels. Holds no state of its own: every call reads
// the live matrix, so it is cheap to construct wherever it is needed.

struct MIDIRouteRow {
    source_id_t source = -1;
    target_id_t target = -1;
    MIDIConnectionPolicy policy;
    uint16_t held_notes = 0;
    uint32_t last_activity_ms = 0;
    bool has_activity = false;
};

struct MIDIRouteSummary {
    uint8_t total = 0;
    uint8_t paused = 0;
    uint16_t held_notes = 0;

    uint8_t live() const { return total - paused; }
    bool can_pause_all() const { return paused < total; }
    bool can_resume_all() const { return paused > 0; }
};

struct MIDIRouteCandidate {
    int8_t id = -1;         // target id (when adding from a source) or source id (when adding to a target)
    bool connected = false;
    bool paused = false;
};

enum class MIDIRouteConnectResult : uint8_t {
    CONNECTED,
    ALREADY_CONNECTED,
    INVALID,
    DISALLOWED,
    FULL,
};

class MIDIRoutingModel {
    public:
    explicit MIDIRoutingModel(MIDIMatrixManager &matrix) : matrix(matrix) {}

    //// listing; rows are ordered by the id of the far end. Return value is the number written.

    uint8_t list_from_source(source_id_t source, MIDIRouteRow *rows, uint8_t max) {
        uint8_t count = 0;
        matrix.for_each_connection_from(source, [&](const MIDIMatrixManager::connection_t &connection) {
            if (count < max) fill_row(connection, rows[count++]);
        });
        return count;
    }

    uint8_t list_to_target(target_id_t target, MIDIRouteRow *rows, uint8_t max) {
        uint8_t count = 0;
        matrix.for_each_connection_to(target, [&](const MIDIMatrixManager::connection_t &connection) {
            // store slots are unordered; insert sorted by source so the list is stable
            uint8_t position = count;
            while (position > 0 && rows[position - 1].source > connection.source) --position;
            if (position >= max) return;
            const uint8_t last = count < max ? count : max - 1;
            for (uint8_t index = last; index > position; --index) rows[index] = rows[index - 1];
            fill_row(connection, rows[position]);
            if (count < max) ++count;
        });
        return count;
    }

    // every route leaving any source that the owner has
    uint8_t list_outbound(const MIDIEndpointOwner *owner, MIDIRouteRow *rows, uint8_t max) {
        uint8_t count = 0;
        matrix.for_each_source_of(owner, [&](source_id_t source) {
            if (count < max) count += list_from_source(source, rows + count, max - count);
        });
        return count;
    }

    // every route arriving at any target that the owner has
    uint8_t list_inbound(const MIDIEndpointOwner *owner, MIDIRouteRow *rows, uint8_t max) {
        uint8_t count = 0;
        matrix.for_each_target_of(owner, [&](target_id_t target) {
            if (count < max) count += list_to_target(target, rows + count, max - count);
        });
        return count;
    }

    bool get_row(source_id_t source, target_id_t target, MIDIRouteRow &row) {
        const MIDIMatrixManager::connection_t *connection = matrix.find_connection(source, target);
        if (connection == nullptr) return false;
        fill_row(*connection, row);
        return true;
    }

    bool is_connected(source_id_t source, target_id_t target) {
        return matrix.is_connected(source, target);
    }

    //// summaries, for section headers and enabling bulk actions

    MIDIRouteSummary summarise_from_source(source_id_t source) {
        MIDIRouteSummary summary;
        matrix.for_each_connection_from(source, [&](const MIDIMatrixManager::connection_t &connection) {
            add_to_summary(connection, summary);
        });
        return summary;
    }

    MIDIRouteSummary summarise_to_target(target_id_t target) {
        MIDIRouteSummary summary;
        matrix.for_each_connection_to(target, [&](const MIDIMatrixManager::connection_t &connection) {
            add_to_summary(connection, summary);
        });
        return summary;
    }

    MIDIRouteSummary summarise_outbound(const MIDIEndpointOwner *owner) {
        MIDIRouteSummary summary;
        matrix.for_each_source_of(owner, [&](source_id_t source) {
            merge(summary, summarise_from_source(source));
        });
        return summary;
    }

    MIDIRouteSummary summarise_inbound(const MIDIEndpointOwner *owner) {
        MIDIRouteSummary summary;
        matrix.for_each_target_of(owner, [&](target_id_t target) {
            merge(summary, summarise_to_target(target));
        });
        return summary;
    }

    //// single-route edits

    MIDIRouteConnectResult connect(source_id_t source, target_id_t target) {
        if (!matrix.is_valid_connection_id_pair(source, target)) return MIDIRouteConnectResult::INVALID;
        if (matrix.is_connected(source, target)) return MIDIRouteConnectResult::ALREADY_CONNECTED;
        if (!matrix.is_allowed(source, target)) return MIDIRouteConnectResult::DISALLOWED;
        return matrix.connect(source, target) ? MIDIRouteConnectResult::CONNECTED : MIDIRouteConnectResult::FULL;
    }

    bool disconnect(source_id_t source, target_id_t target) {
        if (!matrix.is_connected(source, target)) return false;
        matrix.disconnect(source, target);
        return true;
    }

    bool toggle_pause(source_id_t source, target_id_t target) {
        const MIDIConnectionPolicy *policy = matrix.get_connection_policy(source, target);
        return policy != nullptr && matrix.set_paused(source, target, !policy->paused);
    }

    // channel steps through 0 (pass-through), 1..16, wrapping
    bool cycle_channel(source_id_t source, target_id_t target, int8_t direction = 1) {
        const MIDIConnectionPolicy *policy = matrix.get_connection_policy(source, target);
        if (policy == nullptr) return false;
        MIDIConnectionPolicy updated = *policy;
        updated.fixed_channel = (uint8_t)((updated.fixed_channel + (direction < 0 ? 16 : 1)) % 17);
        return matrix.set_connection_policy(source, target, updated);
    }

    bool cycle_quantise(source_id_t source, target_id_t target, int8_t direction = 1) {
        const MIDIConnectionPolicy *policy = matrix.get_connection_policy(source, target);
        if (policy == nullptr) return false;
        MIDIConnectionPolicy updated = *policy;
        updated.quantise_mode = next_quantise_mode(updated.quantise_mode, direction);
        return matrix.set_connection_policy(source, target, updated);
    }

    //// multi-route edits; each returns the number of routes changed

    uint8_t disconnect_others_to_target(target_id_t target, source_id_t keep_source = -1) {
        int8_t sources[MAX_NUM_SOURCES];
        const uint8_t count = collect_sources_to(target, sources);
        uint8_t changed = 0;
        for (uint8_t index = 0; index < count; ++index)
            if (sources[index] != keep_source && disconnect(sources[index], target)) ++changed;
        return changed;
    }

    uint8_t disconnect_others_from_source(source_id_t source, target_id_t keep_target = -1) {
        int8_t targets[MAX_NUM_TARGETS];
        const uint8_t count = collect_targets_from(source, targets);
        uint8_t changed = 0;
        for (uint8_t index = 0; index < count; ++index)
            if (targets[index] != keep_target && disconnect(source, targets[index])) ++changed;
        return changed;
    }

    // Pausing releases the route's held notes; resuming never replays them.
    uint8_t set_paused_from_source(source_id_t source, bool paused) {
        int8_t targets[MAX_NUM_TARGETS];
        const uint8_t count = collect_targets_from(source, targets);
        uint8_t changed = 0;
        for (uint8_t index = 0; index < count; ++index)
            if (set_paused_if_different(source, targets[index], paused)) ++changed;
        return changed;
    }

    uint8_t set_paused_to_target(target_id_t target, bool paused) {
        int8_t sources[MAX_NUM_SOURCES];
        const uint8_t count = collect_sources_to(target, sources);
        uint8_t changed = 0;
        for (uint8_t index = 0; index < count; ++index)
            if (set_paused_if_different(sources[index], target, paused)) ++changed;
        return changed;
    }

    uint8_t set_paused_outbound(const MIDIEndpointOwner *owner, bool paused) {
        uint8_t changed = 0;
        matrix.for_each_source_of(owner, [&](source_id_t source) { changed += set_paused_from_source(source, paused); });
        return changed;
    }

    uint8_t set_paused_inbound(const MIDIEndpointOwner *owner, bool paused) {
        uint8_t changed = 0;
        matrix.for_each_target_of(owner, [&](target_id_t target) { changed += set_paused_to_target(target, paused); });
        return changed;
    }

    //// candidates for "add connection": every allowed far end, flagged if already connected

    uint8_t target_candidates_for_source(source_id_t source, MIDIRouteCandidate *out, uint8_t max) {
        uint8_t count = 0;
        for (target_id_t target = 0; target < matrix.targets_count && count < max; ++target) {
            if (!matrix.is_allowed(source, target)) continue;
            fill_candidate(source, target, target, out[count++]);
        }
        return count;
    }

    uint8_t source_candidates_for_target(target_id_t target, MIDIRouteCandidate *out, uint8_t max) {
        uint8_t count = 0;
        for (source_id_t source = 0; source < matrix.sources_count && count < max; ++source) {
            if (!matrix.is_allowed(source, target)) continue;
            fill_candidate(source, target, source, out[count++]);
        }
        return count;
    }

    //// navigation; -1 when the endpoint has no page

    int16_t source_page(source_id_t source) const { return matrix.get_source_page_index(source); }
    int16_t target_page(target_id_t target) const { return matrix.get_target_page_index(target); }

    //// shared formatting

    static MIDIConnectionQuantiseMode next_quantise_mode(MIDIConnectionQuantiseMode mode, int8_t direction = 1) {
        // menu order: off -> inherit -> scale -> chord
        static const MIDIConnectionQuantiseMode order[] = {
            MIDIConnectionQuantiseMode::FORCE_OFF,
            MIDIConnectionQuantiseMode::INHERIT_BEHAVIOUR,
            MIDIConnectionQuantiseMode::FORCE_SCALE,
            MIDIConnectionQuantiseMode::FORCE_CHORD,
        };
        const uint8_t size = sizeof(order) / sizeof(order[0]);
        uint8_t index = 0;
        while (index < size && order[index] != mode) ++index;
        if (index == size) return MIDIConnectionQuantiseMode::FORCE_OFF;
        return order[(index + (direction < 0 ? size - 1 : 1)) % size];
    }

    static const char *quantise_mode_label(MIDIConnectionQuantiseMode mode) {
        switch (mode) {
            case MIDIConnectionQuantiseMode::FORCE_OFF:         return "off";
            case MIDIConnectionQuantiseMode::INHERIT_BEHAVIOUR: return "inherit";
            case MIDIConnectionQuantiseMode::FORCE_SCALE:       return "scale";
            case MIDIConnectionQuantiseMode::FORCE_CHORD:       return "chord";
            default:                                            return "unknown";
        }
    }

    // compact policy summary for list rows, eg "|| c3 Qs"; empty for a default policy
    static void format_policy_badge(char *out, size_t size, const MIDIConnectionPolicy &policy) {
        if (size == 0) return;
        out[0] = '\0';
        size_t used = 0;
        if (policy.paused) append(out, size, used, "||");
        if (policy.fixed_channel > 0) {
            char channel[12];
            snprintf(channel, sizeof(channel), "c%u", (unsigned int)policy.fixed_channel);
            append(out, size, used, channel);
        }
        switch (policy.quantise_mode) {
            case MIDIConnectionQuantiseMode::FORCE_OFF:   append(out, size, used, "Q-"); break;
            case MIDIConnectionQuantiseMode::FORCE_SCALE: append(out, size, used, "Qs"); break;
            case MIDIConnectionQuantiseMode::FORCE_CHORD: append(out, size, used, "Qc"); break;
            default: break;
        }
    }

    static const char *connect_result_message(MIDIRouteConnectResult result) {
        switch (result) {
            case MIDIRouteConnectResult::CONNECTED:         return "Connected";
            case MIDIRouteConnectResult::ALREADY_CONNECTED: return "Already connected";
            case MIDIRouteConnectResult::DISALLOWED:        return "Cannot connect: disallowed";
            case MIDIRouteConnectResult::FULL:              return "Cannot connect: routing table full";
            default:                                        return "Cannot connect: invalid endpoints";
        }
    }

    private:
    MIDIMatrixManager &matrix;

    void fill_row(const MIDIMatrixManager::connection_t &connection, MIDIRouteRow &row) const {
        row.source = connection.source;
        row.target = connection.target;
        row.policy = connection.policy;
        row.held_notes = matrix.held_notes_for_connection(connection.source, connection.target);
        row.last_activity_ms = connection.last_activity_ms;
        row.has_activity = connection.has_activity;
    }

    void add_to_summary(const MIDIMatrixManager::connection_t &connection, MIDIRouteSummary &summary) const {
        ++summary.total;
        if (connection.policy.paused) ++summary.paused;
        summary.held_notes += matrix.held_notes_for_connection(connection.source, connection.target);
    }

    static void merge(MIDIRouteSummary &into, const MIDIRouteSummary &from) {
        into.total += from.total;
        into.paused += from.paused;
        into.held_notes += from.held_notes;
    }

    void fill_candidate(source_id_t source, target_id_t target, int id, MIDIRouteCandidate &candidate) {
        const MIDIConnectionPolicy *policy = matrix.get_connection_policy(source, target);
        candidate.id = (int8_t)id;
        candidate.connected = policy != nullptr;
        candidate.paused = policy != nullptr && policy->paused;
    }

    // Snapshot ids before editing, so edits never run while iterating the store.
    uint8_t collect_targets_from(source_id_t source, int8_t *targets) {
        uint8_t count = 0;
        matrix.for_each_connection_from(source, [&](const MIDIMatrixManager::connection_t &connection) {
            if (count < MAX_NUM_TARGETS) targets[count++] = (int8_t)connection.target;
        });
        return count;
    }

    uint8_t collect_sources_to(target_id_t target, int8_t *sources) {
        uint8_t count = 0;
        matrix.for_each_connection_to(target, [&](const MIDIMatrixManager::connection_t &connection) {
            if (count < MAX_NUM_SOURCES) sources[count++] = (int8_t)connection.source;
        });
        return count;
    }

    bool set_paused_if_different(source_id_t source, target_id_t target, bool paused) {
        const MIDIConnectionPolicy *policy = matrix.get_connection_policy(source, target);
        if (policy == nullptr || policy->paused == paused) return false;
        return matrix.set_paused(source, target, paused);
    }

    static void append(char *out, size_t size, size_t &used, const char *text) {
        const int written = snprintf(out + used, size - used, used > 0 ? " %s" : "%s", text);
        if (written > 0) used += (size_t)written < size - used ? (size_t)written : size - used - 1;
    }
};
