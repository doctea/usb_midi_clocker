#ifndef BEHAVIOUR_NEUTRON__INCLUDED
#define BEHAVIOUR_NEUTRON__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviour_base_serial.h"
#include "bpm.h"

class DeviceBehaviour_Neutron : public DeviceBehaviourSerialBase, public ClockedBehaviour {
    public:
        DeviceBehaviour_Neutron (
            midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *input_device, 
            midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output_device
        ) : DeviceBehaviourSerialBase (input_device, output_device) {}
};

extern DeviceBehaviour_Neutron behaviour_neutron;

#endif