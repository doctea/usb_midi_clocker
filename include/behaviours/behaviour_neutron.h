#ifndef BEHAVIOUR_NEUTRON__INCLUDED
#define BEHAVIOUR_NEUTRON__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviours/behaviour_base_serial.h"
#include "behaviours/behaviour_clocked.h"
#include "behaviours/behaviour_midibass.h"
#include "bpm.h"

#include "parameters/MIDICCParameter.h"

class DeviceBehaviour_Neutron : public DeviceBehaviourSerialBase, public ClockedBehaviour, public MIDIBassBehaviour {
    using DeviceBehaviourSerialBase::has_input;
    //using DeviceBehaviourSerialBase::sendControlChange;
    using ClockedBehaviour::has_output;

    MIDICCProxyParameter *modwheel_proxy = nullptr;

    //using MIDIBassBehaviour::on_bar;
    public:
        virtual const char *get_label() override {
            return (char*)"Neutron";
        }

        virtual void on_bar(int bar) override {
            MIDIBassBehaviour::on_bar(bar);
            ClockedBehaviour::on_bar(bar);
        }

        #ifdef ENABLE_SCREEN
            virtual LinkedList<MenuItem*> *make_menu_items() override {
                ClockedBehaviour::make_menu_items();
                return MIDIBassBehaviour::make_menu_items();
            }
        #endif

        virtual void sendControlChange(byte cc_number, byte value, byte channel = 0) {
            Serial.printf("behaviour_neutron sendControlChange(cc=%i, value=%i, channel=%i)\n", cc_number, value, channel);
            // if we receive a value from another device, then update the proxy parameter, which will handle the actual sending
            if (cc_number==this->modwheel_proxy->cc_number)
                this->modwheel_proxy->updateValueFromData(value);
            else
                DeviceBehaviourUltimateBase::sendControlChange(cc_number, value, channel);
        }

        FLASHMEM virtual LinkedList<DoubleParameter*> *initialise_parameters() override {
            //Serial.printf(F("DeviceBehaviour_CraftSynth#initialise_parameters()..."));
            static bool already_initialised = false;
            if (already_initialised)
                return this->parameters;

            //Serial.println(F("\tcalling DeviceBehaviourUSBBase::initialise_parameters()")); 
            DeviceBehaviourSerialBase::initialise_parameters();
            //Serial.println(F("\tcalling ClockedBehaviour::initialise_parameters()"));
            ClockedBehaviour::initialise_parameters();
            MIDIBassBehaviour::initialise_parameters();

            //Serial.println(F("\tAdding parameters..."));
            //parameters->clear();
            // todo: read these from a file
            //this->add_parameters();
            this->modwheel_proxy = new MIDICCProxyParameter((char*)"Modwheel",      this,   (byte)1 ,   (byte)midi_matrix_manager->getDefaultChannelForTargetId(this->target_id));
            parameters->add(this->modwheel_proxy); //new MIDICCParameter((char*)"Modwheel",      this,   (byte)1 ,   (byte)0)); //,    (byte)this->channel));
            /*parameters->add(new MIDICCParameter((char*)"Distortion",    this,   (byte)12,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Delay Dry/Wet", this,   (byte)13,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Delay Time",    this,   (byte)14,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Delay Feedback",this,   (byte)15,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Osc 1 Wave",    this,   (byte)16,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Osc 2 Wave",    this,   (byte)17,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Osc Mix",       this,   (byte)18,   (byte)1));
            //parameters->add(new MIDICCParameter((char*)"Spread",        this,   (byte)20,   (byte)1));
            parameters->add(new CraftSynthSpreadParameter((char*)"Spread", this));
            parameters->add(new MIDICCParameter((char*)"Filter Morph",  this,   (byte)33,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Filter Cutoff", this,   (byte)34,   (byte)1));
            parameters->add(new MIDICCParameter((char*)"Filter Reso",   this,   (byte)35,   (byte)1));*/

            //Serial.printf(F("Finished initialise_parameters() in %s\n"), this->get_label());

            return parameters;
        }

};

extern DeviceBehaviour_Neutron *behaviour_neutron;

#endif