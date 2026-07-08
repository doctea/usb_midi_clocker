#pragma once

#include <Arduino.h>

#include "Config.h"
#include "behaviours/behaviour_base_usb.h"
#include "behaviours/behaviour_clocked.h"
#include "midi/midi_cc_source.h"

#include "usb/multi_usb_handlers.h"

extern MIDIOutputWrapper *keystep_output;
void keystep_setOutputWrapper(MIDIOutputWrapper *);

void keystep_handle_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void keystep_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void keystep_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void keystep_handle_pitchbend(uint8_t inChannel, int bend);

class DeviceBehaviour_Keystep : 
    virtual public DeviceBehaviourUSBBase, 
    virtual public ClockedBehaviour,
    virtual public MIDI_CC_Source
{
    public:
        uint16_t vid = 0x1c75, pid = 0x0288;
        virtual uint32_t get_packed_id () override { return (this->vid<<16 | this->pid); }

        DeviceBehaviour_Keystep() : DeviceBehaviourUSBBase() {
            // initialise the CCs that this device can translate into ParameterInputs
            this->addCCParameterInput("ModWheel", "Keystep", (byte)1); //, (byte)1);
            // this->addParameterInput("PitchBend", "Keystep", (byte)2, (byte)1);
        }

        virtual const char *get_label() override {
            return (char*)"Keystep";
        }
        virtual bool receives_midi_notes() override { return true; }

        virtual void receive_control_change(uint8_t channel, uint8_t number, uint8_t value) override {
            //Serial.printf("DeviceBehaviour_Keystep::receive_control_change(%i, %i, %i)\n", channel, number, value);
            MIDI_CC_Source::update_parameter_inputs_cc(number, value, channel);
        }

        virtual void receive_pitch_bend(uint8_t channel, int16_t value) override {
            //Serial.printf("DeviceBehaviour_Keystep::receive_pitch_bend(%i, %i)\n", channel, value);
            MIDI_CC_Source::update_parameter_inputs_pitch_bend(value, channel);
        }

        // FLASHMEM 
        virtual void setup_callbacks() override {
            this->device->setHandleNoteOn(keystep_handle_note_on);
            this->device->setHandleNoteOff(keystep_handle_note_off);
            this->device->setHandleControlChange(keystep_handle_control_change);
            this->device->setHandlePitchChange(keystep_handle_pitchbend);
        }
};

extern DeviceBehaviour_Keystep *behaviour_keystep;
