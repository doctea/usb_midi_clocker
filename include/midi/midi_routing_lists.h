#pragma once

// Display-free ActionListSource implementations for editing an endpoint owner's
// matrix routes (the routing panel on behaviour pages and the MIDI Ports page).
// Rendering lives in mymenu/menu_midi_routing.h; these only produce row text and
// perform edits through MIDIRoutingModel, so they can be tested on the host.

#include <stdio.h>
#include <string.h>

#include "menuitems_actionlist_core.h"
#include "midi/midi_routing_model.h"

// implemented by the app UI (and by host tests)
void midi_routing_ui_message(const char *text, bool error);
void midi_routing_ui_jump_to_page(int16_t page_index);

// a route counts as 'active' (marked with '*') for this long after its last note
#define MIDI_ROUTE_ACTIVITY_MS 250

enum class MIDIRouteDirection : uint8_t {
    INBOUND,    // routes arriving at the owner's targets; the far end is a source
    OUTBOUND,   // routes leaving the owner's sources; the far end is a target
};

namespace midi_routing_text {
    inline const char *source_label(MIDIMatrixManager &matrix, source_id_t source) {
        const char *label = matrix.get_label_for_source_id(source);
        return label != nullptr ? label : "?";
    }
    inline const char *target_label(MIDIMatrixManager &matrix, target_id_t target) {
        const char *label = matrix.get_label_for_target_id(target);
        return label != nullptr ? label : "?";
    }
    inline const char *source_role(MIDIMatrixManager &matrix, source_id_t source) {
        const char *role = matrix.get_role_for_source_id(source);
        return role != nullptr ? role : "?";
    }
    inline const char *target_role(MIDIMatrixManager &matrix, target_id_t target) {
        const char *role = matrix.get_role_for_target_id(target);
        return role != nullptr ? role : "?";
    }
    inline bool recently_active(const MIDIRouteRow &row) {
        return row.has_activity && (uint32_t)(millis() - row.last_activity_ms) < MIDI_ROUTE_ACTIVITY_MS;
    }

    // "  *Prog.Bass        || c3 ~2": label left, badge/held right-aligned within size-1 columns
    inline void format_route(char *out, size_t size, const char *indent, const char *label, const MIDIRouteRow &row) {
        if (size == 0) return;
        char badge[24];
        MIDIRoutingModel::format_policy_badge(badge, sizeof(badge), row.policy);
        if (row.held_notes > 0) {
            const size_t used = strlen(badge);
            snprintf(badge + used, sizeof(badge) - used, "%s~%u", used > 0 ? " " : "", (unsigned)row.held_notes);
        }
        const int columns = (int)size - 1;
        const int fixed = (int)strlen(indent) + 1 + (badge[0] ? (int)strlen(badge) + 1 : 0);
        int label_width = columns - fixed;
        if (label_width < 1) label_width = 1;
        snprintf(out, size, "%s%c%-*.*s%s%s", indent, recently_active(row) ? '*' : ' ',
            badge[0] ? label_width : 0, label_width, label, badge[0] ? " " : "", badge);
    }

    inline void append(char *out, size_t size, const char *text) {
        const size_t used = strlen(out);
        if (used + 1 < size) snprintf(out + used, size - used, "%s", text);
    }
}

// eg "Out used by Neutron"; shown for owners linked to a panel's owner (see MIDIEndpointOwnerLink)
inline void midi_routing_format_link(MIDIEndpointOwnerLink link, char *out, size_t size) {
    if (size == 0) return;
    snprintf(out, size, "%s used by %s", link.role != nullptr ? link.role : "Port",
        link.owner != nullptr ? link.owner->get_endpoint_owner_label() : "?");
}

// Comma-separated far ends of an owner's routes in one direction, eg
// "<- Prog.Bass, ||TM Ch2 +1"; used by the closed (compact) panel view.
inline void midi_routing_format_owner_summary(MIDIMatrixManager &matrix, const MIDIEndpointOwner *owner,
        MIDIRouteDirection direction, char *out, size_t size) {
    if (size == 0) return;
    snprintf(out, size, "%s", direction == MIDIRouteDirection::INBOUND ? "<- " : "-> ");
    const size_t prefix = strlen(out);
    uint8_t shown = 0, hidden = 0;
    auto add = [&](const char *label, bool paused) {
        char item[MIDIPortOwner::MAX_LABEL + 8];
        snprintf(item, sizeof(item), "%s%s%s", shown > 0 ? ", " : "", paused ? "||" : "", label);
        // keep room for a " +NN" overflow marker
        if (hidden == 0 && strlen(out) + strlen(item) + 4 < size) {
            midi_routing_text::append(out, size, item);
            ++shown;
        } else {
            ++hidden;
        }
    };
    if (direction == MIDIRouteDirection::INBOUND) {
        matrix.for_each_target_of(owner, [&](target_id_t target) {
            matrix.for_each_connection_to(target, [&](const MIDIMatrixManager::connection_t &connection) {
                add(midi_routing_text::source_label(matrix, connection.source), connection.policy.paused);
            });
        });
    } else {
        matrix.for_each_source_of(owner, [&](source_id_t source) {
            matrix.for_each_connection_from(source, [&](const MIDIMatrixManager::connection_t &connection) {
                add(midi_routing_text::target_label(matrix, connection.target), connection.policy.paused);
            });
        });
    }
    if (shown == 0 && hidden == 0) {
        out[prefix] = '\0';
        midi_routing_text::append(out, size, "(none)");
    } else if (hidden > 0) {
        char more[8];
        snprintf(more, sizeof(more), "%s+%u", shown > 0 ? " " : "", (unsigned)(hidden > 99 ? 99 : hidden));
        midi_routing_text::append(out, size, more);
    }
}

// Edit one route: pause, channel, quantise, jump to the far end, disconnect.
class MIDIRouteEditSource : public ActionListSource {
    public:
        enum Row : uint8_t { STATUS = 0, PAUSE, CHANNEL, QUANTISE, JUMP, DISCONNECT, ROW_COUNT };

        void open(MIDIMatrixManager *matrix, source_id_t source, target_id_t target, MIDIRouteDirection direction) {
            this->matrix = matrix;
            this->source = source;
            this->target = target;
            this->direction = direction;
            snprintf(title_text, sizeof(title_text), "%s -> %s",
                midi_routing_text::source_label(*matrix, source), midi_routing_text::target_label(*matrix, target));
        }

        source_id_t get_source() const { return source; }
        target_id_t get_target() const { return target; }

        virtual const char *title() override { return title_text; }
        virtual const char *hint() override { return "[>]=step back"; }
        virtual uint8_t count() override { return ROW_COUNT; }

        virtual bool row_selectable(uint8_t index) override { return index != STATUS; }
        virtual bool row_enabled(uint8_t index) override {
            if (!exists()) return index == STATUS;
            if (index == JUMP) return far_page() >= 0;
            return true;
        }

        virtual void row_text(uint8_t index, char *out, size_t size) override {
            MIDIRouteRow row;
            const bool connected = model().get_row(source, target, row);
            switch (index) {
                case STATUS:
                    if (!connected) snprintf(out, size, "Disconnected");
                    else snprintf(out, size, "%s%s, %u held", row.policy.paused ? "Paused" : "Live",
                        midi_routing_text::recently_active(row) ? " *" : "", (unsigned)row.held_notes);
                    break;
                case PAUSE:
                    snprintf(out, size, "%s", connected && row.policy.paused ? "Resume" : "Pause");
                    break;
                case CHANNEL:
                    if (connected && row.policy.fixed_channel > 0)
                        snprintf(out, size, "Channel: %u", (unsigned)row.policy.fixed_channel);
                    else
                        snprintf(out, size, "Channel: default");
                    break;
                case QUANTISE:
                    snprintf(out, size, "Quantise: %s",
                        connected ? MIDIRoutingModel::quantise_mode_label(row.policy.quantise_mode) : "-");
                    break;
                case JUMP:
                    snprintf(out, size, "Go to %s", far_label());
                    break;
                case DISCONNECT:
                    snprintf(out, size, "Disconnect");
                    break;
                default:
                    out[0] = '\0';
            }
        }

        virtual ActionListResult on_select(uint8_t index, ActionListNavigator &) override {
            switch (index) {
                case PAUSE:     model().toggle_pause(source, target); break;
                case CHANNEL:   model().cycle_channel(source, target, 1); break;
                case QUANTISE:  model().cycle_quantise(source, target, 1); break;
                case JUMP:      midi_routing_ui_jump_to_page(far_page()); break;
                case DISCONNECT:
                    if (model().disconnect(source, target)) {
                        midi_routing_ui_message("Disconnected", false);
                        return ActionListResult::POP;
                    }
                    break;
                default: break;
            }
            return ActionListResult::STAY;
        }

        // right steps channel/quantise backwards
        virtual bool on_right(uint8_t index, ActionListNavigator &) override {
            if (index == CHANNEL) return model().cycle_channel(source, target, -1);
            if (index == QUANTISE) return model().cycle_quantise(source, target, -1);
            return false;
        }

        virtual uint32_t generation() override {
            MIDIRouteRow row;
            if (!model().get_row(source, target, row)) return matrix->routing_revision;
            return matrix->routing_revision * 131u + row.held_notes * 3u + (midi_routing_text::recently_active(row) ? 1u : 0u);
        }

    private:
        MIDIMatrixManager *matrix = nullptr;
        source_id_t source = -1;
        target_id_t target = -1;
        MIDIRouteDirection direction = MIDIRouteDirection::INBOUND;
        char title_text[48] = "";

        MIDIRoutingModel model() { return MIDIRoutingModel(*matrix); }
        bool exists() { return matrix->is_connected(source, target); }
        int16_t far_page() {
            return direction == MIDIRouteDirection::INBOUND ? model().source_page(source) : model().target_page(target);
        }
        const char *far_label() {
            return direction == MIDIRouteDirection::INBOUND
                ? midi_routing_text::source_label(*matrix, source) : midi_routing_text::target_label(*matrix, target);
        }
};

// Tick-list of every allowed far end for one endpoint; select connects/disconnects.
class MIDIRoutePickerSource : public ActionListSource {
    public:
        static const uint8_t MAX_CANDIDATES = MAX_NUM_SOURCES > MAX_NUM_TARGETS ? MAX_NUM_SOURCES : MAX_NUM_TARGETS;

        void open(MIDIMatrixManager *matrix, int8_t endpoint, MIDIRouteDirection direction) {
            this->matrix = matrix;
            this->endpoint = endpoint;
            this->direction = direction;
            if (direction == MIDIRouteDirection::INBOUND)
                snprintf(title_text, sizeof(title_text), "Inputs to %s", midi_routing_text::target_role(*matrix, endpoint));
            else
                snprintf(title_text, sizeof(title_text), "Outputs from %s", midi_routing_text::source_role(*matrix, endpoint));
            refresh();
        }

        virtual const char *title() override { return title_text; }
        virtual const char *hint() override { return "[sel]=toggle"; }

        virtual void on_enter() override { refresh(); }

        virtual uint8_t count() override {
            if (built_revision != matrix->routing_revision) refresh();
            return candidate_count;
        }

        virtual bool row_dimmed(uint8_t index) override { return candidates[index].paused; }

        virtual uint16_t row_colour(uint8_t index) override {
            MIDIEndpointOwner *owner = far_owner(candidates[index].id);
            return owner != nullptr ? owner->get_endpoint_owner_colour() : 0;
        }

        virtual void row_text(uint8_t index, char *out, size_t size) override {
            const MIDIRouteCandidate &candidate = candidates[index];
            snprintf(out, size, "[%c] %s%s", candidate.connected ? 'x' : ' ', far_label(candidate.id),
                candidate.paused ? " ||" : "");
        }

        virtual ActionListResult on_select(uint8_t index, ActionListNavigator &) override {
            const int8_t far = candidates[index].id;
            const source_id_t source = direction == MIDIRouteDirection::INBOUND ? far : endpoint;
            const target_id_t target = direction == MIDIRouteDirection::INBOUND ? endpoint : far;
            MIDIRoutingModel model(*matrix);
            if (candidates[index].connected) {
                model.disconnect(source, target);
                midi_routing_ui_message("Disconnected", false);
            } else {
                const MIDIRouteConnectResult result = model.connect(source, target);
                midi_routing_ui_message(MIDIRoutingModel::connect_result_message(result),
                    result != MIDIRouteConnectResult::CONNECTED);
            }
            refresh();
            return ActionListResult::STAY;
        }

        virtual uint32_t generation() override { return matrix->routing_revision; }

    private:
        MIDIMatrixManager *matrix = nullptr;
        int8_t endpoint = -1;
        MIDIRouteDirection direction = MIDIRouteDirection::INBOUND;
        MIDIRouteCandidate candidates[MAX_CANDIDATES];
        uint8_t candidate_count = 0;
        uint32_t built_revision = 0;
        char title_text[40] = "";

        void refresh() {
            MIDIRoutingModel model(*matrix);
            candidate_count = direction == MIDIRouteDirection::INBOUND
                ? model.source_candidates_for_target(endpoint, candidates, MAX_CANDIDATES)
                : model.target_candidates_for_source(endpoint, candidates, MAX_CANDIDATES);
            built_revision = matrix->routing_revision;
        }
        const char *far_label(int8_t far) {
            return direction == MIDIRouteDirection::INBOUND
                ? midi_routing_text::source_label(*matrix, far) : midi_routing_text::target_label(*matrix, far);
        }
        MIDIEndpointOwner *far_owner(int8_t far) {
            return direction == MIDIRouteDirection::INBOUND ? matrix->get_source_owner(far) : matrix->get_target_owner(far);
        }
};

// Root list for one owner: an Inputs section (routes into each of its targets)
// and an Outputs section (routes from each of its sources), each with per-endpoint
// '+ Add' pickers and a section-wide Pause/Resume all.
class MIDIOwnerRoutesSource : public ActionListSource {
    public:
        struct entry_t {
            enum Kind : uint8_t { SECTION, ENDPOINT, ROUTE, ADD, PAUSE_ALL, EMPTY, TRUNCATED, LINK };
            Kind kind = EMPTY;
            MIDIRouteDirection direction = MIDIRouteDirection::INBOUND;
            int8_t endpoint = -1;   // owner's target (inbound) or source (outbound); link index for LINK
            int8_t far = -1;        // route's source (inbound) or target (outbound)
        };
        static const uint8_t MAX_ENTRIES = 48;

        MIDIOwnerRoutesSource(MIDIMatrixManager *matrix, MIDIEndpointOwner *owner) : matrix(matrix), owner(owner) {}

        MIDIEndpointOwner *get_owner() const { return owner; }
        bool has_inputs() { return matrix->count_targets_of(owner) > 0; }
        bool has_outputs() { return matrix->count_sources_of(owner) > 0; }
        uint8_t link_count() { return owner->get_linked_owner_count(); }

        virtual const char *title() override {
            ensure_fresh();
            return shared().title;
        }
        virtual const char *hint() override { return "[>]=go to"; }

        virtual void on_enter() override { shared().built_for = nullptr; }

        virtual uint8_t count() override {
            ensure_fresh();
            return shared().count;
        }

        virtual bool row_selectable(uint8_t index) override {
            const entry_t::Kind kind = entry(index).kind;
            return kind == entry_t::ROUTE || kind == entry_t::ADD || kind == entry_t::PAUSE_ALL || kind == entry_t::LINK;
        }

        virtual bool row_enabled(uint8_t index) override {
            const entry_t &e = entry(index);
            if (e.kind == entry_t::PAUSE_ALL) return summary(e.direction).total > 0;
            return true;
        }

        virtual bool row_dimmed(uint8_t index) override {
            const entry_t &e = entry(index);
            if (e.kind == entry_t::LINK) {
                MIDIEndpointOwner *linked = owner->get_linked_owner((uint8_t)e.endpoint).owner;
                return linked != nullptr && !linked->is_endpoint_owner_online();
            }
            if (e.kind != entry_t::ROUTE) return false;
            const MIDIConnectionPolicy *policy = matrix->get_connection_policy(route_source(e), route_target(e));
            MIDIEndpointOwner *far = far_owner(e);
            return (policy != nullptr && policy->paused) || (far != nullptr && !far->is_endpoint_owner_online());
        }

        virtual uint16_t row_colour(uint8_t index) override {
            const entry_t &e = entry(index);
            if (e.kind == entry_t::LINK) {
                MIDIEndpointOwner *linked = owner->get_linked_owner((uint8_t)e.endpoint).owner;
                return linked != nullptr ? linked->get_endpoint_owner_colour() : 0;
            }
            if (e.kind != entry_t::ROUTE) return 0;
            MIDIEndpointOwner *far = far_owner(e);
            return far != nullptr ? far->get_endpoint_owner_colour() : 0;
        }

        virtual void row_text(uint8_t index, char *out, size_t size) override {
            const entry_t &e = entry(index);
            const bool inbound = e.direction == MIDIRouteDirection::INBOUND;
            switch (e.kind) {
                case entry_t::SECTION: {
                    const MIDIRouteSummary s = summary(e.direction);
                    if (s.paused > 0)
                        snprintf(out, size, "%s %u live, %u paused", inbound ? "Inputs <-" : "Outputs ->", (unsigned)s.live(), (unsigned)s.paused);
                    else
                        snprintf(out, size, "%s %u", inbound ? "Inputs <-" : "Outputs ->", (unsigned)s.total);
                    break;
                }
                case entry_t::ENDPOINT:
                    snprintf(out, size, " %s:", inbound ? midi_routing_text::target_role(*matrix, e.endpoint)
                                                        : midi_routing_text::source_role(*matrix, e.endpoint));
                    break;
                case entry_t::ROUTE: {
                    MIDIRouteRow row;
                    if (!MIDIRoutingModel(*matrix).get_row(route_source(e), route_target(e), row)) {
                        snprintf(out, size, "  (removed)");
                        break;
                    }
                    midi_routing_text::format_route(out, size, "  ", far_label(e), row);
                    break;
                }
                case entry_t::ADD:
                    snprintf(out, size, "  + Add %s...", inbound ? "input" : "output");
                    break;
                case entry_t::PAUSE_ALL: {
                    const MIDIRouteSummary s = summary(e.direction);
                    if (s.can_pause_all())
                        snprintf(out, size, " Pause all %s (%u)", inbound ? "inputs" : "outputs", (unsigned)s.live());
                    else
                        snprintf(out, size, " Resume all %s (%u)", inbound ? "inputs" : "outputs", (unsigned)s.paused);
                    break;
                }
                case entry_t::TRUNCATED:
                    snprintf(out, size, "  (more routes not shown)");
                    break;
                case entry_t::LINK:
                    midi_routing_format_link(owner->get_linked_owner((uint8_t)e.endpoint), out, size);
                    break;
                default:
                    snprintf(out, size, "No MIDI routing");
            }
        }

        virtual ActionListResult on_select(uint8_t index, ActionListNavigator &nav) override {
            const entry_t e = entry(index);
            switch (e.kind) {
                case entry_t::ROUTE:
                    editor().open(matrix, route_source(e), route_target(e), e.direction);
                    nav.push(&editor());
                    break;
                case entry_t::ADD:
                    picker().open(matrix, e.endpoint, e.direction);
                    nav.push(&picker());
                    break;
                case entry_t::PAUSE_ALL: {
                    const bool pause = summary(e.direction).can_pause_all();
                    MIDIRoutingModel model(*matrix);
                    const uint8_t changed = e.direction == MIDIRouteDirection::INBOUND
                        ? model.set_paused_inbound(owner, pause) : model.set_paused_outbound(owner, pause);
                    char message[32];
                    snprintf(message, sizeof(message), "%s %u route%s", pause ? "Paused" : "Resumed",
                        (unsigned)changed, changed == 1 ? "" : "s");
                    midi_routing_ui_message(message, false);
                    break;
                }
                case entry_t::LINK:
                    jump_to_link(e);
                    break;
                default: break;
            }
            return ActionListResult::STAY;
        }

        // right on a route or link jumps to the far end's page
        virtual bool on_right(uint8_t index, ActionListNavigator &) override {
            const entry_t &e = entry(index);
            if (e.kind == entry_t::LINK) return jump_to_link(e);
            if (e.kind != entry_t::ROUTE) return false;
            MIDIRoutingModel model(*matrix);
            const int16_t page = e.direction == MIDIRouteDirection::INBOUND
                ? model.source_page(e.far) : model.target_page(e.far);
            if (page < 0) {
                midi_routing_ui_message("No page for that endpoint", true);
                return false;
            }
            midi_routing_ui_jump_to_page(page);
            return true;
        }

        // changes with the route list, held notes and activity, without needing the row buffer
        virtual uint32_t generation() override {
            uint32_t signature = matrix->routing_revision;
            const uint32_t now = millis();
            auto mix = [&](const MIDIMatrixManager::connection_t &connection) {
                const bool active = connection.has_activity && (uint32_t)(now - connection.last_activity_ms) < MIDI_ROUTE_ACTIVITY_MS;
                signature = signature * 31u + matrix->held_notes_for_connection(connection.source, connection.target) * 2u + (active ? 1u : 0u);
            };
            matrix->for_each_target_of(owner, [&](target_id_t target) { matrix->for_each_connection_to(target, mix); });
            matrix->for_each_source_of(owner, [&](source_id_t source) { matrix->for_each_connection_from(source, mix); });
            return signature;
        }

        // row lookup for tests and the panel
        const entry_t &entry(uint8_t index) {
            ensure_fresh();
            static const entry_t empty = entry_t();
            return index < shared().count ? shared().entries[index] : empty;
        }

        // Only one panel is open at a time, so all owners share one row buffer
        // (rebuilt when a different owner uses it or the routing changes) and one
        // editor/picker instance.
        static MIDIRouteEditSource &editor() { static MIDIRouteEditSource instance; return instance; }
        static MIDIRoutePickerSource &picker() { static MIDIRoutePickerSource instance; return instance; }

    private:
        MIDIMatrixManager *matrix;
        MIDIEndpointOwner *owner;

        struct buffer_t {
            const MIDIOwnerRoutesSource *built_for = nullptr;
            uint32_t revision = 0;
            uint8_t count = 0;
            entry_t entries[MAX_ENTRIES];
            char title[40] = "";
        };
        static buffer_t &shared() { static buffer_t buffer; return buffer; }

        void ensure_fresh() {
            buffer_t &buffer = shared();
            if (buffer.built_for == this && buffer.revision == matrix->routing_revision) return;
            buffer.built_for = this;
            buffer.revision = matrix->routing_revision;
            buffer.count = 0;
            snprintf(buffer.title, sizeof(buffer.title), "%s routing%s", owner->get_endpoint_owner_label(),
                owner->is_endpoint_owner_online() ? "" : " (offline)");
            for (uint8_t i = 0; i < link_count(); ++i)
                add(entry_t::LINK, MIDIRouteDirection::INBOUND, (int8_t)i);
            if (has_inputs()) build_section(MIDIRouteDirection::INBOUND);
            if (has_outputs()) build_section(MIDIRouteDirection::OUTBOUND);
            if (buffer.count == 0) add(entry_t::EMPTY, MIDIRouteDirection::INBOUND);
        }

        bool jump_to_link(const entry_t &e) {
            MIDIEndpointOwner *linked = owner->get_linked_owner((uint8_t)e.endpoint).owner;
            if (linked == nullptr || linked->routing_page_index < 0) {
                midi_routing_ui_message("No page for that device", true);
                return false;
            }
            midi_routing_ui_jump_to_page(linked->routing_page_index);
            return true;
        }

        bool add(entry_t::Kind kind, MIDIRouteDirection direction, int8_t endpoint = -1, int8_t far = -1) {
            buffer_t &buffer = shared();
            if (buffer.count >= MAX_ENTRIES) return false;
            // reserve the last slot for the truncation marker
            if (buffer.count == MAX_ENTRIES - 1 && kind != entry_t::TRUNCATED) {
                if (buffer.entries[buffer.count - 1].kind != entry_t::TRUNCATED) {
                    entry_t &marker = buffer.entries[buffer.count++];
                    marker.kind = entry_t::TRUNCATED;
                    marker.direction = direction;
                }
                return false;
            }
            entry_t &e = buffer.entries[buffer.count++];
            e.kind = kind;
            e.direction = direction;
            e.endpoint = endpoint;
            e.far = far;
            return true;
        }

        void build_section(MIDIRouteDirection direction) {
            const bool inbound = direction == MIDIRouteDirection::INBOUND;
            add(entry_t::SECTION, direction);
            const bool several = (inbound ? matrix->count_targets_of(owner) : matrix->count_sources_of(owner)) > 1;
            MIDIRoutingModel model(*matrix);
            auto add_endpoint = [&](int8_t endpoint) {
                if (several) add(entry_t::ENDPOINT, direction, endpoint);
                MIDIRouteRow rows[16];
                const uint8_t found = inbound ? model.list_to_target(endpoint, rows, 16) : model.list_from_source(endpoint, rows, 16);
                for (uint8_t i = 0; i < found; ++i)
                    add(entry_t::ROUTE, direction, endpoint, (int8_t)(inbound ? rows[i].source : rows[i].target));
                add(entry_t::ADD, direction, endpoint);
            };
            if (inbound) matrix->for_each_target_of(owner, [&](target_id_t target) { add_endpoint((int8_t)target); });
            else matrix->for_each_source_of(owner, [&](source_id_t source) { add_endpoint((int8_t)source); });
            add(entry_t::PAUSE_ALL, direction);
        }

        MIDIRouteSummary summary(MIDIRouteDirection direction) {
            MIDIRoutingModel model(*matrix);
            return direction == MIDIRouteDirection::INBOUND ? model.summarise_inbound(owner) : model.summarise_outbound(owner);
        }

        static source_id_t route_source(const entry_t &e) { return e.direction == MIDIRouteDirection::INBOUND ? e.far : e.endpoint; }
        static target_id_t route_target(const entry_t &e) { return e.direction == MIDIRouteDirection::INBOUND ? e.endpoint : e.far; }
        const char *far_label(const entry_t &e) {
            return e.direction == MIDIRouteDirection::INBOUND
                ? midi_routing_text::source_label(*matrix, e.far) : midi_routing_text::target_label(*matrix, e.far);
        }
        MIDIEndpointOwner *far_owner(const entry_t &e) {
            return e.direction == MIDIRouteDirection::INBOUND ? matrix->get_source_owner(e.far) : matrix->get_target_owner(e.far);
        }
};
