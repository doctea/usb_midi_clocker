// CV clock outputs

#pragma once

#include <Arduino.h>
#include "Config.h"
#include "bpm.h"

#include "interfaces/interfaces.h"

#include "behaviours/behaviour_base.h"

#include "Drums.h"

class VirtualBehaviour_SequencerGates : public DeviceBehaviourUltimateBase {
  GateManager *gate_manager;
  int bank = -1;

  public:
    VirtualBehaviour_SequencerGates(GateManager *gate_manager, int bank = BANK_SEQ) : DeviceBehaviourUltimateBase () {
      this->gate_manager = gate_manager;
      this->bank = bank;
    }

    virtual int getType() override {
      return BehaviourType::virt;
    }

    //virtual bool receives_midi_notes()  { return false; }
    virtual bool transmits_midi_notes() override { return true; }
    virtual bool transmits_midi_clock() override { return true; }

    virtual void on_pre_clock(uint32_t ticks) override {
      this->update_cv_outs(ticks);
    };

    // todo: pull this from a configurable array
    // todo: with a way to display what the mapping should be
    // tood:   name drums - so make get_note_name return a three-letter abbreviation
    virtual uint8_t midi_drum_note_to_gate_number(uint8_t note) {
        switch(note) {
            case GM_NOTE_ACOUSTIC_BASS_DRUM:
            case GM_NOTE_ELECTRIC_BASS_DRUM:
                return 0;
            case GM_NOTE_ACOUSTIC_SNARE:
            case GM_NOTE_ELECTRIC_SNARE:
                return 1;
            case GM_NOTE_HAND_CLAP:
                return 2;
            case GM_NOTE_COWBELL:
                return 3;
            case GM_NOTE_CLOSED_HI_HAT:
                return 4;
            case GM_NOTE_OPEN_HI_HAT:
                return 5;
            case GM_NOTE_PEDAL_HI_HAT:
                return 6;
            case GM_NOTE_CRASH_CYMBAL_1:
                return 7;            
        }
        return note;
    }

    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
        // todo: turns out that if the duration of a received note is very short -- ie 1 tick or less so the note on and off get received in the same tick -- then it can be effectively ignored, leading to missed hits...
        //          work around this by using minimum duration of 2 ticks; maybe can implement a more robust fix in gate interface code by latching to ensure that 'GATE ONs' always last at least 1 tick before going off..?
        // translate MIDI drum notes to gates
        if (channel==GM_CHANNEL_DRUMS) {
            int gate = midi_drum_note_to_gate_number(note);
            if (gate < NUM_SEQUENCES) {
                //Serial_printf("At tick %5i, got NoteOn  for %s,\t%i,\t%i\t\tGate %i\n", ticks, get_note_name_c(note,channel), velocity, channel, gate);
                cv_out_sequence_pin_on(gate);
            }
        }
    }
    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        // translate MIDI drum notes to gates
        if (channel==GM_CHANNEL_DRUMS) {
            int gate = midi_drum_note_to_gate_number(note);
            if (gate < NUM_SEQUENCES) {
                //Serial_printf("At tick %5i, got NoteOff for %s,\t%i,\t%i\t\tGate %i\n", ticks, get_note_name_c(note,channel), velocity, channel, gate);
                cv_out_sequence_pin_off(gate);
            }
        }
    }

    // internal sequencer-y stuff

    #define NUM_STEPS     8
    #define SEQUENCER_MAX_VALUE 6

    int duration = PPQN/8;

    void sequencer_clear_pattern();
    void sequencer_clear_row(byte row);

    void init_sequence();
    byte read_sequence(byte row, byte col);
    void write_sequence(byte row, byte col, byte value);
    void sequencer_press(byte row, byte col, bool shift = false);
    bool should_trigger_sequence(unsigned long ticks, byte sequence, int offset = 0);

    void raw_write_pin(int,int);

    void update_cv_outs(unsigned long ticks);

    void cv_out_sequence_pin_off(byte i) {
      gate_manager->send_gate(this->bank, i, LOW);
    }
    void cv_out_sequence_pin_on(byte i) {
      gate_manager->send_gate(this->bank, i, HIGH);
    }
};

extern VirtualBehaviour_SequencerGates *behaviour_sequencer_gates;