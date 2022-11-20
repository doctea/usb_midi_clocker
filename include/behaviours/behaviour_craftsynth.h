#ifndef BEHAVIOUR_CRAFTSYNTH__INCLUDED
#define BEHAVIOUR_CRAFTSYNTH__INCLUDED

#include <Arduino.h>

#include "Config.h"

#ifdef ENABLE_CRAFTSYNTH_USB

#include "behaviours/behaviour_base_usb.h"
#include "behaviours/behaviour_modwheelreceiver.h"
#include "project.h"
#include "clock.h"

#include "multi_usb_handlers.h"

#include "parameters/MIDICCParameter.h"

//void craftsynth_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
//void craftsynth_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
//void craftsynth_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);

// customised parameter control widget for the Spread parameter that shows the equivalent value
class CraftSynthSpreadParameter : public MIDICCParameter<> {
    public:
        CraftSynthSpreadParameter (char *label, DeviceBehaviourUltimateBase *target) 
            : MIDICCParameter<>(label, target, (byte)20, (byte)1) {
        }

        virtual const char* parseFormattedDataType(byte value) {
            static char fmt[MENU_C_MAX] = "              ";
            switch (value) {
                case 0 ... 63:
                    sprintf(fmt, "%-3i", value); break;
                case 64 ... 70:
                    return "Maj";
                case 71 ... 77:
                    return "Min";
                case 78 ... 84:
                    return "M6";
                case 85 ... 91:
                    return "Su4";
                case 92 ... 98:
                    return "5th";
                case 99 ... 105:
                    return "5tO";
                case 106 ... 112:
                    return "O++";
                case 113 ... 119:
                    return "O+-";
                case 120 ... 127:
                    return "O--";
            }
            return fmt;
            //return "??";
        };
};


// todo: some generic MIDICC types:-
//      on / offs with configurable threshold
//      full left / full right kinda things for osc mix    

// TODO:- LFO1 Rate = CC 36 
/*NO SYNC: 0-127 = 0.02Hz - 32Hz
SYNC: 0-7 = 1/16 / 8-15 = 1/8 / 16-23 = 3/16 / 24-31 = 1/4 /
32-39 = 3/8 / 40-47 = 1/2 / 48-55 = 3/4 / 56-63 = 1 / 64-71 = 3/2
/ 72-79 = 2 / 80-87 = 3 / 88-95 = 4 / 96-103 = 6 /104-111 = 8 /
112-119 = 12 / 120-127 = 16*/

// LFO2 Rate = CC 47
/* CC 47 NO SYNC: 0-63 = 0-32Hz Free / 64-71 Root/8 / 72-79 Root/4 /
80-87 Root/2 / 88-95 Root / 96-103 Root*1.5 /104-111 Root*2 /
112-119 Root*2.5 / 120-127 Root*3
SYNC: 0-7 = 1/16 / 8-15 = 1/8 / 16-23 =1/4 / 24-31 =1/2 / 32-39
= 1 / 40-47 = 5/4 / 48-55 =2 / 56-63 = 4 (Cycles per beat)*/

//TODO:- LFO1 Shape CC 39 / LFO2 Shape CC 50
/* 0-32 Sine to Triangle / 33-64 - Triangle to Sawtooth / 65-96 -
Sawtooth to Square / 97-127 - Square to Sample and Hold*/

class DeviceBehaviour_CraftSynth : public DeviceBehaviourUSBBase, public ClockedBehaviour, public ModwheelReceiver {
    //using ClockedBehaviour::DeviceBehaviourUltimateBase;
    using ClockedBehaviour::DeviceBehaviourUltimateBase::parameters;
    
    public:
        //uint16_t vid = 0x09e8, pid = 0x0028;
        uint16_t vid = 0x04D8, pid = 0xEE1F;
        virtual uint32_t get_packed_id() override { return (this->vid<<16 | this->pid); }

        virtual const char *get_label() override {
            return (char*)"CraftSynth 2.0";
        }
        virtual bool has_output() { return true; }

        /*virtual void setup_callbacks() override {
            //behaviour_apcmini = this;
            //this->device->setHandleControlChange(craftsynth_control_change);
            //this->device->setHandleNoteOn(craftsynth_note_on);
            //this->device->setHandleNoteOff(craftsynth_note_off);
            Serial.println(F("DeviceBehaviour_CraftSynth#setup_callbacks()")); Serial_flush();
        };*/

        /*virtual void init() override {
            Serial.println("DeviceBehaviour_CraftSynth#init()"); Serial_flush();
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

        virtual void sendControlChange(byte cc_number, byte value, byte channel = 0) override {
            //if (this->debug) Serial.printf(F("%s#sendControlChange(cc=%i, value=%i, channel=%i)\n"), this->get_label(), cc_number, value, channel);
            // if we receive a value from another device, then update the proxy parameter, which will handle the actual sending
            // if modwheel should handle this event, handle it and return early
            if (ModwheelReceiver::process(cc_number, value, channel)) 
                return;

            DeviceBehaviourUltimateBase::sendControlChange(cc_number, value, channel);
        }

        FLASHMEM virtual LinkedList<DoubleParameter*> *initialise_parameters() override {
            //Serial.printf(F("DeviceBehaviour_CraftSynth#initialise_parameters()..."));
            static bool already_initialised = false;
            if (already_initialised)
                return this->parameters;

            //Serial.println(F("\tcalling DeviceBehaviourUSBBase::initialise_parameters()")); 
            DeviceBehaviourUSBBase::initialise_parameters();
            //Serial.println(F("\tcalling ClockedBehaviour::initialise_parameters()"));
            ClockedBehaviour::initialise_parameters();

            ModwheelReceiver::initialise_parameters();

            //Serial.println(F("\tAdding parameters..."));
            //parameters->clear();
            // todo: read these from a file
            //this->add_parameters();
            parameters->add(new MIDICCParameter<>((char*)"Glide",         this,   (byte)5,    (byte)1));
            parameters->add(new MIDICCParameter<>((char*)"Distortion",    this,   (byte)12,   (byte)1));
            parameters->add(new MIDICCParameter<>((char*)"Delay Dry/Wet", this,   (byte)13,   (byte)1));
            parameters->add(new MIDICCParameter<>((char*)"Delay Time",    this,   (byte)14,   (byte)1));
            parameters->add(new MIDICCParameter<>((char*)"Delay Feedback",this,   (byte)15,   (byte)1));
            parameters->add(new MIDICCParameter<>((char*)"Osc 1 Wave",    this,   (byte)16,   (byte)1));
            parameters->add(new MIDICCParameter<>((char*)"Osc 2 Wave",    this,   (byte)17,   (byte)1));
            parameters->add(new MIDICCParameter<>((char*)"Osc Mix",       this,   (byte)18,   (byte)1));
            //parameters->add(new MIDICCParameter((char*)"Spread",        this,   (byte)20,   (byte)1));
            parameters->add(new CraftSynthSpreadParameter((char*)"Spread", this));
            parameters->add(new MIDICCParameter<>((char*)"Filter Morph",  this,   (byte)33,   (byte)1));
            parameters->add(new MIDICCParameter<>((char*)"Filter Cutoff", this,   (byte)34,   (byte)1));
            parameters->add(new MIDICCParameter<>((char*)"Filter Reso",   this,   (byte)35,   (byte)1));

            //Serial.printf(F("Finished initialise_parameters() in %s\n"), this->get_label());

            return parameters;
        }
};

//void craftsynth_setOutputWrapper(MIDIOutputWrapper *);
extern DeviceBehaviour_CraftSynth *behaviour_craftsynth;

#endif

#endif