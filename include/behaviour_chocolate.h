#ifndef BEHAVIOUR_CHOCOLATE__INCLUDED
#define BEHAVIOUR_CHOCOLATE__INCLUDED

#include <Arduino.h>

#include "Config.h"

#ifdef ENABLE_CHOCOLATEFEET_USB

#include "behaviour_base.h"

#include "multi_usb_handlers.h"

//extern MIDIOutputWrapper *beatstep_output;
//void beatstep_setOutputWrapper(MIDIOutputWrapper *);

//void beatstep_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void chocolate_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void chocolate_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
//void chocolate_handle_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);

class DeviceBehaviour_Chocolate : public DeviceBehaviourBase {
    public:
        uint16_t vid = 0x4353, pid = 0x4B4D;
        //uint16_t vid = 0x1c75, pid = 0x0288;

        virtual uint32_t get_packed_id () override { return (this->vid<<16 | this->pid); }

        void setup_callbacks() override {
            Serial.println("DeviceBehaviour_Chocolate#setup_callbacks()");
            this->device->setHandleNoteOn(chocolate_handle_note_on);
            this->device->setHandleNoteOff(chocolate_handle_note_off);
            //this->device->setHandleNoteOff(chocolate_handle_control_change);
        }

        void note_on(uint8_t channel, uint8_t note, uint8_t velocity) override {
            // TODO: control looper(s) from here
            Serial.printf("chocolate got noteOn chan %i, note %i, velocity %i\n", channel, note, velocity); Serial.flush();
        }

        void note_off(uint8_t channel, uint8_t note, uint8_t velocity) override {
            // TODO: control looper(s) from here
            Serial.printf("chocolate got noteOff chan %i, note %i, velocity %i\n", channel, note, velocity); Serial.flush();
        }

        /*void control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
            Serial.printf("chocolate got controlchange chan %i, cc %i, value %i\n", inChannel, inNumber, inValue); Serial.flush();
        }*/
};

extern DeviceBehaviour_Chocolate *behaviour_chocolate;

#endif

#endif