#ifndef BEHAVIOUR_KEYSTEP__INCLUDED
#define BEHAVIOUR_KEYSTEP__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviour_base.h"

#include "multi_usb_handlers.h"

extern MIDIOutputWrapper *keystep_output;
void keystep_setOutputWrapper(MIDIOutputWrapper *);

//void keystep_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void keystep_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void keystep_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);

class DeviceBehaviour_Keystep : public DeviceBehaviourUSBBase, public ClockedBehaviour {
    public:
        uint16_t vid = 0x1c75, pid = 0x0288;
        virtual uint32_t get_packed_id () override { return (this->vid<<16 | this->pid); }

        void setup_callbacks() override {
            this->device->setHandleNoteOn(keystep_handle_note_on);
            this->device->setHandleNoteOff(keystep_handle_note_off);
        }

        /*void note_on(uint8_t channel, uint8_t note, uint8_t velocity) override {
            if (keystep_output!=nullptr)
                keystep_output->sendNoteOn(note, velocity); //, MIDI_CHANNEL_BITBOX 3);
        }

        void note_off(uint8_t channel, uint8_t note, uint8_t velocity) override {
            if (keystep_output!=nullptr)
                keystep_output->sendNoteOff(note, velocity);
        }*/

};

extern DeviceBehaviour_Keystep *behaviour_keystep;

#endif