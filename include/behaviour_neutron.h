#ifndef BEHAVIOUR_NEUTRON__INCLUDED
#define BEHAVIOUR_NEUTRON__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviour_base_serial.h"
#include "bpm.h"

class DeviceBehaviour_Neutron : public DeviceBehaviourSerialBase, public ClockedBehaviour {
    public:
        // this commented-out logic for doing 'bass drone' stuff; except midi_matrix stuff doesn't pass through the device here, so this is basically useless here
        //      TODO: either move this to MIDIOutputWrapper, or need to refactor things so that sent notes pass through the Behaviour
        bool new_bar = true;
        bool drone = false;
        int last_drone_note = -1;

        virtual bool is_drone() {
            return drone;
        }
        virtual void set_drone(bool value) {
            this->drone = value;
            this->last_drone_note = -1;
        }

        void on_bar(int bar) override {
            DeviceBehaviourSerialBase::sendNoteOff(last_drone_note, 127);
            DeviceBehaviourSerialBase::sendNoteOn(last_drone_note, 127);
            new_bar = true;
        }
        void on_end_bar(int bar) override {
            DeviceBehaviourSerialBase::sendNoteOff(last_drone_note, 127);
        }

        void sendNoteOn(byte pitch, byte velocity, byte channel = 0) override {
            if (drone) {
                if (last_drone_note==-1 || new_bar)
                    last_drone_note = pitch;
                // do drone stuff
            } else
                DeviceBehaviourSerialBase::sendNoteOn(pitch, velocity, channel);
        }

        void sendNoteOff(byte pitch, byte velocity, byte channel) override {
            if (drone) {
                //
            } else
                DeviceBehaviourSerialBase::sendNoteOff(pitch, velocity, channel);
        }
};

extern DeviceBehaviour_Neutron behaviour_neutron;

#endif