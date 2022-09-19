#ifndef BEHAVIOUR_BAMBLE__INCLUDED
#define BEHAVIOUR_BAMBLE__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviour_base.h"
#include "project.h"
#include "clock.h"

#include "multi_usb_handlers.h"

//extern MIDITrack mpk49_loop_track;
//class MIDITrack;

void bamble_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void bamble_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void bamble_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);

class DeviceBehaviour_Bamble : virtual public DeviceBehaviourUSBBase, virtual public ClockedBehaviour {
    public:
        using DeviceBehaviourUltimateBase::receive_control_change;
        using DeviceBehaviourUltimateBase::receive_note_on;
        using DeviceBehaviourUltimateBase::receive_note_off;
        using DeviceBehaviourUltimateBase::debug;

        uint16_t vid = 0x2886, pid = 0x800B;
        virtual uint32_t get_packed_id() override { return (this->vid<<16 | this->pid); }

        char *get_label() override {
            return "Bambleweeny57";
        }

        virtual void setup_callbacks() override {
            //behaviour_apcmini = this;
            if (this->device==nullptr) return;
            this->device->setHandleControlChange(bamble_control_change);
            this->device->setHandleNoteOn(bamble_note_on);
            this->device->setHandleNoteOff(bamble_note_off);
        };

        source_id_t source_ids[5] = { -1, -1, -1, -1, -1 };
        virtual void self_register_midi_matrix_targets(MIDIMatrixManager *midi_matrix_manager) {
            // register multiple inputs / outputs

            midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"USB : Bamble : ch 1", this, 1));
            midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"USB : Bamble : ch 2", this, 2));
            midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"USB : Bamble : ch 3", this, 3));
            midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"USB : Bamble : ch 4", this, 4));
            midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"USB : Bamble : drums",this, 10));
        }

        #ifdef ENABLE_BAMBLE_INPUT
            virtual void self_register_midi_matrix_sources(MIDIMatrixManager *midi_matrix_manager) {
                this->source_ids[0] = midi_matrix_manager->register_source(this, "bamble_input_ch1");
                this->source_ids[1] = midi_matrix_manager->register_source(this, "bamble_input_ch2");
                this->source_ids[2] = midi_matrix_manager->register_source(this, "bamble_input_ch3");
                this->source_ids[3] = midi_matrix_manager->register_source(this, "bamble_input_ch4");
                this->source_ids[4] = midi_matrix_manager->register_source(this, "bamble_input_ch16");
            }

            // special version that uses source_ids array based on incoming channel to route
            void receive_note_on(uint8_t channel, uint8_t note, uint8_t velocity) override {
                if (channel==16) channel = 5;    // remap midimuso drums to the last channel in the range
                midi_matrix_manager->processNoteOn(this->source_ids[channel-1], note, velocity); //, channel);
            }

            // called when a note_off message is received from the device
            void receive_note_off(uint8_t channel, uint8_t note, uint8_t velocity) override {
                if (channel==16) channel = 5;    // remap midimuso drums to the last channel in the range
                midi_matrix_manager->processNoteOff(this->source_ids[channel-1], note, velocity); //, channel);
            }
        #endif

        virtual void init() override {
            if (!DeviceBehaviourUSBBase::is_connected()) return;
            started = false;

            #ifdef ENABLE_BAMBLE_INPUT
                // enable midi echo - crashes it ?
                DeviceBehaviourUSBBase::sendControlChange(21, 127, 10);
            #endif

            // this should disable euclidian pulses on the pitch outputs ch1 + ch2
            DeviceBehaviourUSBBase::sendControlChange(78, 0, 10);
            DeviceBehaviourUSBBase::sendControlChange(79, 0, 10);
            DeviceBehaviourUSBBase::sendControlChange(50, 0, 10);
            DeviceBehaviourUSBBase::sendControlChange(51, 0, 10);

            // sustain to max for the envelope outputs
            DeviceBehaviourUSBBase::sendControlChange(67, 127, 10);
            DeviceBehaviourUSBBase::sendControlChange(67, 127, 11);
            DeviceBehaviourUSBBase::sendControlChange(75, 127, 10);
            DeviceBehaviourUSBBase::sendControlChange(75, 127, 11);
            DeviceBehaviourUSBBase::sendControlChange(83, 127, 10);
            DeviceBehaviourUSBBase::sendControlChange(83, 127, 11);
            DeviceBehaviourUSBBase::sendControlChange(91, 127, 10);
            DeviceBehaviourUSBBase::sendControlChange(91, 127, 11);
            DeviceBehaviourUSBBase::sendControlChange(99, 127, 10);
            DeviceBehaviourUSBBase::sendControlChange(99, 127, 11);
        }
};

extern DeviceBehaviour_Bamble *behaviour_bamble;

#endif