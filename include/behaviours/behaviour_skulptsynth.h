#ifndef BEHAVIOUR_SKULPTSYNTH__INCLUDED
#define BEHAVIOUR_SKULPTSYNTH__INCLUDED

#include <Arduino.h>

#include "Config.h"

#ifdef ENABLE_SKULPTSYNTH_USB

#include "behaviours/behaviour_base_usb.h"
#include "behaviours/behaviour_clocked.h"
#include "behaviours/behaviour_modwheelreceiver.h"
#include "project.h"
#include "clock.h"

#include "usb/multi_usb_handlers.h"

#include "parameters/MIDICCParameter.h"

//void skulptsynth_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
//void skulptsynth_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
//void skulptsynth_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);

// customised parameter control widget for the Spread parameter that shows the equivalent value
class SkulptSynthSpreadParameter : public MIDICCParameter<> {
    public:
        SkulptSynthSpreadParameter (const char *label, DeviceBehaviourUltimateBase *target) 
            : MIDICCParameter<>(label, target, (byte)20, (byte)1) {
        }

        virtual const char* parseFormattedDataType(byte value) override {
            static char fmt[MENU_C_MAX] = "              ";
            switch (value) {
                case 0 ... 63:
                    snprintf(fmt, MENU_C_MAX, "%-3i", value); break;
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


class SkulptSynthOsc1Parameter : public MIDICCParameter<> {
    public:
        SkulptSynthOsc1Parameter (const char *label, DeviceBehaviourUltimateBase *target, byte cc_number, byte channel = 1) 
            : MIDICCParameter<>(label, target, (byte)cc_number, (byte)channel) {
        }

        virtual const char* parseFormattedDataType(byte value) override {
            //static char fmt[MENU_C_MAX] = "              ";
            switch (value) {
                case 0 ... 21:
                    return "Sin";
                case 22 ... 42:
                    return "Tri";
                case 43 ... 63:
                    return "Saw";
                case 64 ... 127:
                    return "PWM";
            }
            return "n/a";
            //return "??";
        };
};

class SkulptSynthOsc2Parameter : public MIDICCParameter<> {
    public:
        SkulptSynthOsc2Parameter (const char *label, DeviceBehaviourUltimateBase *target, byte cc_number, byte channel = 1) 
            : MIDICCParameter<>(label, target, (byte)cc_number, (byte)channel) {
        }

        virtual const char* parseFormattedDataType(byte value) override {
            //static char fmt[MENU_C_MAX] = "              ";
            switch (value) {
                case 0 ... 21:
                    return "Sin";
                case 22 ... 42:
                    return "Tri";
                case 43 ... 63:
                    return "Saw";
                case 64 ... 85:
                    return "Sqr";
                case 86 ... 127:
                    return "Nse";
            }
            return "n/a";
            //return "??";
        };
};

class SkulptSynthVoiceModeParameter : public MIDICCParameter<> {
    public:
        SkulptSynthVoiceModeParameter (const char *label, DeviceBehaviourUltimateBase *target) 
            : MIDICCParameter<>(label, target, (byte)9, (byte)1) {
        }

        virtual const char* parseFormattedDataType(byte value) override {
            static char fmt[MENU_C_MAX] = "              ";
            switch (value) {
                case 0 ... 42:
                    return "Mono";
                case 43 ... 85:
                    return "Duo";
                case 86 ... 127:
                    return "Poly";
            }
            return fmt;
            //return "??";
        };
};

class SkulptSynthOnOffParameter : public MIDICCParameter<> {
    public:
        SkulptSynthOnOffParameter (const char *label, DeviceBehaviourUltimateBase *target, byte cc_number) 
            : MIDICCParameter<>(label, target, (byte)cc_number, (byte)1) {
        }

        virtual const char* parseFormattedDataType(byte value) override {
            static char fmt[MENU_C_MAX] = "              ";
            switch (value) {
                case 0 ... 63:
                    return "Off";
                case 64 ... 127:
                    return "On";
            }
            return fmt;
            //return "??";
        };
};

class SkulptSynthCenteredParameter : public MIDICCParameter<> {
    public:
        SkulptSynthCenteredParameter (const char *label, DeviceBehaviourUltimateBase *target, byte cc_number) 
            : MIDICCParameter<>(label, target, (byte)cc_number, (byte)1) {
        }

        virtual const char* parseFormattedDataType(byte value) override {
            static char fmt[MENU_C_MAX] = "              ";
            snprintf(fmt, 8, "%3i", value - 63);
            return fmt;
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

class DeviceBehaviour_SkulptSynth : public DeviceBehaviourUSBBase, public ClockedBehaviour, public ModwheelReceiver {
    //using ClockedBehaviour::DeviceBehaviourUltimateBase;
    using ClockedBehaviour::DeviceBehaviourUltimateBase::parameters;
    
    public:
        //uint16_t vid = 0x09e8, pid = 0x0028;
        uint16_t vid = 0x04D8, pid = 0xEEFE;
        virtual uint32_t get_packed_id() override { return (this->vid<<16 | this->pid); }

        virtual const char *get_label() override {
            return "SkulptSynth";
        }
        virtual bool transmits_midi_notes() { return true; }

        /*virtual void setup_callbacks() override {
            //behaviour_apcmini = this;
            //this->device->setHandleControlChange(skulptsynth_control_change);
            //this->device->setHandleNoteOn(skulptsynth_note_on);
            //this->device->setHandleNoteOff(skulptsynth_note_off);
            Serial.println(F("DeviceBehaviour_SkulptSynth#setup_callbacks()")); Serial_flush();
        };*/

        /*virtual void init() override {
            Serial.println("DeviceBehaviour_SkulptSynth#init()"); Serial_flush();
            started = false;
        }*/

        /*virtual void read() { Serial.println("SkulptSynth#read"); };
        virtual void send_clock(uint32_t ticks) { Serial.println("SkulptSynth#send_clock"); };
        virtual void loop(uint32_t ticks) { Serial.println("SkulptSynth#loop");};
        virtual void on_tick(uint32_t ticks) {Serial.println("SkulptSynth#on_tick");};
        virtual void on_restart() {Serial.println("SkulptSynth#on_restart");};
        virtual void receive_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) { Serial.println("SkulptSynth#receive_note_on"); };
        virtual void note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) { Serial.println("SkulptSynth#note_off"); };
        virtual void receive_control_change (uint8_t inChannel, uint8_t inNumber, uint8_t inValue) { Serial.println("SkulptSynth#receive_control_change");};*/

        virtual void sendControlChange(byte cc_number, byte value, byte channel = 0) override {
            //if (this->debug) Serial.printf(F("%s#sendControlChange(cc=%i, value=%i, channel=%i)\n"), this->get_label(), cc_number, value, channel);
            // if we receive a value from another device, then update the proxy parameter, which will handle the actual sending
            // if modwheel should handle this event, handle it and return early
            if (ModwheelReceiver::process(cc_number, value, channel)) 
                return;

            DeviceBehaviourUltimateBase::sendControlChange(cc_number, value, channel);
        }

        FLASHMEM virtual LinkedList<FloatParameter*> *initialise_parameters() override {
            Serial.printf(F("DeviceBehaviour_SkulptSynth#initialise_parameters()..."));
            static bool already_initialised = false;
            if (already_initialised)
                return this->parameters;

            DeviceBehaviourUSBBase::initialise_parameters();
            ClockedBehaviour::initialise_parameters();

            ModwheelReceiver::initialise_parameters();

            // todo: read these from a configuration file
            // todo: add the rest of the available parameters from https://midi.guide/d/modal/skulpt/

            parameters->add(new MIDICCParameter<>("Glide",         this,   (byte)5,    (byte)1));
            parameters->add(new MIDICCParameter<>("Distortion",    this,   (byte)12,   (byte)1));
            parameters->add(new MIDICCParameter<>("Delay Dry/Wet", this,   (byte)13,   (byte)1));
            parameters->add(new MIDICCParameter<>("Delay Time",    this,   (byte)14,   (byte)1));
            parameters->add(new MIDICCParameter<>("Delay Feedback",this,   (byte)15,   (byte)1));

            parameters->add(new SkulptSynthOsc1Parameter("Osc 1 Wave",    this,   (byte)16,   (byte)1));
            parameters->add(new SkulptSynthOsc2Parameter("Osc 2 Wave",    this,   (byte)17,   (byte)1));
            parameters->add(new MIDICCParameter<>("Osc Mix",       this,   (byte)18,   (byte)1));

            parameters->add(new MIDICCParameter<>("Osc Morph",     this,   (byte)33,   (byte)1));
            parameters->add(new MIDICCParameter<>("Filter Cutoff", this,   (byte)34,   (byte)1));
            parameters->add(new MIDICCParameter<>("Filter Reso",   this,   (byte)35,   (byte)1));

            /*parameters->add(new MIDICCParameter<>("FM amt",        this,   (byte)19,   (byte)1));
            parameters->add(new SkulptSynthSpreadParameter((char*)"Spread",        this)); //,   (byte)20,   (byte)1));
            parameters->add(new SkulptSynthOnOffParameter("Chord mode", this, 21));

            parameters->add(new SkulptSynthVoiceModeParameter("Voice Mode", this));
            parameters->add(new SkulptSynthOnOffParameter("Ring mod", this, 53));
            parameters->add(new SkulptSynthOnOffParameter("Delay sync", this, 55));

            parameters->add(new SkulptSynthCenteredParameter("Velo depth", this, 62));
            parameters->add(new SkulptSynthCenteredParameter("Note depth", this, 63));
            parameters->add(new SkulptSynthOnOffParameter("Sustain pedal", this, 64));
            parameters->add(new SkulptSynthCenteredParameter("Aft depth", this, 65));*/

            Serial.printf(F("Finished initialise_parameters() in %s\n"), this->get_label());

            return parameters;
        }
};

//void skulptsynth_setOutputWrapper(MIDIOutputWrapper *);
extern DeviceBehaviour_SkulptSynth *behaviour_skulptsynth;

#endif

#endif