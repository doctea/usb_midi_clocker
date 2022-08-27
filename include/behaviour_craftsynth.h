#ifndef BEHAVIOUR_CRAFTSYNTH__INCLUDED
#define BEHAVIOUR_CRAFTSYNTH__INCLUDED

#include <Arduino.h>

#include "Config.h"

#ifdef ENABLE_CRAFTSYNTH_USB

#include "behaviour_base_usb.h"
#include "project.h"
#include "clock.h"

#include "multi_usb_handlers.h"

//void craftsynth_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
//void craftsynth_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
//void craftsynth_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);

class DeviceBehaviour_CraftSynth : public DeviceBehaviourUSBBase, public ClockedBehaviour { // public DeviceBehaviourUSBBase { //
    public:
        //uint16_t vid = 0x09e8, pid = 0x0028;
        uint16_t vid = 0x04D8, pid = 0xEE1F;
        virtual uint32_t get_packed_id() override { return (this->vid<<16 | this->pid); }

        virtual void setup_callbacks() override {
            //behaviour_apcmini = this;
            //this->device->setHandleControlChange(craftsynth_control_change);
            //this->device->setHandleNoteOn(craftsynth_note_on);
            //this->device->setHandleNoteOff(craftsynth_note_off);
            Serial.println("DeviceBehaviour_CraftSynth#setup_callbacks()"); Serial.flush();
        };

        /*virtual void init() override {
            Serial.println("DeviceBehaviour_CraftSynth#init()"); Serial.flush();
            started = false;
        }*/

        /*virtual void read() { Serial.println("CraftSynth#read"); };
        virtual void send_clock(uint32_t ticks) { Serial.println("CraftSynth#send_clock"); };
        virtual void loop(uint32_t ticks) { Serial.println("CraftSynth#loop");};
        virtual void on_tick(uint32_t ticks) {Serial.println("CraftSynth#on_tick");};
        virtual void on_restart() {Serial.println("CraftSynth#on_restart");};
        virtual void note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) { Serial.println("CraftSynth#note_on"); };
        virtual void note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) { Serial.println("CraftSynth#note_off"); };
        virtual void control_change (uint8_t inChannel, uint8_t inNumber, uint8_t inValue) { Serial.println("CraftSynth#control_change");};*/

};

//void craftsynth_setOutputWrapper(MIDIOutputWrapper *);
extern DeviceBehaviour_CraftSynth *behaviour_craftsynth;

#endif

#endif