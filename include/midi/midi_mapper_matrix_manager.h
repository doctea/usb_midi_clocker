#if (defined __GNUC__) && (__GNUC__ >= 5) && (__GNUC_MINOR__ >= 4) && (__GNUC_PATCHLEVEL__ > 1)
    #pragma GCC diagnostic ignored "-Wpragmas"
    #pragma GCC diagnostic ignored "-Wformat-truncation"
    #pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

#pragma once

#include "Config.h"
#include "midi/midi_outs.h"
#include "midi/midi_out_wrapper.h"
#include "midi_helpers.h"
#include "midi/midi_looper.h"

#include "conductor.h"

#include "Drums.h"

#include "LinkedList.h"

void setup_midi_mapper_matrix_manager();

void behaviour_manager_kill_all_current_notes();

#ifdef ENABLE_SCALES
    void behaviour_manager_notify_harmony_changed();
#endif

//#include "behaviours/behaviour_bamble.h"

#define MAX_NUM_SOURCES 50
#define MAX_NUM_TARGETS 50
#define NUM_REGISTERED_TARGETS targets_count
#define NUM_REGISTERED_SOURCES sources_count

#define LANGST_HANDEL_ROUT 25   // longest possible name of a target handle / get roo to do it <3

#include "midi/midi_mapper_matrix_types.h"
#include "midi/midi_connection_note_state.h"
#include "midi/midi_connection_store.h"
#include "midi/midi_note_diagnostics.h"
#include <memory>
#include <new>

#include "scales.h"

class MIDITrack;
class DeviceBehaviourUltimateBase;

class MIDIMatrixManager : public SHDynamic<0, 8> {
    public:
    static MIDIMatrixManager* getInstance();

    using ConnectionQuantiseMode = MIDIConnectionQuantiseMode;
    using ConnectionNoteMapMode = MIDIConnectionNoteMapMode;
    using connection_policy_t = MIDIConnectionPolicy;
    using connection_store_t = MIDIConnectionStore<MAX_NUM_SOURCES, MAX_NUM_TARGETS>;
    using connection_t = connection_store_t::connection_t;

    struct transformed_note_event_t {
        int8_t pitch;
        uint8_t channel;
        bool drop;
    };

    bool debug = false;

    // #ifdef ENABLE_SCALES
    //     bool    global_quantise_on = false, global_quantise_chord_on = false;
    //     scale_identity_t global_scale_identity = {SCALE_MAJOR, SCALE_ROOT_C};
    //     chord_identity_t global_chord_identity = {CHORD::TRIAD, -1, 0};
    // #endif
    /*int8_t  global_chord_degree = -1;
    CHORD::Type global_chord_type = CHORD::TRIAD;
    int8_t global_chord_inversion = 0;*/

    // so we wanna do something like:-
    //      for each source
    //          store a list of targets it is connected to
    //      this requires a concept of a 'source', so like a 'source wrapper'.. 
    //          devicebehaviours might be a candidate for these?
    //          but there are also serial-midi sources, and sources from loopers...
    //          each source needs to be able to talk to this matrix manager, to know which targets to send notes to
    //      then we have targets (outputwrappers?)
    //          they don't need to know what their sources are?
    //      sometimes a source can also be a target, eg, looper.
    //      
    //  sources:-
    //      midi devices (keyboards and sequencers, usb and serial), with channel
    //      loopers
    //  targets:-
    //      synths (usb and midi), with channel
    //      loopers


    /* stuff for handling sources of midi data */
    struct source_entry {
        char handle[LANGST_HANDEL_ROUT];    // 25 * 24 = 600 bytes
        uint8_t connection_count = 0;
        uint8_t default_channel = 0;
        int16_t menu_page_index = -1;  // index into Menu::pages; -1 = no associated page
        MIDIEndpointOwner *owner = nullptr;
        const char *role = nullptr;    // short display name within owner (eg "ch10"); nullptr = use handle
        uint32_t last_activity_ms = 0;
        bool has_activity = false;
    };
    uint8_t sources_count = 0;

    source_entry *sources = nullptr;

    // assign a source_id for the given name
    // role must point to storage that outlives the matrix (eg a string literal)
    FLASHMEM source_id_t register_source(const char *handle, uint8_t default_channel = 0, MIDIEndpointOwner *owner = nullptr, const char *role = nullptr) {
        if (NUM_REGISTERED_SOURCES >= MAX_NUM_SOURCES || sources == nullptr) {
            Serial_printf(F("!! register_source('%s') failed: source table full (%u/%u) or uninitialised\n"),
                handle ? handle : "(null)", (unsigned int)NUM_REGISTERED_SOURCES, (unsigned int)MAX_NUM_SOURCES);
            return -1;
        }
        //Serial_printf(F("midi_mapper_matrix_manager#register_source() registering handle '%s'\n"), handle);
        // sources table is CALLOC'd so default member initialisers haven't run; reset explicitly
        source_entry &entry = sources[NUM_REGISTERED_SOURCES];
        entry = source_entry{};
        strncpy(entry.handle, handle, LANGST_HANDEL_ROUT);
        entry.handle[LANGST_HANDEL_ROUT - 1] = '\0';
        entry.default_channel = default_channel;
        entry.owner = owner;
        entry.role = role;
        return NUM_REGISTERED_SOURCES++;
    }
    // assign a source_id for the midi track
    //FLASHMEM 
    source_id_t register_source(MIDITrack *loop_track, const char *handle, uint8_t default_channel = 0, MIDIEndpointOwner *owner = nullptr, const char *role = nullptr);
    // assign a source_id for the device; the device becomes the source's owner
    //FLASHMEM 
    source_id_t register_source(DeviceBehaviourUltimateBase *device, const char *handle, uint8_t default_channel = 0, const char *role = nullptr);

    // get id of source for string
    FLASHMEM source_id_t get_source_id_for_handle(const char *handle) {
        for (source_id_t i = 0; i < NUM_REGISTERED_SOURCES ; i++) {
            if (strcmp(handle, sources[i].handle)==0)
                return i;
        }
        Serial_printf(F("!! get_source_id_for_handle couldn't find a source for handle '%s'\n"), handle);
        return -1;
    }

    connection_store_t connections;
    // Authoritative Note On/Off pairing for matrix-routed notes. Behaviour-local
    // notes use DeviceBehaviourUltimateBase::note_tracker instead.
    MIDIConnectionNoteState active_connection_notes;
    routed_note_output_t target_transform_snapshot[MIDI_CONNECTION_NOTE_CAPACITY] = {};
    DeviceBehaviourUltimateBase *target_transform_behaviour = nullptr;
    uint8_t target_transform_change_depth = 0;
    std::unique_ptr<MIDINoteTrace> note_trace;

    bool enable_note_trace(int source = -1, int target = -1) {
        if (source < -1 || source >= sources_count || target < -1 || target >= targets_count) {
            Serial_println(F("!! note trace: invalid source or target filter"));
            return false;
        }
        if (!note_trace) note_trace.reset(new (std::nothrow) MIDINoteTrace);
        if (!note_trace) {
            Serial_println(F("!! note trace: allocation failed"));
            return false;
        }
        note_trace->configure(source, target);
        return true;
    }

    MIDINoteTraceEvent make_note_trace_event(MIDINoteTraceKind kind, int source, int target,
            int8_t input_pitch = -1, uint8_t input_channel = 0, int8_t output_pitch = -1,
            uint8_t output_channel = 0, uint8_t velocity = 0, bool emitted = false, uint32_t started_ms = 0) {
        MIDINoteTraceEvent event;
        event.kind = kind;
        event.source = source;
        event.target = target;
        event.time_ms = millis();
        event.started_ms = started_ms;
        event.input_pitch = input_pitch;
        event.input_channel = input_channel;
        event.output_pitch = output_pitch;
        event.output_channel = output_channel;
        event.velocity = velocity;
        event.emitted = emitted;
        event.global_quantise = conductor->get_global_quantise_mode();
        const auto *policy = get_connection_policy(source, target);
        if (policy != nullptr) {
            event.fixed_channel = policy->fixed_channel;
            event.quantise = static_cast<uint8_t>(policy->quantise_mode);
            event.note_map = static_cast<uint8_t>(policy->note_map_mode);
            event.paused = policy->paused;
        }
        return event;
    }

    void trace_note(MIDINoteTraceKind kind, int source, int target,
            int8_t input_pitch = -1, uint8_t input_channel = 0, int8_t output_pitch = -1,
            uint8_t output_channel = 0, uint8_t velocity = 0, bool emitted = false, uint32_t started_ms = 0) {
        if (note_trace && note_trace->accepts(source, target))
            note_trace->record(make_note_trace_event(kind, source, target, input_pitch, input_channel,
                output_pitch, output_channel, velocity, emitted, started_ms));
    }

    void trace_note(MIDINoteTraceKind kind, const MIDIConnectionNoteState::entry_t &entry) {
        trace_note(kind, entry.source_id, entry.target_id, entry.input_pitch, entry.input_channel,
            entry.output_pitch, entry.output_channel, entry.velocity, entry.emitted, entry.started_ms);
    }

    void dispatch_connection_note(const MIDIConnectionNoteState::entry_t &entry,
            const routed_note_output_t &output, uint8_t velocity, bool on, bool resolved = false) {
        MIDIOutputWrapper *wrapper = targets[entry.target_id].wrapper;
        const bool recording = note_trace && note_trace->accepts(entry.source_id, entry.target_id);
        MIDINoteTraceContext context{note_trace.get(), MIDINoteTraceEvent{}};
        if (recording) {
            context.event = make_note_trace_event(on ? MIDINoteTraceKind::DISPATCH_ON : MIDINoteTraceKind::DISPATCH_OFF,
                entry.source_id, entry.target_id, entry.input_pitch, entry.input_channel,
                output.pitch, output.channel, velocity, output.emitted, entry.started_ms);
            note_trace->record(context.event);
        }
        MIDINoteTraceScope scope(wrapper->note_trace_context, recording ? &context : nullptr);
        if (resolved) {
            if (on) wrapper->send_resolved_routed_note_on(output.pitch, velocity, output.channel);
            else wrapper->send_resolved_routed_note_off(output.pitch, velocity, output.channel);
        } else {
            if (on) wrapper->sendNoteOn(output.pitch, velocity, output.channel);
            else wrapper->sendNoteOff(output.pitch, velocity, output.channel);
        }
    }

    bool is_valid_connection_id_pair(source_id_t source_id, target_id_t target_id) const {
        if (source_id < 0 || target_id < 0)
            return false;
        if (source_id >= NUM_REGISTERED_SOURCES || target_id >= NUM_REGISTERED_TARGETS)
            return false;
        return true;
    }

    const connection_t *find_connection(source_id_t source_id, target_id_t target_id) const {
        if (!is_valid_connection_id_pair(source_id, target_id))
            return nullptr;
        return connections.find(source_id, target_id);
    }

    const connection_policy_t *get_connection_policy(source_id_t source_id, target_id_t target_id) const {
        const connection_t *connection = find_connection(source_id, target_id);
        return connection != nullptr ? &connection->policy : nullptr;
    }

    bool set_connection_policy(source_id_t source_id, target_id_t target_id, const connection_policy_t &policy) {
        connection_t *connection = connections.find(source_id, target_id);
        if (!is_valid_connection_id_pair(source_id, target_id) || connection == nullptr
            || !midi_connection_policy_valid(policy)) {
            Serial_printf(F("!! set_connection_policy: invalid or absent connection %i -> %i, or invalid policy\n"), source_id, target_id);
            return false;
        }
        connection->policy = policy;
        ++routing_revision;
        trace_note(MIDINoteTraceKind::POLICY, source_id, target_id);
        if (policy.paused)
            release_connection_notes(source_id, target_id, true, nullptr, MIDINoteTraceKind::PAUSE_RELEASE);
        else
            migrate_connection_notes(source_id, target_id);
        return true;
    }

    bool set_paused(source_id_t source_id, target_id_t target_id, bool paused) {
        const connection_policy_t *policy = get_connection_policy(source_id, target_id);
        if (policy == nullptr) {
            Serial_printf(F("!! set_paused: absent connection %i -> %i\n"), source_id, target_id);
            return false;
        }
        connection_policy_t updated = *policy;
        updated.paused = paused;
        return set_connection_policy(source_id, target_id, updated);
    }

    template <typename Fn> void for_each_connection_from(source_id_t source_id, const Fn &fn) {
        connections.for_each_from(source_id, [&fn](const connection_t &entry) { fn(entry); });
    }
    template <typename Fn> void for_each_connection_to(target_id_t target_id, const Fn &fn) const {
        connections.for_each_to(target_id, fn);
    }

    uint16_t held_notes_for_connection(source_id_t source_id, target_id_t target_id) const {
        uint16_t count = 0;
        if (active_connection_notes.count() == 0) return 0;
        for (const auto &entry : active_connection_notes.entries)
            if (entry.occupied && entry.emitted && entry.source_id == source_id && entry.target_id == target_id)
                ++count;
        return count;
    }

    void stamp_connection_activity(connection_t &connection, uint32_t now) {
        connection.last_activity_ms = targets[connection.target].last_activity_ms = now;
        connection.has_activity = targets[connection.target].has_activity = true;
    }

    transformed_note_event_t apply_connection_policy_note(source_id_t source_id, target_id_t target_id, int8_t pitch, uint8_t channel) {
        const connection_policy_t *policy = get_connection_policy(source_id, target_id);
        if (policy == nullptr)
            return transformed_note_event_t{pitch, channel, false};
        return apply_connection_policy_note(*policy, pitch, channel);
    }

    transformed_note_event_t apply_connection_policy_note(const connection_policy_t &policy, int8_t pitch, uint8_t channel) {
        transformed_note_event_t event = { pitch, channel, false };
        if (policy.fixed_channel > 0)
            event.channel = policy.fixed_channel;

        if (policy.quantise_mode == ConnectionQuantiseMode::FORCE_OFF) {
            // do nothing
        } else if (policy.quantise_mode == ConnectionQuantiseMode::INHERIT_BEHAVIOUR) {
            if (conductor->get_global_quantise_mode()==QUANTISE_MODE_CHORD) event.pitch = ::quantise_pitch_to_chord(event.pitch, 3);
            if (conductor->get_global_quantise_mode()==QUANTISE_MODE_SCALE) event.pitch = ::quantise_pitch_to_scale(event.pitch);
        } else if (policy.quantise_mode == ConnectionQuantiseMode::FORCE_SCALE) {
            event.pitch = quantise_pitch_to_scale(event.pitch);
        } else if (policy.quantise_mode == ConnectionQuantiseMode::FORCE_CHORD) {
            event.pitch = quantise_pitch_to_chord(event.pitch, 3);
        }

        if (!is_valid_note(event.pitch))
            event.drop = true;
        return event;
    }

    static bool routed_note_outputs_match(const routed_note_output_t &left, const routed_note_output_t &right) {
        return left.emitted == right.emitted
            && (!left.emitted || (left.pitch == right.pitch && left.channel == right.channel));
    }

    // Shared migration engine for mutable connection and target transforms.
    // Releasing every changed old output before starting replacements preserves
    // target refcounts when several held inputs collapse onto the same output.
    template<typename Match, typename ResolvePrevious, typename ResolveNext,
             typename SendOff, typename SendOn, typename Commit>
    int migrate_active_connection_notes(const Match &matches,
                                        const ResolvePrevious &resolve_previous,
                                        const ResolveNext &resolve_next,
                                        const SendOff &send_off,
                                        const SendOn &send_on,
                                        const Commit &commit,
                                        MIDINoteTraceKind reason = MIDINoteTraceKind::MIGRATE) {
        int changed = 0;

        for (uint16_t index = 0; index < MIDI_CONNECTION_NOTE_CAPACITY; ++index) {
            MIDIConnectionNoteState::entry_t &entry = active_connection_notes.entries[index];
            if (!entry.occupied || !matches(entry, index))
                continue;

            routed_note_output_t previous = resolve_previous(entry, index);
            routed_note_output_t next = resolve_next(entry, index);
            if (routed_note_outputs_match(previous, next))
                continue;

            trace_note(reason, entry);
            if (previous.emitted)
                send_off(entry, previous);
            ++changed;
        }

        for (uint16_t index = 0; index < MIDI_CONNECTION_NOTE_CAPACITY; ++index) {
            MIDIConnectionNoteState::entry_t &entry = active_connection_notes.entries[index];
            if (!entry.occupied || !matches(entry, index))
                continue;

            routed_note_output_t previous = resolve_previous(entry, index);
            routed_note_output_t next = resolve_next(entry, index);
            if (routed_note_outputs_match(previous, next))
                continue;

            if (next.emitted)
                send_on(entry, next);
            trace_note(MIDINoteTraceKind::MIGRATE_RESULT, entry.source_id, entry.target_id,
                entry.input_pitch, entry.input_channel, next.pitch, next.channel,
                entry.velocity, next.emitted, entry.started_ms);
            commit(entry, next);
        }

        return changed;
    }

    bool source_regenerates_on_harmony_change(source_id_t source_id) const {
        if (source_id < 0 || source_id >= NUM_REGISTERED_SOURCES || sources == nullptr) return false;
        return sources[source_id].owner != nullptr && sources[source_id].owner->regenerates_notes_on_harmony_change();
    }

    int migrate_connection_notes(source_id_t only_source = -1, target_id_t only_target = -1, bool skip_regenerating_sources = false) {
        return migrate_active_connection_notes(
            [this, only_source, only_target, skip_regenerating_sources](const MIDIConnectionNoteState::entry_t &entry, uint16_t) {
                return (only_source < 0 || entry.source_id == only_source)
                    && (only_target < 0 || entry.target_id == only_target)
                    && !(skip_regenerating_sources && source_regenerates_on_harmony_change(entry.source_id));
            },
            [](const MIDIConnectionNoteState::entry_t &entry, uint16_t) {
                return routed_note_output_t(entry.output_pitch, entry.output_channel, entry.emitted);
            },
            [this](const MIDIConnectionNoteState::entry_t &entry, uint16_t) {
                transformed_note_event_t event = apply_connection_policy_note(
                    entry.source_id, entry.target_id, entry.input_pitch, entry.input_channel
                );
                return routed_note_output_t(event.pitch, event.channel, !event.drop);
            },
            [this](const MIDIConnectionNoteState::entry_t &entry, const routed_note_output_t &previous) {
                dispatch_connection_note(entry, previous, MIDI_MIN_VELOCITY, false);
            },
            [this](const MIDIConnectionNoteState::entry_t &entry, const routed_note_output_t &next) {
                dispatch_connection_note(entry, next, entry.velocity, true);
            },
            [](MIDIConnectionNoteState::entry_t &entry, const routed_note_output_t &next) {
                entry.output_pitch = next.pitch;
                entry.output_channel = next.channel;
                entry.emitted = next.emitted;
            }
        );
    }

    void begin_behaviour_transform_change(DeviceBehaviourUltimateBase *behaviour) {
        if (target_transform_change_depth++ > 0)
            return;
        target_transform_behaviour = behaviour;
        for (uint16_t index = 0; index < MIDI_CONNECTION_NOTE_CAPACITY; ++index) {
            MIDIConnectionNoteState::entry_t &entry = active_connection_notes.entries[index];
            if (!entry.occupied || !entry.emitted
                || !targets[entry.target_id].wrapper->routes_to_behaviour(behaviour)) {
                target_transform_snapshot[index] = routed_note_output_t{};
                continue;
            }
            target_transform_snapshot[index] = targets[entry.target_id].wrapper->resolve_routed_note_output(
                entry.output_pitch, entry.output_channel
            );
        }
    }

    int end_behaviour_transform_change(DeviceBehaviourUltimateBase *behaviour) {
        if (target_transform_change_depth == 0 || --target_transform_change_depth > 0)
            return 0;
        if (target_transform_behaviour != behaviour) {
            target_transform_behaviour = nullptr;
            return 0;
        }

        int changed = migrate_active_connection_notes(
            [this, behaviour](const MIDIConnectionNoteState::entry_t &entry, uint16_t) {
                return entry.emitted && targets[entry.target_id].wrapper->routes_to_behaviour(behaviour);
            },
            [this](const MIDIConnectionNoteState::entry_t &, uint16_t index) {
                return target_transform_snapshot[index];
            },
            [this](const MIDIConnectionNoteState::entry_t &entry, uint16_t) {
                return targets[entry.target_id].wrapper->resolve_routed_note_output(
                    entry.output_pitch, entry.output_channel
                );
            },
            [this](const MIDIConnectionNoteState::entry_t &entry, const routed_note_output_t &previous) {
                dispatch_connection_note(entry, previous, MIDI_MIN_VELOCITY, false, true);
            },
            [this](const MIDIConnectionNoteState::entry_t &entry, const routed_note_output_t &next) {
                dispatch_connection_note(entry, next, entry.velocity, true, true);
            },
            [](MIDIConnectionNoteState::entry_t &, const routed_note_output_t &) {},
            MIDINoteTraceKind::TARGET_MIGRATE
        );
        target_transform_behaviour = nullptr;
        return changed;
    }

    int release_connection_notes(source_id_t only_source = -1, target_id_t only_target = -1,
                                 bool emit_note_offs = true, uint32_t *channel_mask = nullptr,
                                 MIDINoteTraceKind reason = MIDINoteTraceKind::CLEANUP) {
        int released = 0;
        for (uint16_t index = 0; index < MIDI_CONNECTION_NOTE_CAPACITY; ++index) {
            MIDIConnectionNoteState::entry_t &entry = active_connection_notes.entries[index];
            if (!entry.occupied
                || (only_source >= 0 && entry.source_id != only_source)
                || (only_target >= 0 && entry.target_id != only_target)) {
                continue;
            }
            trace_note(emit_note_offs ? reason : MIDINoteTraceKind::PANIC_DISCARD, entry);
            if (entry.emitted) {
                uint8_t output_channel = entry.output_channel;
                if (output_channel == 0)
                    output_channel = targets[entry.target_id].wrapper->default_channel;
                if (channel_mask != nullptr && output_channel <= 16)
                    *channel_mask |= (uint32_t)1 << output_channel;
                if (emit_note_offs)
                    dispatch_connection_note(entry, routed_note_output_t(entry.output_pitch, entry.output_channel, true),
                        MIDI_MIN_VELOCITY, false);
            }
            active_connection_notes.erase(entry);
            ++released;
        }
        return released;
    }

    // Apply fixed-channel remap for non-note events (CC, pitchbend).
    uint8_t apply_connection_policy_channel(source_id_t source_id, target_id_t target_id, uint8_t channel) {
        const connection_policy_t *policy = get_connection_policy(source_id, target_id);
        return policy != nullptr ? apply_connection_policy_channel(*policy, channel) : channel;
    }
    uint8_t apply_connection_policy_channel(const connection_policy_t &policy, uint8_t channel) {
        return policy.fixed_channel > 0 ? policy.fixed_channel : channel;
    }

    // reset all connections (eg loading preset)
    void reset_matrix() {
        trace_note(MIDINoteTraceKind::RESET, -1, -1);
        release_connection_notes();
        connections.clear();
        ++routing_revision;
        for (source_id_t source_id = 0; source_id < NUM_REGISTERED_SOURCES; ++source_id) {
            sources[source_id].connection_count = 0;
            sources[source_id].has_activity = false;
        }
        for (target_id_t target_id = 0; target_id < NUM_REGISTERED_TARGETS; ++target_id) {
            targets[target_id].connection_count = 0;
            targets[target_id].has_activity = false;
        }
    }
    // is this source id connected to target id
    bool is_connected(source_id_t source_id, target_id_t target_id) {
        if (source_id < 0 || target_id < 0 || source_id >= NUM_REGISTERED_SOURCES || target_id >= NUM_REGISTERED_TARGETS)
            return false;
        return connections.find(source_id, target_id) != nullptr;
    }

    // toggle a connection - return false if connection is now broken, true if it is now made
    bool toggle_connect(source_id_t source_id, target_id_t target_id) {
        if (is_connected(source_id, target_id)) {
            this->disconnect(source_id, target_id);
            return false;
        } else {
            return this->connect(source_id, target_id);
        }
    }

    // don't allow source X to connect to target Y -- eg to avoid loopbacks
    void disallow(source_id_t source_id, target_id_t target_id) {
        if (!is_valid_connection_id_pair(source_id, target_id) || !connections.disallow(source_id, target_id)) {
            Serial_printf(F("!! disallow: invalid pair or disallow table full (%i -> %i)\n"), source_id, target_id);
            return;
        }
        disconnect(source_id, target_id);
    }
    bool is_allowed(source_id_t source_id, target_id_t target_id) {
        if (!is_valid_connection_id_pair(source_id, target_id) || !connections.allowed(source_id, target_id))
            return false;
        MIDIEndpointOwner *owner = get_source_owner(source_id);
        return owner == nullptr || owner != get_target_owner(target_id) || owner->allows_self_routing();
    }
    
    // connect source to target)
    void connect(MIDITrack *source_track, DeviceBehaviourUltimateBase *target_behaviour);
    void connect(DeviceBehaviourUltimateBase *source_behaviour, const char *target_handle);
    void connect(const char *source_handle, const char *target_handle) {
        if (source_handle==nullptr || target_handle==nullptr) return;
        this->connect(
            this->get_source_id_for_handle(source_handle),
            this->get_target_id_for_handle(target_handle)
        );
    }

    bool connect(source_id_t source_id, target_id_t target_id) {
        if (!is_valid_connection_id_pair(source_id, target_id) || sources == nullptr) {
            Serial_printf(F("!! connect: invalid endpoints %i -> %i\n"), source_id, target_id);
            return false;
        }
        if (!is_allowed(source_id, target_id)) {
            Serial_printf(F("!! connect: disallowed connection %i -> %i\n"), source_id, target_id);
            return false;
        }
        if (is_connected(source_id, target_id)) return true;
        connection_policy_t policy;
        bool source_is_drums = sources[source_id].default_channel == GM_CHANNEL_DRUMS;
        bool target_is_drums = targets[target_id].wrapper != nullptr
                            && targets[target_id].wrapper->default_channel == GM_CHANNEL_DRUMS;
        if (source_is_drums || target_is_drums)
            policy.quantise_mode = ConnectionQuantiseMode::FORCE_OFF;
        if (connections.connect(source_id, target_id, policy) == nullptr) {
            Serial_printf(F("!! connect: connection table full (%i -> %i)\n"), source_id, target_id);
            return false;
        }
        sources[source_id].connection_count++;
        targets[target_id].connection_count++;
        ++routing_revision;
        trace_note(MIDINoteTraceKind::CONNECT, source_id, target_id);
        return true;
    }

    void disconnect(const char *source_handle, const char *target_handle) {
        if (source_handle==nullptr || target_handle==nullptr) return;
        this->disconnect(
            this->get_source_id_for_handle(source_handle),
            this->get_target_id_for_handle(target_handle)
        );
    }
    void disconnect(source_id_t source_id, target_id_t target_id) {
        if (!is_valid_connection_id_pair(source_id, target_id) || sources == nullptr) return;
        if (is_connected(source_id, target_id)) {
            trace_note(MIDINoteTraceKind::DISCONNECT, source_id, target_id);
            release_connection_notes(source_id, target_id);
            // decrement counts, only if connected
            sources[source_id].connection_count--;
            targets[target_id].connection_count--;
            connections.disconnect(source_id, target_id);
            ++routing_revision;
        }
    }

    uint8_t connected_to_source_count(source_id_t source_id) {
        if (source_id < 0 || source_id >= NUM_REGISTERED_SOURCES || sources == nullptr) return 0;

        return sources[source_id].connection_count;
    }
    uint8_t connected_to_target_count(target_id_t target_id) {
        if (target_id < 0 || target_id >= NUM_REGISTERED_TARGETS) return 0;

        return targets[target_id].connection_count;
    }

    // process quantisation
    /*int8_t quantise_pitch(int8_t pitch) {
        return ::quantise_pitch_to_scale(pitch);
    }
    int8_t quantise_chord(int8_t pitch, int8_t distance_threshold) {
        return ::quantise_pitch_to_chord(pitch, distance_threshold);
    }*/
    #ifdef ENABLE_SCALES
        PROGMEM
        // TODO: move this to conductor
        int8_t do_quant(int8_t pitch, int8_t channel) {
            if (channel!=GM_CHANNEL_DRUMS) {
                if (conductor->get_global_quantise_mode()==QUANTISE_MODE_CHORD) pitch = ::quantise_pitch_to_chord(pitch, 3);
                if (conductor->get_global_quantise_mode()==QUANTISE_MODE_SCALE) pitch = ::quantise_pitch_to_scale(pitch);
            }
            return pitch;
        }
    #endif

    ///// handle incoming or generated events (from a midi device, looper, etc) and route to connected outputs
    // Note On calculates the mutable connection transform once and records its
    // result. Note Off below consumes that result; it must not quantise again.
    void processNoteOn(source_id_t source_id, int8_t pitch, uint8_t velocity, uint8_t channel = 0) {
        if (source_id < 0 || source_id >= NUM_REGISTERED_SOURCES || sources == nullptr) {
            Serial_printf(F("!! midi_mapper_matrix_manager#processNoteOn() passed source_id of %i!\n"), source_id);
            return;
        }
        if (this->debug) Serial_printf(F("midi_mapper_matrix_manager#processNoteOn(source_id=%i (%s),\tpitch=%i (%s),\tvelocity=%i,\tchannel=%i)\n"), source_id, this->sources[source_id].handle, pitch, get_note_name_c(pitch), velocity, channel);
        if (!is_valid_note(pitch)) { 
            if (this->debug) Serial_printf("!! midi_mapper_matrix_manager#processNoteOff() passed invalid pitch %s (%i) - ignoring\n", get_note_name_c(pitch), pitch);
            return;
        }

        /*if (debug) Serial_printf(F("!! midi_mapper_matrix_manager#processNoteOn(source_id=%i,\tpitch=%i (%s),\tvelocity=%i,\tchannel=%i) "), source_id, pitch, get_note_name_c(pitch), velocity, channel);
        pitch = do_quant(pitch, channel); 
        if (debug) Serial_printf(F("=> quantised to %i (%s)\n"), pitch, get_note_name_c(pitch));
        if (!is_valid_note(pitch)) return;*/

        const uint32_t now = millis();
        trace_note(MIDINoteTraceKind::SOURCE_ON, source_id, -1, pitch, channel, -1, 0, velocity);
        sources[source_id].last_activity_ms = now;
        sources[source_id].has_activity = true;
        connections.for_each_from(source_id, [&](connection_t &connection) {
            if (!connection.policy.paused) {
                const target_id_t target_id = connection.target;
                stamp_connection_activity(connection, now);
                //targets[target_id].wrapper->debug = true;
                if (this->debug) Serial_printf(F("\tsource %i aka %s\tshould send ON  to\t%i aka %s\t(source_id=%i)\n"), source_id, sources[source_id].handle, target_id, targets[target_id].handle);
                transformed_note_event_t event = apply_connection_policy_note(connection.policy, pitch, channel);
                MIDIConnectionNoteState::entry_t *entry = active_connection_notes.record(
                    source_id, target_id, pitch, channel,
                    event.pitch, event.channel, velocity, !event.drop, now
                );
                if (entry == nullptr) {
                    trace_note(MIDINoteTraceKind::LEDGER_FULL, source_id, target_id, pitch, channel,
                        event.pitch, event.channel, velocity);
                    Serial_printf(F("!! MIDI connection note ledger full; dropping Note On from source %i to target %i\n"), source_id, target_id);
                    return;
                }
                trace_note(MIDINoteTraceKind::PAIRED_ON, *entry);
                if (entry->emitted)
                    dispatch_connection_note(*entry, routed_note_output_t(entry->output_pitch, entry->output_channel, true), velocity, true);
                //targets[target_id].wrapper->debug = false;
            } else {
                trace_note(MIDINoteTraceKind::PAUSED_ON, source_id, connection.target, pitch, channel, -1, 0, velocity);
            }
        });
    }
    // Match by the original source pitch/channel, then release the output that
    // was recorded by processNoteOn even if harmony or policy has since changed.
    void processNoteOff(source_id_t source_id, int8_t pitch, uint8_t velocity, uint8_t channel = 0) {
        if (this->debug) Serial_printf(F("midi_mapper_matrix_manager#processNoteOff(source_id=%i,\tpitch=%i,\tvelocity=%i,\tchannel=%i)\n"), source_id, pitch, velocity, channel);

        if (source_id < 0 || source_id >= NUM_REGISTERED_SOURCES || sources == nullptr) {
            Serial_printf(F("\t!! midi_mapper_matrix_manager#processNoteOff() passed source_id of %i!\n"), source_id);
            return;
        }
        if (!is_valid_note(pitch)) {
            if (this->debug) Serial_printf("midi_mapper_matrix_manager#processNoteOff() passed invalid pitch %s (%i) - ignoring\n", get_note_name_c(pitch), pitch);
            return;
        }

        /*if (debug) Serial_printf(F("midi_mapper_matrix_manager#processNoteOff(source_id=%i,\tpitch=%i (%s),\tvelocity=%i,\tchannel=%i) "), source_id, pitch, get_note_name_c(pitch), velocity, channel);
        pitch = do_quant(pitch, channel);
        if (debug) Serial_printf(F("=> quantised to %i (%s)\n"), pitch, get_note_name_c(pitch));*/

        if (!is_valid_note(pitch)) {
            if (this->debug) Serial_printf("midi_mapper_matrix_manager#processNoteOff() requantised pitch to %s (%i) - so is now invalid - so ignoring!\n", get_note_name_c(pitch), pitch);
            return;
        }

        const uint32_t now = millis();
        trace_note(MIDINoteTraceKind::SOURCE_OFF, source_id, -1, pitch, channel, -1, 0, velocity);
        sources[source_id].last_activity_ms = now;
        sources[source_id].has_activity = true;
        connections.for_each_from(source_id, [&](connection_t &connection) {
            if (!connection.policy.paused) {
                const target_id_t target_id = connection.target;
                stamp_connection_activity(connection, now);
                //targets[target_id].wrapper->debug = true;
                if (this->debug) Serial_printf(F("\t%s\tshould send to\t%s\t(target_id=%i)\n"), sources[source_id].handle, targets[target_id].handle, target_id);
                if (this->debug) Serial_printf(F("\tsource %i aka %s\tshould send OFF to\t%i aka %s\t(source_id=%i)\n"), source_id, sources[source_id].handle, target_id, targets[target_id].handle);
                MIDIConnectionNoteState::entry_t entry;
                if (!active_connection_notes.take(source_id, target_id, pitch, channel, entry)) {
                    trace_note(MIDINoteTraceKind::UNMATCHED_OFF, source_id, target_id, pitch, channel, -1, 0, velocity);
                    if (this->debug) Serial_printf(F("\tno paired Note On for source %i, target %i, pitch %i, channel %i; ignoring Note Off\n"), source_id, target_id, pitch, channel);
                    return;
                }
                trace_note(MIDINoteTraceKind::PAIRED_OFF, entry);
                if (entry.emitted)
                    dispatch_connection_note(entry, routed_note_output_t(entry.output_pitch, entry.output_channel, true), velocity, false);
                //targets[target_id].wrapper->debug = false;
            } else {
                trace_note(MIDINoteTraceKind::PAUSED_OFF, source_id, connection.target, pitch, channel, -1, 0, velocity);
            }
        });
    }
    void processControlChange(source_id_t source_id, int8_t cc, uint8_t value, uint8_t channel = 0) {
        if (source_id < 0 || source_id >= NUM_REGISTERED_SOURCES || sources == nullptr) {
            Serial_printf(F("!! processControlChange: invalid source %i\n"), source_id);
            return;
        }
        const uint32_t now = millis();
        sources[source_id].last_activity_ms = now;
        sources[source_id].has_activity = true;
        connections.for_each_from(source_id, [&](connection_t &connection) {
            if (!connection.policy.paused) {
                const target_id_t target_id = connection.target;
                stamp_connection_activity(connection, now);
                /*Serial_printf("midi_matrix_manager#processControlChange(%i, %i, %i, %i): %i is connected to %i!\n",
                    source_id, cc, value, channel, source_id, target_id
                ); Serial_flush();
                if (targets[target_id].wrapper==nullptr) {
                    Serial_printf("target_id %i has a nullptr wrapper!\n", target_id); Serial_flush();
                }*/
                targets[target_id].wrapper->sendControlChange(cc, value, apply_connection_policy_channel(connection.policy, channel));
                //Serial_println("successfully sent!"); Serial_flush();
            }
        });
    }
    void processPitchBend(source_id_t source_id, int16_t bend, uint8_t channel = 0) {
        if (source_id < 0 || source_id >= NUM_REGISTERED_SOURCES || sources == nullptr) {
            Serial_printf(F("!! processPitchBend: invalid source %i\n"), source_id);
            return;
        }
        const uint32_t now = millis();
        sources[source_id].last_activity_ms = now;
        sources[source_id].has_activity = true;
        connections.for_each_from(source_id, [&](connection_t &connection) {
            if (!connection.policy.paused) {
                const target_id_t target_id = connection.target;
                stamp_connection_activity(connection, now);
                /*Serial_printf("midi_matrix_manager#processControlChange(%i, %i, %i, %i): %i is connected to %i!\n",
                    source_id, cc, value, channel, source_id, target_id
                ); Serial_flush();
                if (targets[target_id].wrapper==nullptr) {
                    Serial_printf("target_id %i has a nullptr wrapper!\n", target_id); Serial_flush();
                }*/
                targets[target_id].wrapper->sendPitchBend(bend, apply_connection_policy_channel(connection.policy, channel));
                //Serial_println("successfully sent!"); Serial_flush();
            }
        });
    }
    void stop_all_notes_for_source(source_id_t source_id, bool force = false) {
        if (source_id==-1) return;
        if (force) {
            for (target_id_t target_id = 0; target_id < NUM_REGISTERED_TARGETS; target_id++) {
                if (is_connected(source_id, target_id))
                    stop_all_notes_for_target(target_id, true);
            }
            return;
        }
        release_connection_notes(source_id);
    }
    void stop_all_notes_for_target(target_id_t target_id, bool force = false) {
        if (target_id==-1 || target_id >= NUM_REGISTERED_TARGETS) return;
        force = force || targets[target_id].wrapper->always_force_stop_all;
        Serial_printf("stop_all_notes_for_target on target_id=%i wrapper %s\n", target_id, targets[target_id].wrapper->label); Serial_flush();
        if (!force) {
            release_connection_notes(-1, target_id);
            targets[target_id].wrapper->stop_all_notes(false);
            return;
        }

        uint32_t channel_mask = 0;
        release_connection_notes(-1, target_id, false, &channel_mask);
        uint8_t default_channel = targets[target_id].wrapper->default_channel;
        if (default_channel <= 16)
            channel_mask |= (uint32_t)1 << default_channel;
        for (uint8_t channel = 0; channel <= 16; channel++) {
            if (channel_mask & ((uint32_t)1 << channel)) {
                trace_note(MIDINoteTraceKind::PANIC_CHANNEL, -1, target_id, -1, 0, -1, channel);
                targets[target_id].wrapper->force_stop_all_notes_on_channel(channel);
            }
        }
    }

    void stop_all_notes(bool force = false) {
        for (target_id_t target_id = 0 ; target_id < NUM_REGISTERED_TARGETS ; target_id++) {
            Serial_printf("stop_all_notes on target_id=%i..\n", target_id); Serial_flush();
            this->stop_all_notes_for_target(target_id, force);
        }
    }
    void stop_all_notes_force() {
        this->stop_all_notes(true);
    }

    // bumped on every connect/disconnect/policy change/reset, so views can cache route lists
    uint32_t routing_revision = 0;

    const char *get_label_for_source_id(source_id_t source_id) {
        if (source_id < 0 || source_id >= NUM_REGISTERED_SOURCES || source_id >= MAX_NUM_SOURCES || this->sources == nullptr)
            return nullptr;
        return this->sources[source_id].handle;
    }
    const char *get_label_for_target_id(target_id_t target_id) {
        if (target_id < 0 || target_id >= NUM_REGISTERED_TARGETS || target_id >= MAX_NUM_TARGETS)
            return nullptr;
        if(this->targets[target_id].wrapper!=nullptr) 
            return this->targets[target_id].wrapper->label;
        return (const char*)F("[error - unknown]");
    }

    uint8_t getDefaultChannelForTargetId(target_id_t target_id) {
        if (target_id>=0 && target_id < NUM_REGISTERED_TARGETS)
            return this->targets[target_id].wrapper->default_channel;
        return 0;
    }

    //// stuff for handling targets of midi data
    struct target_entry {
        char handle[LANGST_HANDEL_ROUT];
        uint8_t connection_count = 0;
        MIDIOutputWrapper *wrapper = nullptr;
        int16_t menu_page_index = -1;  // index into Menu::pages; -1 = no associated page
        MIDIEndpointOwner *owner = nullptr;
        const char *role = nullptr;    // short display name within owner (eg "Voice A"); nullptr = use handle
        uint32_t last_activity_ms = 0;
        bool has_activity = false;
    };

    uint8_t targets_count = 0;
    target_entry targets[MAX_NUM_TARGETS] = {};

    void set_source_page_index(source_id_t source_id, int16_t page_index) {
        if (source_id >= 0 && source_id < NUM_REGISTERED_SOURCES && source_id < MAX_NUM_SOURCES && sources != nullptr)
            sources[source_id].menu_page_index = page_index;
    }
    void set_target_page_index(target_id_t target_id, int16_t page_index) {
        if (target_id >= 0 && target_id < NUM_REGISTERED_TARGETS && target_id < MAX_NUM_TARGETS)
            targets[target_id].menu_page_index = page_index;
    }
    // explicit per-endpoint page index wins; otherwise fall back to the owner's page
    int16_t get_source_page_index(source_id_t source_id) const {
        if (source_id >= 0 && source_id < NUM_REGISTERED_SOURCES && source_id < MAX_NUM_SOURCES && sources != nullptr) {
            if (sources[source_id].menu_page_index >= 0)
                return sources[source_id].menu_page_index;
            if (sources[source_id].owner != nullptr)
                return sources[source_id].owner->routing_page_index;
        }
        return -1;
    }
    int16_t get_target_page_index(target_id_t target_id) const {
        if (target_id >= 0 && target_id < NUM_REGISTERED_TARGETS && target_id < MAX_NUM_TARGETS) {
            if (targets[target_id].menu_page_index >= 0)
                return targets[target_id].menu_page_index;
            if (targets[target_id].owner != nullptr)
                return targets[target_id].owner->routing_page_index;
        }
        return -1;
    }

    //// endpoint ownership
    MIDIEndpointOwner *get_source_owner(source_id_t source_id) const {
        if (source_id < 0 || source_id >= NUM_REGISTERED_SOURCES || sources == nullptr) return nullptr;
        return sources[source_id].owner;
    }
    MIDIEndpointOwner *get_target_owner(target_id_t target_id) const {
        if (target_id < 0 || target_id >= NUM_REGISTERED_TARGETS) return nullptr;
        return targets[target_id].owner;
    }
    void set_source_owner(source_id_t source_id, MIDIEndpointOwner *owner, const char *role = nullptr) {
        if (source_id < 0 || source_id >= NUM_REGISTERED_SOURCES || sources == nullptr) return;
        sources[source_id].owner = owner;
        if (role != nullptr) sources[source_id].role = role;
    }
    void set_target_owner(target_id_t target_id, MIDIEndpointOwner *owner, const char *role = nullptr) {
        if (target_id < 0 || target_id >= NUM_REGISTERED_TARGETS) return;
        targets[target_id].owner = owner;
        if (role != nullptr) targets[target_id].role = role;
    }
    // short label for an endpoint within its owner's context; falls back to the full handle/label
    const char *get_role_for_source_id(source_id_t source_id) {
        if (source_id < 0 || source_id >= NUM_REGISTERED_SOURCES || sources == nullptr) return nullptr;
        return sources[source_id].role != nullptr ? sources[source_id].role : get_label_for_source_id(source_id);
    }
    const char *get_role_for_target_id(target_id_t target_id) {
        if (target_id < 0 || target_id >= NUM_REGISTERED_TARGETS) return nullptr;
        return targets[target_id].role != nullptr ? targets[target_id].role : get_label_for_target_id(target_id);
    }

    template<typename F> void for_each_source_of(const MIDIEndpointOwner *owner, F fn) {
        if (owner == nullptr || sources == nullptr) return;
        for (source_id_t i = 0; i < NUM_REGISTERED_SOURCES; i++)
            if (sources[i].owner == owner) fn(i);
    }
    template<typename F> void for_each_target_of(const MIDIEndpointOwner *owner, F fn) {
        if (owner == nullptr) return;
        for (target_id_t i = 0; i < NUM_REGISTERED_TARGETS; i++)
            if (targets[i].owner == owner) fn(i);
    }
    uint8_t count_sources_of(const MIDIEndpointOwner *owner) {
        uint8_t n = 0;
        for_each_source_of(owner, [&n](source_id_t) { n++; });
        return n;
    }
    uint8_t count_targets_of(const MIDIEndpointOwner *owner) {
        uint8_t n = 0;
        for_each_target_of(owner, [&n](target_id_t) { n++; });
        return n;
    }
    bool owns_endpoints(const MIDIEndpointOwner *owner) {
        return count_sources_of(owner) > 0 || count_targets_of(owner) > 0;
    }

    // built-in ports that have no behaviour (serial MIDI, PC USB, drum looper...);
    // the 'MIDI Ports' page shows one section per entry
    static const uint8_t MAX_PORT_OWNERS = 16;
    MIDIEndpointOwner *port_owners[MAX_PORT_OWNERS] = {};
    uint8_t port_owners_count = 0;
    MIDIEndpointOwner *register_port_owner(MIDIEndpointOwner *owner) {
        if (owner == nullptr) return nullptr;
        if (port_owners_count >= MAX_PORT_OWNERS) {
            Serial_printf(F("!! register_port_owner('%s') failed: table full\n"), owner->get_endpoint_owner_label());
            return owner;
        }
        port_owners[port_owners_count++] = owner;
        return owner;
    }

    FLASHMEM target_id_t register_target(MIDIOutputWrapper *target) {
        return this->register_target(target, target->label);
    }
    FLASHMEM target_id_t register_target(MIDITrack *target, const char *handle, uint8_t channel = 1, MIDIEndpointOwner *owner = nullptr, const char *role = nullptr) {
        return this->register_target(make_midioutputwrapper(handle, target, channel), handle, owner, role);
    }
    // owner defaults to the wrapper's own owner (eg the behaviour it delivers to)
    FLASHMEM target_id_t register_target(MIDIOutputWrapper *target, const char *handle, MIDIEndpointOwner *owner = nullptr, const char *role = nullptr) {
        if (NUM_REGISTERED_TARGETS >= MAX_NUM_TARGETS) {
            Serial_printf(F("!! register_target('%s') failed: target table full (%u/%u)\n"),
                handle ? handle : "(null)", (unsigned int)NUM_REGISTERED_TARGETS, (unsigned int)MAX_NUM_TARGETS);
            return -1;
        }
        // TODO: detect and warn if duplicate handle used
        strncpy(targets[NUM_REGISTERED_TARGETS].handle, handle, LANGST_HANDEL_ROUT);
        targets[NUM_REGISTERED_TARGETS].handle[LANGST_HANDEL_ROUT - 1] = '\0';
        targets[NUM_REGISTERED_TARGETS].wrapper = target;
        targets[NUM_REGISTERED_TARGETS].owner = owner != nullptr ? owner : (target != nullptr ? target->get_owner() : nullptr);
        targets[NUM_REGISTERED_TARGETS].role = role;
        Serial_printf(F("midi_mapper_matrix_manager#register_target() registering handle '%s' as target_id %i\n"), handle, NUM_REGISTERED_TARGETS);
        if (target==nullptr) {
            Serial_printf(F("WARNING: register_target for handle %s (target_id %i) passed a null target wrapper!!!\n"), target, NUM_REGISTERED_TARGETS);
        }
        return NUM_REGISTERED_TARGETS++;
    }
    
    target_id_t get_target_id_for_handle(const char *handle) {
        //Serial_printf(F("get_target_id_for_handle(%s)\n"), handle);
        for (unsigned int i = 0 ; i < NUM_REGISTERED_TARGETS ; i++) {
            //Serial_printf(F("\t%i: looking for '%s', comparing '%s'..\n"), i, handle, targets[i].handle);
            if (strcmp(handle, targets[i].handle)==0) {
                return i;
            }
        }
        Serial_printf(F("!! get_target_id_for_handle couldn't find a target for handle '%s'\n"), handle);
        return -1;
    }
    MIDIOutputWrapper *get_target_for_handle(char *handle) {
        //return this->targets[this->get_target_id_for_handle(handle)].wrapper;
        return this->get_target_for_id(this->get_target_id_for_handle(handle));
    }
    MIDIOutputWrapper *get_target_for_id(target_id_t target_id) {
        if (target_id>=0 && target_id < NUM_REGISTERED_TARGETS && targets[target_id].wrapper!=nullptr)
            return targets[target_id].wrapper;
        return nullptr;
    }

    // look up the serial midi number for a given uart device
    serial_midi_number_t get_serial_midi_number_for_device(midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *device) {
        for (int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
            if (device==midi_out_serial[i])
                return i;
        }
        return -1;
    }

    #ifdef ENABLE_SCALES
        quantise_mode_t previous_global_quantise_mode = QUANTISE_MODE_NONE;

        /////// scale and quantisation functions
        // getters & setters for quantisation on/offs
        // void set_global_quantise_on(bool v, bool requantise_immediately = true) {
        //     bool changed = v != conductor->is_global_quantise_on();
        //     conductor->set_global_quantise_on(v);
        //     if (changed && requantise_immediately) {
        //         //Serial_println(F("set_global_quantise_on() about to call behaviour_manager_requantise_all_notes()"));
        //         behaviour_manager_requantise_all_notes();
        //     }
        // }
        // bool is_global_quantise_on() {
        //     return conductor->is_global_quantise_on();
        // }

        // void set_global_quantise_chord_on(bool v, bool requantise_immediately = true) {
        //     bool changed = v != conductor->is_global_quantise_chord_on();
        //     conductor->set_global_quantise_chord_on(v);
        //     if (changed && requantise_immediately) {
        //         //Serial_println(F("set_global_quantise_chord_on() about to call behaviour_manager_requantise_all_notes()"));
        //         behaviour_manager_requantise_all_notes();
        //     }
        // }
        // bool is_global_quantise_chord_on() {
        //     return conductor->is_global_quantise_chord_on();
        // }

        // getters & setters for global scale + global chord settings
        chord_identity_t get_global_chord() {
            return chord_identity_t {
                conductor->get_chord_type(),
                conductor->get_chord_degree(),
                conductor->get_chord_inversion()
            };
        }
        // TODO: should probably move this logic to the conductor
        // and have the conductor notify subscribers when chord or scale settings change
        // void set_global_chord(chord_identity_t chord, bool requantise_immediately = true) {
        //     bool changed = chord.diff(conductor->get_chord_identity());

        //     conductor->set_chord_identity(chord);
        //     if (changed && requantise_immediately) {
        //         //Serial_println(F("set_global_chord() about to call behaviour_manager_requantise_all_notes()"));
        //         behaviour_manager_requantise_all_notes();
        //     }
        // }
        void harmony_changed_notification(scale_identity_t new_scale, chord_identity_t new_chord, bool requantise_immediately = true) {
            if (!requantise_immediately)
                return;

            quantise_mode_t current_mode = conductor->get_global_quantise_mode();
            quantise_mode_t prior_mode = previous_global_quantise_mode;
            previous_global_quantise_mode = current_mode;
            const bool notify_behaviours = prior_mode != QUANTISE_MODE_NONE || current_mode != QUANTISE_MODE_NONE;

            // Connection policies own note quantisation, so held routed notes are re-pitched here.
            // Sources that regenerate their own notes are left for their behaviour to re-issue.
            migrate_connection_notes(-1, -1, notify_behaviours);

            if (notify_behaviours) {
                behaviour_manager_notify_harmony_changed();
            }
        }

        // int8_t get_global_scale_root() {
        //     return conductor->get_scale_root();
        // }
        // void set_global_scale_root(int8_t scale_root, bool requantise_immediately = true) {
        //     if (debug) Serial_printf("midi_mapper_matrix_manager#set_global_scale_root(%i) currently has global_scale_identity.root_note=%i\n", scale_root, conductor->get_scale_root());
        //     bool changed = scale_root != conductor->get_scale_root();
        //     if (changed && debug) {
        //         Serial_printf("======== midi_mapper_matrix_manager#set_global_scale_root() changing %s (%i) to %s (%i)\n", get_note_name_c(conductor->get_scale_root()), conductor->get_scale_root(), get_note_name_c(scale_root), scale_root);
        //         Serial_printf("Current scale:\t");
        //         print_scale(conductor->get_scale_root(), conductor->get_scale_type());
        //         Serial_printf("New scale:\t");
        //         print_scale(scale_root, conductor->get_scale_type());
        //     }
        //     conductor->set_scale_root(scale_root);
        //     if (changed && requantise_immediately) {
        //         if (debug) Serial_println(F("set_global_scale_root() about to call behaviour_manager_requantise_all_notes()"));
        //         behaviour_manager_requantise_all_notes();
        //         if (debug) Serial_printf("======= midi_mapper_matrix_manager#set_global_scale_root() done requantising all notes\n");
        //     }
        // }

        // scale_index_t get_global_scale_type() {
        //     return conductor->get_scale_type();
        // }
        // void set_global_scale_type(scale_index_t scale_type, bool requantise_immediately = true) {
        //     bool changed = scale_type != conductor->get_scale_type();
        //     conductor->set_scale_type(scale_type);
        //     if (changed && requantise_immediately) {
        //         //Serial_println(F("set_global_scale_type() about to call behaviour_manager_requantise_all_notes()"));
        //         behaviour_manager_requantise_all_notes();
        //     }
        // }

        // int8_t get_global_chord_degree() {
        //     return conductor->get_chord_degree();
        // }
        // void set_global_chord_degree(int8_t degree, bool requantise_immediately = true) {
        //     bool changed = degree != conductor->get_chord_degree();
        //     conductor->set_chord_degree(degree);
        //     if (changed && requantise_immediately) {
        //         Serial_println(F("set_global_chord_degree() about to call behaviour_manager_requantise_all_notes()"));
        //         behaviour_manager_requantise_all_notes();
        //     }
        // }

        // CHORD::Type get_global_chord_type() {
        //     return conductor->get_chord_type();
        // }
        // void set_global_chord_type(CHORD::Type chord_type, bool requantise_immediately = true) {
        //     bool changed = chord_type != conductor->get_chord_type();
        //     conductor->set_chord_type(chord_type);
        //     if (changed && requantise_immediately) {
        //         Serial_println(F("set_global_chord_type() about to call behaviour_manager_requantise_all_notes()"));
        //         behaviour_manager_requantise_all_notes();
        //     }
        // }

        // int8_t get_global_chord_inversion() {
        //     return conductor->get_chord_inversion();
        // }
        // void set_global_chord_inversion(int8_t inversion, bool requantise_immediately = true) {
        //     bool changed = inversion != conductor->get_chord_inversion();
        //     conductor->set_chord_inversion(inversion);
        //     if (changed && requantise_immediately) {
        //         Serial_println(F("set_global_chord_inversion() about to call behaviour_manager_requantise_all_notes()"));
        //         behaviour_manager_requantise_all_notes();
        //     }
        // }
    #endif

    // ---- saveloadlib settings ----
    //
    // One line per connection, with named policy fields for easy extension:
    //   midi_matrix~<src_handle>~<target_handle>~connection=version=2;channel=0;quantise=0;note_map=0;paused=0
    // Legacy "targets=tgt1;tgt2;..." lines are still accepted when loading.
    // Scale settings are registered with SL_SCOPE_PROJECT;
    // connections are emitted/parsed dynamically (sources are registered AFTER
    // sl_setup_all runs, so static SourceNode children cannot be used).
    // reset_matrix() must be called before loading routing (done in project.h).
    char routing_line_buf[SL_MAX_LINE];  // shared scratch buf (singleton, single-threaded)

    bool _load_connection_policy(const char* value, connection_policy_t& policy) {
        if (midi_parse_connection_policy(value, policy)) return true;
        Serial_printf(F("!! midi_matrix load routing: invalid connection policy '%s'\n"), value ? value : "(null)");
        return false;
    }

    // Internal helper: emit one connection and its policy per line.
    // Used by both save_recursive overloads.
    void _emit_routing_lines(const char* prefix, size_t prefix_len,
                             void (*cb_noct)(const char*),
                             void (*cb_ctx)(const char*, void*), void* ctx) {
        static char out[SL_MAX_LINE];
        for (source_id_t s = 0; s < (source_id_t)sources_count; s++) {
            connections.for_each_from(s, [&](const connection_t &connection) {
                const target_id_t t = connection.target;
                char policy[96];
                if (!midi_format_connection_policy(policy, sizeof(policy), connection.policy)) {
                    Serial_println(F("!! midi_matrix save routing: policy buffer too small"));
                    return;
                }
                int length;
                if (prefix_len == 0) {
                    length = snprintf(out, SL_MAX_LINE, "%s~%s~connection=%s",
                             sources[s].handle, targets[t].handle, policy);
                } else {
                    length = snprintf(out, SL_MAX_LINE, "%s~%s~%s~connection=%s",
                             prefix, sources[s].handle, targets[t].handle, policy);
                }
                if (length < 0 || static_cast<size_t>(length) >= SL_MAX_LINE) {
                    Serial_println(F("!! midi_matrix save routing: connection line too long"));
                    return;
                }
                if (cb_noct) cb_noct(out);
                if (cb_ctx)  cb_ctx(out, ctx);
            });
        }
    }

    // Override save_recursive to dynamically emit per-source target lists.
    // Sources are registered after sl_setup_all, so we can't use static children.
    virtual void save_recursive(char* prefix, size_t prefix_len,
                                void (*output_cb)(const char*),
                                sl_scope_t scope = SL_SCOPE_ALL) override {
        ISaveableSettingHost::save_recursive(prefix, prefix_len, output_cb, scope);
        if (scope & SL_SCOPE_ROUTING)
            _emit_routing_lines(prefix, prefix_len, output_cb, nullptr, nullptr);
    }
    virtual void save_recursive(char* prefix, size_t prefix_len,
                                void (*output_cb)(const char*, void*), void* ctx,
                                sl_scope_t scope = SL_SCOPE_ALL) override {
        ISaveableSettingHost::save_recursive(prefix, prefix_len, output_cb, ctx, scope);
        if (scope & SL_SCOPE_ROUTING)
            _emit_routing_lines(prefix, prefix_len, nullptr, output_cb, ctx);
    }

    // Make routing connections visible in showtree / sl_print_recursive.
    virtual void print_dynamic_entries(const char* prefix, SL_PrintCallback cb, void* ctx, sl_scope_t scope) override {
        if (scope & SL_SCOPE_ROUTING)
            _emit_routing_lines(prefix, strlen(prefix), nullptr, cb, ctx);
    }

    // Override load_line to handle dynamic connections without needing registered
    // SourceNode children (sources registered too late for that).
    virtual bool load_line(char** segments, int seg_count, const char* value,
                           sl_scope_t scope = SL_SCOPE_ALL) override {
        if (seg_count == 3 && strcmp(segments[2], "connection") == 0) {
            if (!(scope & SL_SCOPE_ROUTING)) return false;
            source_id_t src_id = get_source_id_for_handle(segments[0]);
            target_id_t tgt_id = get_target_id_for_handle(segments[1]);
            connection_policy_t policy;
            if (!_load_connection_policy(value, policy)) return true;
            if (connect(src_id, tgt_id)) {
                set_connection_policy(src_id, tgt_id, policy);
            } else {
                Serial_printf(F("midi_matrix load routing: failed connection '%s' -> '%s' (src_id=%i tgt_id=%i)\n"),
                              segments[0], segments[1], src_id, tgt_id);
            }
            return true;
        }
        if (seg_count == 2 && strcmp(segments[1], "targets") == 0) {
            if (!(scope & SL_SCOPE_ROUTING)) return false;
            char src_handle_buf[SL_MAX_LABEL];
            strncpy(src_handle_buf, segments[0], sizeof(src_handle_buf) - 1);
            src_handle_buf[sizeof(src_handle_buf) - 1] = '\0';
            sl_trim_inplace(src_handle_buf);
            const char* src_handle = src_handle_buf;
            if (!value || !*value) return true;  // empty targets line — valid
            strncpy(routing_line_buf, value, SL_MAX_LINE - 1);
            routing_line_buf[SL_MAX_LINE - 1] = '\0';
            char* p = routing_line_buf;
            int attempted = 0;
            int connected = 0;
            while (*p) {
                char* semi = strchr(p, ';');
                if (semi) *semi = '\0';
                sl_trim_inplace(p);
                if (*p) {
                    attempted++;
                    source_id_t src_id = get_source_id_for_handle(src_handle);
                    target_id_t tgt_id = get_target_id_for_handle(p);
                    if (connect(src_id, tgt_id)) {
                        connected++;
                    } else {
                        Serial_printf(F("midi_matrix load routing: failed connect '%s' -> '%s' (src_id=%i tgt_id=%i)\n"),
                                      src_handle, p,
                                      src_id,
                                      tgt_id);
                    }
                }
                if (!semi) break;
                p = semi + 1;
            }
            if (attempted > 0 && connected == 0) {
                Serial_printf(F("midi_matrix load routing: no targets restored for source '%s'\n"), src_handle);
            }
            return true;
        }
        // Single segment or unknown: try registered settings (scale etc)
        return ISaveableSettingHost::load_line(segments, seg_count, value, scope);
    }

    virtual void setup_saveable_settings() override {
        // now owned by conductor
        // #ifdef ENABLE_SCALES
        //     register_setting(new LSaveableSetting<scale_index_t>(
        //         "global_scale_type", "Scale", nullptr,
        //         [=](scale_index_t v) { this->set_global_scale_type(v); },
        //         [=]() -> scale_index_t { return this->get_global_scale_type(); }
        //     ), SL_SCOPE_PROJECT, false);
        //     register_setting(new LSaveableSetting<int8_t>(
        //         "global_scale_root", "Scale", nullptr,
        //         [=](int8_t v) { this->set_global_scale_root(v); },
        //         [=]() -> int8_t { return this->get_global_scale_root(); }
        //     ), SL_SCOPE_PROJECT, false);
        //     register_setting(new LSaveableSetting<bool>(
        //         "global_quantise_on", "Scale",
        //         &this->global_quantise_on
        //     ), SL_SCOPE_PROJECT, false);
        //     register_setting(new LSaveableSetting<bool>(
        //         "global_quantise_chord_on", "Scale",
        //         &this->global_quantise_chord_on
        //     ), SL_SCOPE_PROJECT, false);
        // #endif
    }

    private:
        // stuff for making singleton
        static MIDIMatrixManager* inst_;
        MIDIMatrixManager() {
            set_path_segment("midi_matrix");  // fixed segment; set here so register_child captures it correctly
            //setup_midi_output_wrapper_manager();
            sources = (source_entry*)CALLOC_FUNC(MAX_NUM_SOURCES, sizeof(source_entry));
            if (sources == nullptr) {
                Serial_println(F("!! MIDIMatrixManager: failed to allocate sources table"));
            }

            conductor->register_harmony_change_callback([this](scale_identity_t new_scale, chord_identity_t new_chord1) {
                this->harmony_changed_notification(new_scale, new_chord1);
            });
            #ifdef ENABLE_SCALES
                previous_global_quantise_mode = conductor->get_global_quantise_mode();
            #endif

            // #ifdef ENABLE_SCALES
            //     set_global_scale_identity_target(&this->global_scale_identity);
            //     set_global_chord_identity_target(&this->global_chord_identity);
            // #endif
        }
        MIDIMatrixManager(const MIDIMatrixManager&);
        MIDIMatrixManager& operator=(const MIDIMatrixManager&);
};

extern MIDIMatrixManager *midi_matrix_manager;
