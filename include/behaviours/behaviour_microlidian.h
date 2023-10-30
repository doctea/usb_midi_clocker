#ifndef BEHAVIOUR_MICROLIDIAN__INCLUDED
#define BEHAVIOUR_MICROLIDIAN__INCLUDED

#include <Arduino.h>

#include "Config.h"

// microlidian !

#ifdef ENABLE_MICROLIDIAN

#include "behaviours/behaviour_base_usb.h"
#include "behaviours/behaviour_clocked.h"
#include "midi/midi_cc_source.h"
//#include "behaviours/behaviour_modwheelreceiver.h"
#include "project.h"
#include "clock.h"

#include "usb/multi_usb_handlers.h"

#include <Drums.h>

//#include "parameters/MIDICCParameter.h"

void microlidian_control_change(byte number, byte value, byte channel);
void microlidian_note_on(byte pitch, byte value, byte channel);
void microlidian_note_off(byte pitch, byte value, byte channel);

class DeviceBehaviour_Microlidian : public DeviceBehaviourUSBBase, public DividedClockedBehaviour, public MIDI_CC_Source { //}, public ModwheelReceiver {
    //using ClockedBehaviour::DeviceBehaviourUltimateBase;
    using DividedClockedBehaviour::DeviceBehaviourUltimateBase::parameters;
    using DeviceBehaviourUltimateBase::receive_note_on;
    using DeviceBehaviourUltimateBase::receive_note_off;
    using ClockedBehaviour::on_tick;
    
    public:

        DeviceBehaviour_Microlidian() : DividedClockedBehaviour() {
            this->addParameterInput("Env1", "Microlidian", (byte)1 /*MUSO_CC_CV_1*/, (byte)1);
            this->addParameterInput("Env2", "Microlidian", (byte)7 /*MUSO_CC_CV_2*/, (byte)1);
            this->addParameterInput("Env3", "Microlidian", (byte)11 /*MUSO_CC_CV_3*/, (byte)1);
            this->addParameterInput("Env4", "Microlidian", (byte)71 /*MUSO_CC_CV_4*/, (byte)1);
            this->addParameterInput("Env5", "Microlidian", (byte)74 /*MUSO_CC_CV_5*/, (byte)1);
        }

        //uint16_t vid = 0x09e8, pid = 0x0028;
        uint16_t vid = 0x2E8A, pid = 0x000A;        // Seeed xiao RP2040 IDs
        uint16_t vid2 = 0x1337, pid2 = 0xBEEF;      // Microlidian custom IDs
        virtual uint32_t get_packed_id() override  { return (this->vid<<16 | this->pid); }
        virtual uint32_t get_packed_id2() { return (this->vid2<<16 | this->pid2); }
        virtual bool matches_identifiers(uint32_t packed_id) override {
            return packed_id==get_packed_id() || packed_id==get_packed_id2();
        }

        virtual const char *get_label() override {
            return "Microlidian";
        }
        virtual bool transmits_midi_notes() { return false; }
        virtual bool receives_midi_notes() { return true; }

        source_id_t source_id_2 = -1;

        virtual void setup_callbacks() override {
            //behaviour_apcmini = this;
            this->device->setHandleControlChange(microlidian_control_change);
            this->device->setHandleNoteOn(microlidian_note_on);
            this->device->setHandleNoteOff(microlidian_note_off);
            Serial.println(F("DeviceBehaviour_Microlidian#setup_callbacks()")); Serial_flush();
        };

        virtual void receive_note_on(uint8_t channel, uint8_t note, uint8_t velocity) override;
        virtual void receive_note_off(uint8_t channel, uint8_t note, uint8_t velocity) override;

        virtual void receive_control_change (uint8_t channel, uint8_t number, uint8_t value) override {
            this->update_parameter_inputs_cc(number, value, channel);
        }


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

        /*
        virtual void sendControlChange(byte cc_number, byte value, byte channel = 0) override {
            //if (this->debug) Serial.printf(F("%s#sendControlChange(cc=%i, value=%i, channel=%i)\n"), this->get_label(), cc_number, value, channel);
            // if we receive a value from another device, then update the proxy parameter, which will handle the actual sending
            // if modwheel should handle this event, handle it and return early
            if (ModwheelReceiver::process(cc_number, value, channel)) 
                return;

            DeviceBehaviourUltimateBase::sendControlChange(cc_number, value, channel);
        }

        FLASHMEM virtual LinkedList<FloatParameter*> *initialise_parameters() override {
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
            parameters->add(new MIDICCParameter<>("Glide",         this,   (byte)5,    (byte)1));
            parameters->add(new MIDICCParameter<>("Distortion",    this,   (byte)12,   (byte)1));
            parameters->add(new MIDICCParameter<>("Delay Dry/Wet", this,   (byte)13,   (byte)1));
            parameters->add(new MIDICCParameter<>("Delay Time",    this,   (byte)14,   (byte)1));
            parameters->add(new MIDICCParameter<>("Delay Feedback",this,   (byte)15,   (byte)1));
            parameters->add(new MIDICCParameter<>("Osc 1 Wave",    this,   (byte)16,   (byte)1));
            parameters->add(new MIDICCParameter<>("Osc 2 Wave",    this,   (byte)17,   (byte)1));
            parameters->add(new MIDICCParameter<>("Osc Mix",       this,   (byte)18,   (byte)1));
            //parameters->add(new MIDICCParameter((char*)"Spread",        this,   (byte)20,   (byte)1));
            parameters->add(new CraftSynthSpreadParameter("Spread", this));
            parameters->add(new MIDICCParameter<>("Filter Morph",  this,   (byte)33,   (byte)1));
            parameters->add(new MIDICCParameter<>("Filter Cutoff", this,   (byte)34,   (byte)1));
            parameters->add(new MIDICCParameter<>("Filter Reso",   this,   (byte)35,   (byte)1));

            //Serial.printf(F("Finished initialise_parameters() in %s\n"), this->get_label());

            return parameters;
        }*/

};

//void craftsynth_setOutputWrapper(MIDIOutputWrapper *);
extern DeviceBehaviour_Microlidian *behaviour_microlidian;

#endif
#endif