#ifndef BEHAVIOUR_NEUTRON__INCLUDED
#define BEHAVIOUR_NEUTRON__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviour_base_serial.h"
#include "bpm.h"

#include "midi_mapper_matrix_manager.h"

class DeviceBehaviour_Neutron : public DeviceBehaviourSerialBase, public ClockedBehaviour {
    public:
        bool new_bar = true;        // reset on new bar, so that we can pick up only the first note played
        bool drone_enabled = false; // whether or not drone is enabled
        int last_drone_note = -1;   // note that we should drone on
        byte drone_channel = 0;     // channel that we should drone on

        virtual bool is_drone() {
            return drone_enabled;
        }
        virtual void set_drone(bool value) {
            if (this->drone_enabled && !value && this->last_drone_note>=0) {
                // turning drone off and there is a note to kill
                if (this->debug) Serial.println("set_drone setting drone_enabled off - calling kill_drone_note!");
                this->kill_drone_note();
            } else if (!this->drone_enabled && value) {
                // should kill any (non-drone) notes that we have playing... except thats handled by the MIDIOutputWrapper that's wrapped around this behaviour, rather than the output that this is connected to.  
                // hmmm.  so, how do we untangle this?  should DeviceBehaviours be subclasses of MIDIOutputWrapper too?
                // TODO: yeah this ugly af kludge should work for now
                midi_matrix_manager->stop_all_notes_for_target(midi_matrix_manager->get_target_id_for_handle("S3 : Neutron : ch 4"));
            }
            this->drone_enabled = value;
            this->last_drone_note = -1;
        }
        virtual int get_drone_note() {
            return this->last_drone_note;
        }

        virtual void kill_drone_note() {            
            if (last_drone_note>=0) {
                if (this->debug) Serial.printf("\t\tkill_drone_note(%i, %i, %i)\n", last_drone_note, 0, drone_channel);
                DeviceBehaviourSerialBase::sendNoteOff(last_drone_note, 0, drone_channel);
            }
            last_drone_note = -1;
        }
        virtual void send_drone_note() {
            //DeviceBehaviourSerialBase::sendNoteOff(last_drone_note, 0, drone_channel);
            if (last_drone_note>=0) {
                if (this->debug) Serial.printf("\t\tsend_drone_note sending (%i, %i, %i)\n", last_drone_note, 127, drone_channel);
                DeviceBehaviourSerialBase::sendNoteOn(last_drone_note, 127, drone_channel);
            }
        }

        virtual void on_bar(int bar) override {
            if (this->debug) Serial.printf("begin>=DeviceBehaviour_Neutron#on_bar(%i)\n", bar);
            //if (drone && last_drone_note>=0) {
            //    Serial.printf("\tDeviceBehaviour_Neutron#on_bar is in drone mode with drone note %i!\n", last_drone_note);
            //    this->send_drone_note();
            //}
            new_bar = true;
            last_drone_note = -1;
            //Serial.println("end<=DeviceBehaviour_Neutron#on_bar()");
        }
        virtual void on_end_bar(int bar) override {
            if (this->debug) Serial.println("DeviceBehaviour_Neutron#on_end_bar!");
            if (drone_enabled && last_drone_note>=0)
                this->kill_drone_note();
        }

        virtual void sendNoteOn(byte pitch, byte velocity, byte channel = 0) override {
            //Serial.printf("DeviceBehaviour_Neutron#sendNoteOn(%i, %i, %i)\n", pitch, velocity, channel, last_drone_note);
            if (drone_enabled) {
                // do drone stuff
                if (this->debug) Serial.printf("DeviceBehaviour_Neutron#sendNoteOn in DRONE mode!\n");
                if (last_drone_note==-1 && new_bar) {
                    last_drone_note = pitch;
                    if (this->debug) Serial.printf("\tDeviceBehaviour_Neutron#sendNoteOn(%i, %i, %i) got new last_drone_note: %i\n", pitch, velocity, channel, last_drone_note);
                    new_bar = false;
                    drone_channel = channel;
                    this->send_drone_note();
                }
            } else
                DeviceBehaviourSerialBase::sendNoteOn(pitch, velocity, channel);
        }

        virtual void sendNoteOff(byte pitch, byte velocity, byte channel) override {
            if (drone_enabled) {
                //
            } else
                DeviceBehaviourSerialBase::sendNoteOff(pitch, velocity, channel);
        }
};

extern DeviceBehaviour_Neutron behaviour_neutron;

#endif