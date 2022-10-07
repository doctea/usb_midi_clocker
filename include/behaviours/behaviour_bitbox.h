#ifndef BEHAVIOUR_BITBOX__INCLUDED
#define BEHAVIOUR_BITBOX__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviours/behaviour_base_serial.h"
#include "behaviours/behaviour_clocked.h"
#include "bpm.h"

class DeviceBehaviour_Bitbox : public DeviceBehaviourSerialBase, public ClockedBehaviour {
    public:
        virtual const char *get_label() override {
            return (char*)"BitBox";
        }
        /*DeviceBehaviour_Bitbox (
            midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *input_device, 
            midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output_device
        ) : DeviceBehaviourSerialBase (input_device, output_device) {}*/
};

extern DeviceBehaviour_Bitbox *behaviour_bitbox;

#endif