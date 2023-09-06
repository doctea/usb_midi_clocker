#ifndef BEHAVIOUR_BITBOX__INCLUDED
#define BEHAVIOUR_BITBOX__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviours/behaviour_base_serial.h"
#include "behaviours/behaviour_clocked.h"
#include "bpm.h"

class DeviceBehaviour_Bitbox : public DeviceBehaviourSerialBase, public DividedClockedBehaviour {
    public:
        virtual const char *get_label() override {
            return (const char*)"BitBox";
        }

        virtual bool receives_midi_notes() override {
            return false;
        }
        virtual bool transmits_midi_notes() override {
            return false;
        }
};

extern DeviceBehaviour_Bitbox *behaviour_bitbox;

#endif