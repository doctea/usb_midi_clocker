#pragma once

#include "behaviour_midibass.h"

class MIDIBassBehaviourProxy : virtual public MIDIBassBehaviour {
    public:

    //bool transmits_midi_notes() override { return true; };
    bool receives_midi_notes()  override { return true; };

    MIDIBassBehaviourProxy() : DeviceBehaviourUltimateBase() {
        this->drone_channel = 1;
    }

    virtual const char *get_label() override {
        return "MIDI Bass Proxy";
    }

    virtual void actualSendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
        if (this->debug) Serial.printf("%s#actualSendNoteOn(%i, %i, %i) (calling receive_note_on!)\n", this->get_label(), note, velocity, channel);
        this->receive_note_on(channel, note, velocity);
    }
    virtual void actualSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        if (this->debug) Serial.printf("%s#actualSendNoteOff(%i, %i, %i)  (calling receive_note_off!)\n", this->get_label(), note, velocity, channel);
        this->receive_note_off(channel, note, velocity);
    }
    virtual void actualSendControlChange(uint8_t cc, uint8_t value, uint8_t channel) override {
        this->receive_control_change(channel, cc, value);    
    }

    virtual void on_tick(uint32_t ticks) override {
        MIDIBassBehaviour::on_tick(ticks);
    }
};

extern MIDIBassBehaviourProxy *behaviour_midibassproxy;