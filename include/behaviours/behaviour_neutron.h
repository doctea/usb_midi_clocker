#ifndef BEHAVIOUR_NEUTRON__INCLUDED
#define BEHAVIOUR_NEUTRON__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviours/behaviour_base_serial.h"
#include "behaviours/behaviour_clocked.h"
#include "behaviours/behaviour_midibass.h"
#include "behaviour_modwheelreceiver.h"
#include "bpm.h"

#include "parameters/MIDICCParameter.h"

class DeviceBehaviour_Neutron : public DeviceBehaviourSerialBase, public ClockedBehaviour, public MIDIBassBehaviour, public ModwheelReceiver {
    using DeviceBehaviourSerialBase::has_input;
    //using DeviceBehaviourSerialBase::sendControlChange;
    using ClockedBehaviour::has_output;

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
            //if (cc_number==this->modwheel_proxy->cc_number)
            if (!ModwheelReceiver::process(cc_number, value, channel))
                DeviceBehaviourUltimateBase::sendControlChange(cc_number, value, channel);
        }

        virtual LinkedList<DoubleParameter*> *initialise_parameters() override {
            //Serial.printf(F("DeviceBehaviour_CraftSynth#initialise_parameters()..."));
            static bool already_initialised = false;
            if (already_initialised)
                return this->parameters;

            //Serial.println(F("\tcalling DeviceBehaviourUSBBase::initialise_parameters()")); 
            DeviceBehaviourSerialBase::initialise_parameters();
            //Serial.println(F("\tcalling ClockedBehaviour::initialise_parameters()"));
            ClockedBehaviour::initialise_parameters();
            MIDIBassBehaviour::initialise_parameters();

            ModwheelReceiver::initialise_parameters();

            return parameters;
        }

};

extern DeviceBehaviour_Neutron *behaviour_neutron;

#endif