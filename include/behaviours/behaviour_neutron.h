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

class DeviceBehaviour_Neutron : virtual public DeviceBehaviourSerialBase, public ClockedBehaviour, public MIDIBassBehaviour, public ModwheelReceiver {
    //using DeviceBehaviourSerialBase::sendControlChange; 
    using DeviceBehaviourSerialBase::receives_midi_notes;
    using DeviceBehaviourSerialBase::transmits_midi_notes;
    //using ClockedBehaviour::transmits_midi_notes;
    //using MIDIBassBehaviour::on_bar;

    using MIDIBassBehaviour::sendNoteOff;
    using MIDIBassBehaviour::sendNoteOn;
    using MIDIBassBehaviour::killCurrentNote;
    
    public:
        virtual const char *get_label() override {
            return (char*)"Neutron";
        }

        virtual bool transmits_midi_notes() override {
            return true;
        }

        virtual void on_bar(int bar) override {
            MIDIBassBehaviour::on_bar(bar);
            ClockedBehaviour::on_bar(bar);
        }

        #ifdef ENABLE_SCREEN
            FLASHMEM
            virtual LinkedList<MenuItem*> *make_menu_items() override {
                DeviceBehaviourSerialBase::make_menu_items();
                ClockedBehaviour::make_menu_items();
                return MIDIBassBehaviour::make_menu_items();
            }
        #endif

        virtual void sendControlChange(byte cc_number, byte value, byte channel = 0) override {
            //Serial.printf("behaviour_neutron sendControlChange(cc=%i, value=%i, channel=%i)\n", cc_number, value, channel);
            // if we receive a value from another device, then update the proxy parameter, which will handle the actual sending
            //if (cc_number==this->modwheel_proxy->cc_number)
            if (!ModwheelReceiver::process(cc_number, value, channel))
                DeviceBehaviourUltimateBase::sendControlChange(cc_number, value, channel);
        }

        virtual void on_tick(uint32_t ticks) override {
            MIDIBassBehaviour::on_tick(ticks);
            ClockedBehaviour::on_tick(ticks);
        }

        virtual void setup_saveable_parameters() override {
            DeviceBehaviourUltimateBase::setup_saveable_parameters();
            ClockedBehaviour::setup_saveable_parameters();
            MIDIBassBehaviour::setup_saveable_parameters();
        }

        bool already_initialised = false;
        FLASHMEM
        virtual LinkedList<FloatParameter*> *initialise_parameters() override {
            //Serial.printf(F("DeviceBehaviour_CraftSynth#initialise_parameters()..."));
            //static bool already_initialised = false;
            if (already_initialised && this->parameters!=nullptr)
                return this->parameters;

            //Serial.println(F("\tcalling DeviceBehaviourUSBBase::initialise_parameters()")); 
            DeviceBehaviourSerialBase::initialise_parameters();
            //Serial.println(F("\tcalling ClockedBehaviour::initialise_parameters()"));
            ClockedBehaviour::initialise_parameters();
            MIDIBassBehaviour::initialise_parameters();

            ModwheelReceiver::initialise_parameters();

            already_initialised = true;

            return parameters;
        }

};

extern DeviceBehaviour_Neutron *behaviour_neutron;

#endif