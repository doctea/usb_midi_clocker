#pragma once

// #include "behaviour_midibass.h"

// todo:
//  more features, like clock-based triggering and arpeggiation patterns
//  maybe unify the arp generation with flexiarp's logic?

#if defined(ENABLE_SCALES) && defined(ENABLE_SCREEN)
    #include "mymenu/menuitems_notedisplay.h"
#endif

class VirtualBehaviour_Arpeggiator : virtual public VirtualBehaviourBase {
    public:

    const uint8_t MIDI_CHANNEL_CHORDS = 1;
    const uint8_t MIDI_CHANNEL_TRIGGER = 2;

    target_id_t target_id_trigger = -1;

    bool transmits_midi_notes() override { return true; };
    bool receives_midi_notes()  override { return true; };
    bool supports_note_limits() override { return true; };

    VirtualBehaviour_Arpeggiator() : DeviceBehaviourUltimateBase() {
        //this->debug = true;
    }

    virtual const char *get_label() override {
        return "Arpeggiator";
    }

    virtual void actualSendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
        //if (this->debug) 
        //Serial.printf("%s#actualSendNoteOn(%i, %i, %i) (calling receive_note_on!)\n", this->get_label(), note, velocity, channel);
        //this->receive_note_on(channel, note, velocity);
    }
    virtual void actualSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        //if (this->debug) 
        //Serial.printf("%s#actualSendNoteOff(%i, %i, %i)  (calling receive_note_off!)\n", this->get_label(), note, velocity, channel);
        //this->receive_note_off(channel, note, velocity);
    }

    int current_index = 0;
    int8_t current_note = NOTE_OFF;

    void trigger_next_note_on() {
        if (is_valid_note(current_note)) {
            trigger_note_off();
        }
        int8_t note = note_tracker.get_held_note_index(current_index);
        routed_note_output_t r = this->resolve_routed_note_output(note, 0);
        if (r.emitted) {
            current_index++;
            //if (debug) Serial.printf("Arpeggiator#on_tick: note is valid - playing %i (%s)\n", r.pitch, get_note_name_c(r.pitch));
            midi_matrix_manager->processNoteOn(this->source_id, r.pitch, MIDI_MAX_VELOCITY, 0);
            current_note = r.pitch;
        }
    }

    void trigger_note_off() {
        if (is_valid_note(current_note)) {
            midi_matrix_manager->processNoteOff(this->source_id, current_note, MIDI_MAX_VELOCITY, 0);
            current_note = NOTE_OFF;
        }
    }
   
    // @@TODO: re-enable triggering based on clock instead of trigger input;
    // can probably use re-use the same timing info as used in the CVInput triggerer
    // maybe we could even use a shuffle pattern for the arpeggiation too?
    /*virtual void on_tick(uint32_t ticks) {
        if (is_bpm_on_beat(ticks)) {
            if (debug) Serial.println("Arpeggiator#on_tick: on beat");
            // do the arpeggiation
            trigger_next_note_on();
        } else if (is_bpm_on_beat(ticks, PPQN/2)) {
            if (debug) Serial.println("Arpeggiator#on_tick: on off-beat - stopping note");
            trigger_note_off();
        }
    }*/

    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
        if (this->debug) Serial.printf("%s#sendNoteOn(%i, %i, %i)\n", this->get_label(), note, velocity, channel);

        if (channel==MIDI_CHANNEL_CHORDS) {
            if (debug) Serial.printf("Arpeggiator#sendNoteOn: channel 1: holding chord note\n");
            note_tracker.held_note_on(note, note);
        } else if (channel==MIDI_CHANNEL_TRIGGER) {
            if (debug) Serial.printf("Arpeggiator#sendNoteOn: channel 2, so triggering next note\n");
            this->trigger_next_note_on();
        } else {
            if (debug) Serial.printf("Arpeggiator#sendNoteOn: channel %i not recognised\n", channel);
        }
    }
    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        if (debug) Serial.printf("%s#sendNoteOff(%i, %i, %i)\n", this->get_label(), note, velocity, channel);

        if (channel==MIDI_CHANNEL_CHORDS) {
            if (debug) Serial.printf("Arpeggiator#sendNoteOff: channel 1, so releasing chord note\n");
            note_tracker.held_note_off(note);
        } else if (channel==MIDI_CHANNEL_TRIGGER) {
            if (debug) Serial.printf("Arpeggiator#sendNoteOff: channel 2, so stopping note\n");
            this->trigger_note_off();
        } else {
            if (debug) Serial.printf("Arpeggiator#sendNoteOff: channel %i not recognised\n", channel);
        }
    }

    #ifdef ENABLE_SCREEN
        virtual MenuItemList *make_menu_items() override {
            MenuItemList *menuitems = DeviceBehaviourUltimateBase::make_menu_items();

            menuitems->add(new NoteDisplay("Notes", &this->note_tracker));
            menuitems->add(new NoteHarmonyDisplay(
                (const char*)"Harmony", 
                &conductor->global_scale_identity.scale_number, 
                &conductor->global_scale_identity.root_note, 
                &this->note_tracker,
                &conductor->global_quantise_mode
            ));
            menuitems->add(new CallbackMenuItem(
                (const char*)"Output note",
                [=](void) -> const char * {
                    return get_note_name_c(this->current_note);
                }
                // [=]() -> bool { return this->debug; }
            ));

            return menuitems;
        }
    #endif

};

extern VirtualBehaviour_Arpeggiator *behaviour_arpeggiator;