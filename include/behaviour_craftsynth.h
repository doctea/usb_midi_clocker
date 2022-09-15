#ifndef BEHAVIOUR_CRAFTSYNTH__INCLUDED
#define BEHAVIOUR_CRAFTSYNTH__INCLUDED

#include <Arduino.h>

#include "Config.h"

#ifdef ENABLE_CRAFTSYNTH_USB

#include "behaviour_base_usb.h"
#include "project.h"
#include "clock.h"

#include "multi_usb_handlers.h"

#include "parameters/MIDICCParameter.h"

//void craftsynth_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
//void craftsynth_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
//void craftsynth_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);

class DeviceBehaviour_CraftSynth : public DeviceBehaviourUSBBase, public ClockedBehaviour {
    public:
        //uint16_t vid = 0x09e8, pid = 0x0028;
        uint16_t vid = 0x04D8, pid = 0xEE1F;
        virtual uint32_t get_packed_id() override { return (this->vid<<16 | this->pid); }

        virtual char *get_label() override {
            return "CraftSynth 2.0";
        }

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
        virtual void receive_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) { Serial.println("CraftSynth#receive_note_on"); };
        virtual void note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) { Serial.println("CraftSynth#note_off"); };
        virtual void receive_control_change (uint8_t inChannel, uint8_t inNumber, uint8_t inValue) { Serial.println("CraftSynth#receive_control_change");};*/


        virtual LinkedList<DataParameter*> *get_parameters () override {
            static bool already_initialised = false;
            static LinkedList<DataParameter*> *parameters = new LinkedList<DataParameter*>();

            if (already_initialised) 
                return parameters;
            already_initialised = true;

            parameters->clear();

            /*MIDICCParameter *parameter_b = new MIDICCParameter(
                (char*)"CS Spread", 
                //(MIDIOutputWrapper*)midi_matrix_manager->get_target_for_handle((char*)"USB : CraftSynth : ch 1"), 
                behaviour_craftsynth,
                (byte)20,
                (byte)1
            );    // spread
            parameters->add(parameter_b);*/

            parameters->add(new MIDICCParameter((char*)"Distortion",    this,   (byte)12,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Delay Dry/Wet", this,   (byte)13,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Delay Time",    this,   (byte)14,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Osc 1 Wave",    this,   (byte)16,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Osc 2 Wave",    this,   (byte)17,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Osc Mix",       this,   (byte)18,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"CS Spread",     this,   (byte)20,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Filter Morph",  this,   (byte)33,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"CS Cutoff",     this,   (byte)34,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Filter Reso",   this,   (byte)35,   (byte)1));

            return parameters;
        }

};

//void craftsynth_setOutputWrapper(MIDIOutputWrapper *);
extern DeviceBehaviour_CraftSynth *behaviour_craftsynth;

#endif

#endif