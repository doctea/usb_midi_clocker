#pragma once

#include <stdint.h>

#include "midi/midi_mapper_matrix_types.h"

#ifndef MIDI_CONNECTION_NOTE_CAPACITY
#define MIDI_CONNECTION_NOTE_CAPACITY 128
#endif

// Runtime ledger for matrix-routed notes. Each entry remembers the exact
// connection-level pitch/channel produced by one logical Note On, so Note Off
// never has to re-run a transform against potentially changed harmony.
class MIDIConnectionNoteState {
public:
    struct entry_t {
        int8_t source_id = -1;
        int8_t target_id = -1;
        int8_t input_pitch = -1;
        uint8_t input_channel = 0;
        int8_t output_pitch = -1;
        uint8_t output_channel = 0;
        uint8_t velocity = 0;
        bool emitted = false;
        bool occupied = false;
    };

    entry_t entries[MIDI_CONNECTION_NOTE_CAPACITY] = {};

    entry_t *record(source_id_t source_id, target_id_t target_id,
                    int8_t input_pitch, uint8_t input_channel,
                    int8_t output_pitch, uint8_t output_channel,
                    uint8_t velocity, bool emitted) {
        for (uint16_t index = 0; index < MIDI_CONNECTION_NOTE_CAPACITY; ++index) {
            entry_t &entry = entries[index];
            if (entry.occupied)
                continue;

            entry.source_id = (int8_t)source_id;
            entry.target_id = (int8_t)target_id;
            entry.input_pitch = input_pitch;
            entry.input_channel = input_channel;
            entry.output_pitch = output_pitch;
            entry.output_channel = output_channel;
            entry.velocity = velocity;
            entry.emitted = emitted;
            entry.occupied = true;
            ++active_count;
            return &entry;
        }
        return nullptr;
    }

    bool take(source_id_t source_id, target_id_t target_id,
              int8_t input_pitch, uint8_t input_channel, entry_t &result) {
        for (uint16_t index = 0; index < MIDI_CONNECTION_NOTE_CAPACITY; ++index) {
            entry_t &entry = entries[index];
            if (!entry.occupied
                || entry.source_id != source_id
                || entry.target_id != target_id
                || entry.input_pitch != input_pitch
                || entry.input_channel != input_channel) {
                continue;
            }

            result = entry;
            erase(entry);
            return true;
        }
        return false;
    }

    void erase(entry_t &entry) {
        if (!entry.occupied)
            return;
        entry = entry_t{};
        if (active_count > 0)
            --active_count;
    }

    uint16_t count() const {
        return active_count;
    }

    void clear() {
        for (uint16_t index = 0; index < MIDI_CONNECTION_NOTE_CAPACITY; ++index)
            entries[index] = entry_t{};
        active_count = 0;
    }


private:
    uint16_t active_count = 0;
};
