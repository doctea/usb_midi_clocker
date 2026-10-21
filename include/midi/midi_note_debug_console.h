#pragma once

#include <stdlib.h>
#include <ctype.h>
#include "midi/midi_mapper_matrix_manager.h"

inline const char *midi_note_debug_source(MIDIMatrixManager &matrix, int id) {
    const char *label = matrix.get_label_for_source_id(id);
    return label != nullptr ? label : "-";
}

inline const char *midi_note_debug_target(MIDIMatrixManager &matrix, int id) {
    return id >= 0 && id < matrix.targets_count ? matrix.targets[id].handle : "-";
}

inline void midi_note_debug_event(MIDIMatrixManager &matrix, const MIDINoteTraceEvent &event) {
    Serial_printf("t=%lu %s src=%i[%s] tgt=%i[%s] in=%i/ch%u out=%i/ch%u v=%u emitted=%u started=%lu refs=%i policy=c%u/q%u/map%u/p%u global_q=%u\n",
        (unsigned long)event.time_ms, midi_note_trace_kind_name(event.kind),
        event.source, midi_note_debug_source(matrix, event.source),
        event.target, midi_note_debug_target(matrix, event.target),
        event.input_pitch, event.input_channel, event.output_pitch, event.output_channel,
        event.velocity, event.emitted, (unsigned long)event.started_ms, event.refcount,
        event.fixed_channel, event.quantise, event.note_map, event.paused, event.global_quantise);
}

inline void midi_note_debug_status(MIDIMatrixManager &matrix) {
    Serial_printf("MIDI notes: held=%u/%u trace=%s events=%u/%u overwritten=%lu filters=src%i/tgt%i history_bytes=%u event_bytes=%u ledger_bytes=%u\n",
        matrix.active_connection_notes.count(), MIDI_CONNECTION_NOTE_CAPACITY,
        matrix.note_trace && matrix.note_trace->enabled ? "on" : "off",
        matrix.note_trace ? matrix.note_trace->count() : 0, MIDINoteTrace::CAPACITY,
        (unsigned long)(matrix.note_trace ? matrix.note_trace->overwritten : 0),
        matrix.note_trace ? matrix.note_trace->source_filter : -1,
        matrix.note_trace ? matrix.note_trace->target_filter : -1,
        (unsigned int)(matrix.note_trace ? sizeof(MIDINoteTrace) : 0),
        (unsigned int)sizeof(MIDINoteTraceEvent), (unsigned int)sizeof(MIDIConnectionNoteState));
}

inline bool midi_note_debug_matches(MIDIMatrixManager &matrix, int source, int target,
        int source_filter, int target_filter, MIDIEndpointOwner *owner) {
    return (source_filter < 0 || source == source_filter)
        && (target_filter < 0 || target == target_filter)
        && (owner == nullptr || matrix.get_source_owner(source) == owner || matrix.get_target_owner(target) == owner);
}

inline void midi_note_debug_held(MIDIMatrixManager &matrix, int source_filter = -1,
        int target_filter = -1, MIDIEndpointOwner *owner = nullptr) {
    const uint32_t now = millis();
    midi_note_debug_status(matrix);
    Serial_printf("Snapshot t=%lu: connection outputs, not proof of physical delivery; age is not a stuck-note verdict\n", (unsigned long)now);
    unsigned int shown = 0;
    for (uint16_t index = 0; index < MIDI_CONNECTION_NOTE_CAPACITY; ++index) {
        MIDIConnectionNoteState::entry_t entry;
        MIDI_NOTE_DIAGNOSTIC_ATOMIC { entry = matrix.active_connection_notes.entries[index]; }
        if (!entry.occupied || !midi_note_debug_matches(matrix, entry.source_id, entry.target_id, source_filter, target_filter, owner))
            continue;
        ++shown;
        midi_note_debug_event(matrix, matrix.make_note_trace_event(MIDINoteTraceKind::HELD,
            entry.source_id, entry.target_id, entry.input_pitch, entry.input_channel,
            entry.output_pitch, entry.output_channel, entry.velocity, entry.emitted, entry.started_ms));
        Serial_printf("  held_age_ms=%lu\n", (unsigned long)(uint32_t)(now - entry.started_ms));
    }
    Serial_printf("Shown held entries=%u\n", shown);
    for (int source = 0; source < matrix.sources_count; ++source) {
        matrix.for_each_connection_from(source, [&](const MIDIMatrixManager::connection_t &connection) {
            if (!midi_note_debug_matches(matrix, source, connection.target, source_filter, target_filter, owner)) return;
            Serial_printf("route src=%i[%s] role=%s -> tgt=%u[%s] role=%s c=%u q=%u map=%u paused=%u held=%u\n",
                source, midi_note_debug_source(matrix, source), matrix.get_role_for_source_id(source),
                connection.target, midi_note_debug_target(matrix, connection.target),
                matrix.get_role_for_target_id(connection.target), connection.policy.fixed_channel,
                (unsigned int)connection.policy.quantise_mode, (unsigned int)connection.policy.note_map_mode,
                connection.policy.paused, matrix.held_notes_for_connection(source, connection.target));
        });
    }
    // Wrapper counters are target-wide, so include other sources explicitly.
    for (int target = 0; target < matrix.targets_count; ++target) {
        if (target_filter >= 0 && target != target_filter) continue;
        bool relevant = owner == nullptr && source_filter < 0;
        relevant = relevant || (owner != nullptr && matrix.get_target_owner(target) == owner);
        for (const auto &entry : matrix.active_connection_notes.entries)
            if (entry.occupied && entry.target_id == target
                && midi_note_debug_matches(matrix, entry.source_id, target, source_filter, target_filter, owner))
                relevant = true;
        for (int source = 0; source < matrix.sources_count && !relevant; ++source)
            if (matrix.is_connected(source, target)
                && midi_note_debug_matches(matrix, source, target, source_filter, target_filter, owner))
                relevant = true;
        if (!relevant || matrix.targets[target].wrapper == nullptr) continue;
        auto *wrapper = matrix.targets[target].wrapper;
        const auto state = wrapper->get_note_debug_state();
        Serial_printf("target=%i[%s] default_ch=%i target_wide_refs (all sources; pitch-only counters) behaviour=%u tuning=%i limits=%i..%i modes=%u/%u\n",
            target, midi_note_debug_target(matrix, target), wrapper->default_channel,
            state.behaviour, state.tuning, state.low, state.high, state.low_mode, state.high_mode);
        for (int pitch = 0; pitch < 128; ++pitch)
            if (wrapper->playing_notes[pitch] != 0)
                Serial_printf("  pitch=%i refs=%i\n", pitch, wrapper->playing_notes[pitch]);
    }
}

inline bool midi_note_debug_filters(const char *args, MIDIMatrixManager &matrix, int &source, int &target) {
    source = target = -1;
    int *outputs[] = {&source, &target};
    const int limits[] = {matrix.sources_count, matrix.targets_count};
    const char *cursor = args;
    for (int index = 0; index < 2; ++index) {
        while (isspace(static_cast<unsigned char>(*cursor))) ++cursor;
        if (!*cursor) return true;
        char *end = nullptr;
        const long parsed = strtol(cursor, &end, 10);
        if (end == cursor || (*end && !isspace(static_cast<unsigned char>(*end)))
            || parsed < -1 || parsed >= limits[index]) return false;
        *outputs[index] = static_cast<int>(parsed);
        cursor = end;
    }
    while (isspace(static_cast<unsigned char>(*cursor))) ++cursor;
    return !*cursor;
}

inline void execute_midi_note_debug_command(MIDIMatrixManager *matrix, const char *action, const char *args) {
    if (matrix == nullptr) {
        Serial_println(F("!! MIDI matrix unavailable"));
        return;
    }
    if (strcmp(action, "on") == 0 || strcmp(action, "held") == 0) {
        int source, target;
        if (!midi_note_debug_filters(args, *matrix, source, target)) {
            Serial_println(F("Usage: notes on|held [source_id|-1] [target_id|-1]"));
            return;
        }
        if (strcmp(action, "on") == 0) {
            if (matrix->enable_note_trace(source, target)) midi_note_debug_status(*matrix);
        } else midi_note_debug_held(*matrix, source, target);
    } else if (strcmp(action, "owner") == 0) {
        MIDIEndpointOwner *owner = nullptr;
        bool ambiguous = false;
        auto consider = [&](MIDIEndpointOwner *candidate) {
            if (candidate == nullptr || strcmp(candidate->get_endpoint_owner_label(), args) != 0) return;
            if (owner != nullptr && owner != candidate) ambiguous = true;
            owner = candidate;
        };
        for (int source = 0; source < matrix->sources_count; ++source) consider(matrix->get_source_owner(source));
        for (int target = 0; target < matrix->targets_count; ++target) consider(matrix->get_target_owner(target));
        if (owner == nullptr || ambiguous) {
            Serial_println(F("!! notes owner: unknown or ambiguous label; use notes list and numeric filters"));
            return;
        }
        midi_note_debug_held(*matrix, -1, -1, owner);
    } else if (*args) {
        Serial_println(F("!! notes: unexpected arguments"));
    } else if (strcmp(action, "list") == 0) {
        for (int source = 0; source < matrix->sources_count; ++source) {
            auto *owner = matrix->get_source_owner(source);
            Serial_printf("src=%i [%s] role=%s owner=%s\n", source, midi_note_debug_source(*matrix, source),
                matrix->get_role_for_source_id(source), owner ? owner->get_endpoint_owner_label() : "-");
        }
        for (int target = 0; target < matrix->targets_count; ++target) {
            auto *owner = matrix->get_target_owner(target);
            Serial_printf("tgt=%i [%s] role=%s owner=%s\n", target, midi_note_debug_target(*matrix, target),
                matrix->get_role_for_target_id(target), owner ? owner->get_endpoint_owner_label() : "-");
        }
    } else if (strcmp(action, "off") == 0) {
        if (matrix->note_trace) matrix->note_trace->enabled = false;
        midi_note_debug_status(*matrix);
    } else if (strcmp(action, "clear") == 0) {
        if (matrix->note_trace) matrix->note_trace->clear();
        midi_note_debug_status(*matrix);
    } else if (strcmp(action, "dump") == 0) {
        if (matrix->note_trace) matrix->note_trace->enabled = false;
        midi_note_debug_status(*matrix);
        Serial_println(F("Trace frozen. SEND means wrapper handoff attempt, not physical receipt; refs=-1 means bypassed/unknown."));
        if (matrix->note_trace)
            for (uint16_t index = 0; index < matrix->note_trace->count(); ++index)
                midi_note_debug_event(*matrix, matrix->note_trace->at(index));
        midi_note_debug_held(*matrix);
    } else if (!*action || strcmp(action, "status") == 0) {
        midi_note_debug_status(*matrix);
    } else {
        Serial_println(F("Usage: notes list|status|on [src|-1] [tgt|-1]|off|clear|dump|held [src|-1] [tgt|-1]|owner <label>"));
    }
}
