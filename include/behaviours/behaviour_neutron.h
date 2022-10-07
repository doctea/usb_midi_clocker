#ifndef BEHAVIOUR_NEUTRON__INCLUDED
#define BEHAVIOUR_NEUTRON__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviours/behaviour_base_serial.h"
#include "behaviours/behaviour_clocked.h"
#include "behaviours/behaviour_midibass.h"
#include "bpm.h"

class DeviceBehaviour_Neutron : public DeviceBehaviourSerialBase, public ClockedBehaviour, public MIDIBassBehaviour {
    using DeviceBehaviourSerialBase::has_input;
    using ClockedBehaviour::has_output;
    //using MIDIBassBehaviour::on_bar;
    public:
        virtual const char *get_label() override {
            return (char*)"Neutron";
        }

        /*#ifdef ENABLE_SCREEN
            virtual LinkedList<MenuItem *> make_menu_items() override;
        #endif*/

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

};

extern DeviceBehaviour_Neutron *behaviour_neutron;

#endif