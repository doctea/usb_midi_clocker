// CV clock outputs

#pragma once

#include <Arduino.h>
#include "Config.h"
#include "bpm.h"

#include "interfaces/interfaces.h"

#include "behaviours/behaviour_base.h"

#include "Drums.h"

#include "menuitems_lambda.h"

// todo: save/load mappings to patterns
// todo: use a more user-friendly and less memory-hungry way of presenting the UI

class VirtualBehaviour_SequencerGates : public DeviceBehaviourUltimateBase {
  GateManager *gate_manager;
  int bank = -1;

  bool sequencer_enabled = true;

  public:
    VirtualBehaviour_SequencerGates(GateManager *gate_manager, int bank = BANK_SEQ) : DeviceBehaviourUltimateBase () {
      initialise_note_to_gate_map();

      this->gate_manager = gate_manager;
      this->bank = bank;
    }

    const char *get_label() override {
        return "Gate Sequencer";
    }

    virtual int getType() override {
      return BehaviourType::virt;
    }

    //virtual bool receives_midi_notes()  { return false; }
    virtual bool transmits_midi_notes() override { return true; }
    virtual bool transmits_midi_clock() override { return true; }

    virtual void on_pre_clock(uint32_t ticks) override {
      this->process_sequencer(ticks);
    };

    virtual LinkedList<MenuItem*> *make_menu_items() override {
        LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();

        menuitems->add(new LambdaToggleControl(
            "Enable sequencer",
            [=](bool v) -> void { 
                this->sequencer_enabled = v; 
                this->clear_all_outputs();
            },
            [=](void) -> bool { return this->sequencer_enabled; }
        ));

        // todo: use a more user-friendly and less memory-hungry way of doing this
        for (int i = 0 ; i < MIDI_MAX_NOTE ; i++) {
            //for (int i = GM_NOTE_MINIMUM ; i < GM_NOTE_MAXIMUM ; i++) {
            char label[MENU_C_MAX] = "";
            snprintf(label, MENU_C_MAX, "Gate for %3s: ", get_note_name_c(i, GM_CHANNEL_DRUMS));
            LambdaNumberControl<int8_t> *s = new LambdaNumberControl<int8_t>(
                label, 
                [this,i] (int8_t v) -> void { map_note_to_gate[i] = v; },
                [this,i] (void) -> int8_t { return map_note_to_gate[i]; },
                nullptr,
                (int8_t)-1, 
                (int8_t)NUM_SEQUENCES, 
                true,
                true
            );
            s->horiz_label = s->show_header = true;
            menuitems->add(s);
        }

        return menuitems;
    }

    int8_t *map_note_to_gate = nullptr;
    /*int8_t map_note_to_gate[MIDI_MAX_NOTE+1] = {
        NOTE_OFF,  // 0
        NOTE_OFF,  // 1
        NOTE_OFF,  // 2
        NOTE_OFF,  // 3
        NOTE_OFF,  // 4
        NOTE_OFF,  // 5
        NOTE_OFF,  // 6
        NOTE_OFF,  // 7
        NOTE_OFF,  // 8
        NOTE_OFF,  // 9       NOTE_OFF,
        NOTE_OFF,  // 10
        NOTE_OFF,  // 11
        NOTE_OFF,  // 12
        NOTE_OFF,  // 13      NOTE_OFF,
        NOTE_OFF,  // 14
        NOTE_OFF,  // 15
        NOTE_OFF,  // 16
        NOTE_OFF,  // 17        NOTE_OFF,
        NOTE_OFF,  // 18
        NOTE_OFF,  // 19
        NOTE_OFF,  // 20
        NOTE_OFF,  // 21       NOTE_OFF,
        NOTE_OFF,  // 22
        NOTE_OFF,  // 23
        NOTE_OFF,  // 24
        NOTE_OFF,  // 25        NOTE_OFF,
        NOTE_OFF,  // 26
        NOTE_OFF,  // 27
        NOTE_OFF,  // 28
        NOTE_OFF,  // 29
        NOTE_OFF,  // 30
        NOTE_OFF,  // 31
        NOTE_OFF,  // 32
        NOTE_OFF,  // 33
        NOTE_OFF,  // 34
        0,         //NOTE_ACOUSTIC_BASS_DRUM 35
        0,         //NOTE_ELECTRIC_BASS_DRUM 36
        NOTE_OFF,  //NOTE_SIDE_STICK 37
        1,         //NOTE_ACOUSTIC_SNARE 38
        2,         //NOTE_HAND_CLAP 39
        1,         //NOTE_ELECTRIC_SNARE 40
        NOTE_OFF,  //NOTE_LOW_FLOOR_TOM 41
        4,         //NOTE_CLOSED_HI_HAT 42
        NOTE_OFF,  //NOTE_HIGH_FLOOR_TOM 43
        6,         //NOTE_PEDAL_HI_HAT 44
        NOTE_OFF,  //NOTE_LOW_TOM 45
        5,         //NOTE_OPEN_HI_HAT 46
        NOTE_OFF,  //NOTE_LOW_MID_TOM 47
        NOTE_OFF,  //NOTE_HI_MID_TOM 48
        7,         //NOTE_CRASH_CYMBAL_1 49
        NOTE_OFF,  //NOTE_HIGH_TOM 50
        NOTE_OFF,  //NOTE_RIDE_CYMBAL_1 51
        NOTE_OFF,  //NOTE_CHINESE_CYMBAL 52
        NOTE_OFF,  //NOTE_RIDE_BELL 53
        NOTE_OFF,  //NOTE_TAMBOURINE 54
        NOTE_OFF,  //NOTE_SPLASH_CYMBAL 55
        3,         //NOTE_COWBELL 56
        NOTE_OFF,  //NOTE_CRASH_CYMBAL_2 57
        NOTE_OFF,  //NOTE_VIBRA_SLAP 58
        NOTE_OFF,  //NOTE_RIDE_CYMBAL_2 59
        NOTE_OFF,  //NOTE_HIGH_BONGO 60
        NOTE_OFF,  //NOTE_LOW_BONGO 61
        NOTE_OFF,  //NOTE_MUTE_HIGH_CONGA 62
        NOTE_OFF,  //NOTE_OPEN_HIGH_CONGA 63
        NOTE_OFF,  //NOTE_LOW_CONGA 64
        NOTE_OFF,  //NOTE_HIGH_TIMBALE 65
        NOTE_OFF,  //NOTE_LOW_TIMBALE 66
        NOTE_OFF,  //NOTE_HIGH_AGOGO 67
        NOTE_OFF,  //NOTE_LOW_AGOGO 68
        NOTE_OFF,  //NOTE_CABASA 69
        NOTE_OFF,  //NOTE_MARACAS 70
        NOTE_OFF,  //NOTE_SHORT_WHISTLE 71
        NOTE_OFF,  //NOTE_LONG_WHISTLE 72
        NOTE_OFF,  //NOTE_SHORT_GUIRO 73
        NOTE_OFF,  //NOTE_LONG_GUIRO 74
        NOTE_OFF,  //NOTE_CLAVES 75
        NOTE_OFF,  //NOTE_HIGH_WOODBLOCK 76
        NOTE_OFF,  //NOTE_LOW_WOODBLOCK 77
        NOTE_OFF,  //NOTE_MUTE_CUICA 78
        NOTE_OFF,  //NOTE_OPEN_CUICA 79
        NOTE_OFF,  //NOTE_MUTE_TRIANGLE 80
        NOTE_OFF,  //NOTE_OPEN_TRIANGLE 81
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,  
        NOTE_OFF,        
    };*/

    void initialise_note_to_gate_map() {
        this->map_note_to_gate = (int8_t*)calloc(sizeof(int8_t), MIDI_MAX_NOTE+1);
        for(int8_t i = 0 ; i < MIDI_MAX_NOTE ; i++) {
            this->map_note_to_gate[i] = get_default_gate_number_for_note(i);
        }
    }

    int8_t get_default_gate_number_for_note(int8_t note) {
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
        return -1;
    }

    // done: pull this from a configurable array
    // done: with a way to display what the mapping should be
    // done:   name drums - so make get_note_name return a three-letter abbreviation
    virtual uint8_t midi_drum_note_to_gate_number(uint8_t note) {
        if (is_valid_note(note))
            return map_note_to_gate[note];
        return -1;
        /*switch(note) {
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
        return note;*/
    }

    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
        // todo: turns out that if the duration of a received note is very short -- ie 1 tick or less so the note on and off get received in the same tick -- then it can be effectively ignored, leading to missed hits...
        //          work around this by using minimum duration of 2 ticks; maybe can implement a more robust fix in gate interface code by latching to ensure that 'GATE ONs' always last at least 1 tick before going off..?
        // translate MIDI drum notes to gates
        if (channel==GM_CHANNEL_DRUMS) {
            int gate = midi_drum_note_to_gate_number(note);
            if (gate >= 0 && gate < NUM_SEQUENCES) {
                //Serial_printf("At tick %5i, got NoteOn  for %s,\t%i,\t%iGate %i\n", ticks, get_note_name_c(note,channel), velocity, channel, gate);
                cv_out_sequence_pin_on(gate);
            }
        }
    }
    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        // translate MIDI drum notes to gates
        if (channel==GM_CHANNEL_DRUMS) {
            int gate = midi_drum_note_to_gate_number(note);
            if (gate >= 0 && gate < NUM_SEQUENCES) {
                //Serial_printf("At tick %5i, got NoteOff for %s,\t%i,\t%iGate %i\n", ticks, get_note_name_c(note,channel), velocity, channel, gate);
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

    void process_sequencer(unsigned long ticks);

    void clear_all_outputs() {
        for (int i = 0 ; i < NUM_SEQUENCES ; i++) {
            cv_out_sequence_pin_off(i);
        }
    }

    void cv_out_sequence_pin_off(byte i) {
        if (i >= 0 && i < NUM_SEQUENCES)
            gate_manager->send_gate(this->bank, i, LOW);
    }
    void cv_out_sequence_pin_on(byte i) {
        if (i >= 0 && i < NUM_SEQUENCES)
            gate_manager->send_gate(this->bank, i, HIGH);
    }
};

extern VirtualBehaviour_SequencerGates *behaviour_sequencer_gates;