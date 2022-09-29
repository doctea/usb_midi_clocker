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

class CraftSynthSpreadParameter : public MIDICCParameter {
    public:
        CraftSynthSpreadParameter (char *label, DeviceBehaviourUltimateBase *target) 
            : MIDICCParameter(label, target, (byte)20, (byte)1) {
        }

        virtual const char* parseFormattedDataType(byte value) {
            static char fmt[20] = "              ";
            //sprintf(fmt, "%i", this->getCurrentDataValue()); //get_midi_value_for_double(this->getCurrentNormalValue()));
            //sprintf(fmt, "%s")
            switch (value) {
                case 0 ... 63:
                    sprintf(fmt, "%-3i", value); break;
                case 64 ... 70:
                    strcpy(fmt, "Maj"); break;
                case 71 ... 77:
                    strcpy(fmt, "Min"); break;
                case 78 ... 84:
                    strcpy(fmt, "M6"); break;
                case 85 ... 91:
                    strcpy(fmt, "Su4"); break;
                case 92 ... 98:
                    strcpy(fmt, "5th"); break;
                case 99 ... 105:
                    strcpy(fmt, "5tO"); break;
                case 106 ... 112:
                    strcpy(fmt, "O++"); break;
                case 113 ... 119:
                    strcpy(fmt, "O+-"); break;
                case 120 ... 127:
                    strcpy(fmt, "O--"); break;
                default:
                    strcpy(fmt, "??"); break;
            }
            return fmt;
        };
};



class DeviceBehaviour_CraftSynth : public DeviceBehaviourUSBBase, public ClockedBehaviour {
    public:
        //uint16_t vid = 0x09e8, pid = 0x0028;
        uint16_t vid = 0x04D8, pid = 0xEE1F;
        virtual uint32_t get_packed_id() override { return (this->vid<<16 | this->pid); }

        virtual const char *get_label() override {
            return (char*)"CraftSynth 2.0";
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

        virtual LinkedList<DoubleParameter*> *initialise_parameters() override {
            Serial.printf("DeviceBehaviour_CraftSynth#initialise_parameters()...");
            static bool already_initialised = false;
            if (already_initialised)
                return parameters;

            Serial.println("\tcalling DeviceBehaviourUSBBase::initialise_parameters()"); 
            DeviceBehaviourUSBBase::initialise_parameters();
            Serial.println("\tcalling ClockedBehaviour::initialise_parameters()"); 
            ClockedBehaviour::initialise_parameters();

            Serial.println("\tAdding parameters...");
            //parameters->clear();
            // todo: read these from a file
            //this->add_parameters();
            parameters->add(new MIDICCParameter((char*)"Distortion",    this,   (byte)12,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Delay Dry/Wet", this,   (byte)13,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Delay Time",    this,   (byte)14,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Osc 1 Wave",    this,   (byte)16,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Osc 2 Wave",    this,   (byte)17,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Osc Mix",       this,   (byte)18,   (byte)1));
            //parameters->add(new MIDICCParameter((char*)"Spread",        this,   (byte)20,   (byte)1));
            parameters->add(new CraftSynthSpreadParameter((char*)"Spread", this));
            parameters->add(new MIDICCParameter((char*)"Filter Morph",  this,   (byte)33,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Cutoff",        this,   (byte)34,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Filter Reso",   this,   (byte)35,   (byte)1));

            Serial.printf("Finished initialise_parameters() in %s\n", this->get_label());

            return parameters;
        }
};

//void craftsynth_setOutputWrapper(MIDIOutputWrapper *);
extern DeviceBehaviour_CraftSynth *behaviour_craftsynth;

#endif

#endif