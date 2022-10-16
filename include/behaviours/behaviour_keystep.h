#ifndef BEHAVIOUR_KEYSTEP__INCLUDED
#define BEHAVIOUR_KEYSTEP__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviours/behaviour_base_usb.h"
#include "behaviours/behaviour_clocked.h"

#include "multi_usb_handlers.h"

extern MIDIOutputWrapper *keystep_output;
void keystep_setOutputWrapper(MIDIOutputWrapper *);

void keystep_handle_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void keystep_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void keystep_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);

class DeviceBehaviour_Keystep : virtual public DeviceBehaviourUSBBase, virtual public ClockedBehaviour {
    public:
        uint16_t vid = 0x1c75, pid = 0x0288;
        virtual uint32_t get_packed_id () override { return (this->vid<<16 | this->pid); }

        virtual const char *get_label() override {
            return (char*)"Keystep";
        }
        virtual bool has_input() { return true; }

        FLASHMEM virtual void setup_callbacks() override {
            this->device->setHandleNoteOn(keystep_handle_note_on);
            this->device->setHandleNoteOff(keystep_handle_note_off);
            this->device->setHandleControlChange(keystep_handle_control_change);
        }
};

extern DeviceBehaviour_Keystep *behaviour_keystep;

#endif