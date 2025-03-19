#pragma once

#include "behaviour_midibass.h"

// todo:
// - "input 1" for receiving notes that will make up the current arp
// - "input 2" that will receive events to trigger the next note of the arp

class VirtualBehaviour_Arpeggiator : virtual public VirtualBehaviourBase {
    public:

    target_id_t target_id_trigger = -1;

    //bool transmits_midi_notes() override { return true; };
    bool receives_midi_notes()  override { return true; };

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
    /*virtual void actualSendControlChange(uint8_t cc, uint8_t value, uint8_t channel) override {
        this->receive_control_change(channel, cc, value);    
    }*/

    /*virtual void actualSendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
        if (this->debug) Serial.printf("%s#actualSendNoteOn(%i, %i, %i) (calling receive_note_on!)\n", this->get_label(), note, velocity, channel);
        this->receive_note_on(channel, note, velocity);
    }
    virtual void actualSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        if (this->debug) Serial.printf("%s#actualSendNoteOff(%i, %i, %i)  (calling receive_note_off!)\n", this->get_label(), note, velocity, channel);
        this->receive_note_off(channel, note, velocity);
    }
    virtual void actualSendControlChange(uint8_t cc, uint8_t value, uint8_t channel) override {
        this->receive_control_change(channel, cc, value);    
    }*/

    int current_index = 0;
    int8_t current_note = NOTE_OFF;

    void trigger_next_note_on() {
        if (is_valid_note(current_note)) {
            //this->sendNoteOff(current_note, 0, 1);
            trigger_note_off();
        }
        int8_t note = note_tracker.get_held_note_index(current_index);
        if (is_valid_note(note)) {
            current_index++;
            //if (debug) 
            Serial.printf("Arpeggiator#on_tick: note is valid - playing %i (%s)\n", note, get_note_name_c(note));
            //this->sendNoteOn(note, MIDI_MAX_VELOCITY, 1);
            midi_matrix_manager->processNoteOn(this->source_id, note, MIDI_MAX_VELOCITY, 0);
            current_note = note;
        }
    }

    void trigger_note_off() {
        if (is_valid_note(current_note)) {
            //this->sendNoteOff(current_note, 0, 1);
            midi_matrix_manager->processNoteOff(this->source_id, current_note, MIDI_MAX_VELOCITY, 0);
            current_note = NOTE_OFF;
        }
    }
   
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
        //this->receive_note_on(channel, note, velocity);
        if (channel==1) {
            VirtualBehaviourBase::sendNoteOn(note, velocity, channel);
        } else if (channel==2) {
            //Serial.printf("Arpeggiator#sendNoteOn: channel 2, so triggering next note\n");
            this->trigger_next_note_on();
        } else {
            //Serial.printf("Arpeggiator#sendNoteOff: channel %i not recognised\n", channel);
        }
    }
    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        if (this->debug) Serial.printf("%s#sendNoteOff(%i, %i, %i)\n", this->get_label(), note, velocity, channel);
        //this->receive_note_off(channel, note, velocity);
        if (channel==1) {
            VirtualBehaviourBase::sendNoteOff(note, velocity, channel);
        } else if (channel==2) {
            //Serial.printf("Arpeggiator#sendNoteOn: channel 2, so stopping note\n");
            this->trigger_note_off();
        } else {
            //Serial.printf("Arpeggiator#sendNoteOff: channel %i not recognised\n", channel);
        }
    }

};

extern VirtualBehaviour_Arpeggiator *behaviour_arpeggiator;