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

        virtual void setup_callbacks() override {
            //behaviour_apcmini = this;
            if (this->device==nullptr) return;
            this->device->setHandleControlChange(bamble_control_change);
            this->device->setHandleNoteOn(bamble_note_on);
            this->device->setHandleNoteOff(bamble_note_off);
        };

        virtual void init() override {
            if (!DeviceBehaviourUSBBase::is_connected()) return;
            started = false;

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