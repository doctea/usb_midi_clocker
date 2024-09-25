#ifndef BEHAVIOUR_BEHRINGER_EDGE__INCLUDED
#define BEHAVIOUR_BEHRINGER_EDGE__INCLUDED

#include <Arduino.h>

#include "Config.h"

#ifdef ENABLE_BEHRINGER_EDGE_SERIAL

#include "behaviours/behaviour_clocked.h"

class DeviceBehaviour_Bedge_Serial : virtual public DeviceBehaviourSerialBase,virtual  public DividedClockedBehaviour {
    using ClockedBehaviour::DeviceBehaviourUltimateBase::parameters;

    public:
        virtual const char *get_label() override {
            return "BEdge";
        }
        virtual bool transmits_midi_notes() { return false; }
};

#elif defined(ENABLE_BEHRINGER_EDGE_DEDICATED)

#include "behaviours/behaviour_base_usb.h"
#include "behaviours/behaviour_clocked.h"
//#include "behaviours/behaviour_modwheelreceiver.h"
#include "project.h"
#include "clock.h"

#include "usb/multi_usb_handlers.h"
class DeviceBehaviour_Bedge : virtual public DeviceBehaviourUSBBase, virtual public ClockedBehaviour {
    //using ClockedBehaviour::DeviceBehaviourUltimateBase;
    using ClockedBehaviour::DeviceBehaviourUltimateBase::parameters;
    
    public:
        //uint16_t vid = 0x09e8, pid = 0x0028;
        uint16_t vid = 0x1397, pid = 0x125A;
        virtual uint32_t get_packed_id() override { return (this->vid<<16 | this->pid); }

        virtual const char *get_label() override {
            return "BEdge";
        }
        virtual bool transmits_midi_notes() { return true; }


        /*            
            virtual void on_restart() override {
                Serial.println("behringer edge on_restart!");

                //if (is_connected()) {
                //    DeviceBehaviourUSBBase::device->sendSongPosition(0);
                //} else {
                //    Serial.println("\tisn't connected?");
                }
                //virtual void on_restart() override {
                    //if (!is_connected()) return;

                if (this->clock_enabled) {
                    this->sendRealTime(midi::Stop);
                    if (is_connected())
                        DeviceBehaviourUSBBase::device->sendSongPosition(0);
                    this->sendRealTime(midi::Start);
                    //this->sendRealTime(midi::Continue);
                    //this->sendNow();
                    //this->started = true;
                }
            //}
        }*/
};

//void craftsynth_setOutputWrapper(MIDIOutputWrapper *);
extern DeviceBehaviour_Bedge *behaviour_bedge;

#endif

#endif