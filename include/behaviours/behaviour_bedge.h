#ifndef BEHAVIOUR_BEHRINGER_EDGE__INCLUDED
#define BEHAVIOUR_BEHRINGER_EDGE__INCLUDED

#include <Arduino.h>

#include "Config.h"

#ifdef ENABLE_BEHRINGER_EDGE_DEDICATED

#include "behaviours/behaviour_base_usb.h"
#include "behaviours/behaviour_clocked.h"
//#include "behaviours/behaviour_modwheelreceiver.h"
#include "project.h"
#include "clock.h"

#include "multi_usb_handlers.h"
class DeviceBehaviour_Bedge : public DeviceBehaviourUSBBase, public ClockedBehaviour {
    //using ClockedBehaviour::DeviceBehaviourUltimateBase;
    using ClockedBehaviour::DeviceBehaviourUltimateBase::parameters;
    
    public:
        //uint16_t vid = 0x09e8, pid = 0x0028;
        uint16_t vid = 0x1397, pid = 0x125A;
        virtual uint32_t get_packed_id() override { return (this->vid<<16 | this->pid); }

        virtual const char *get_label() override {
            return "BEdge";
        }
        virtual bool has_output() { return true; }
};

//void craftsynth_setOutputWrapper(MIDIOutputWrapper *);
extern DeviceBehaviour_Bedge *behaviour_bedge;

#endif

#endif