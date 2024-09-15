#pragma once

#include <Arduino.h>

#include "Config.h"
#ifdef ENABLE_BITBOX_DEDICATED
    #include "behaviours/behaviour_base_serial.h"
    #include "behaviours/behaviour_clocked.h"
    #include "bpm.h"

    class DeviceBehaviour_Bitbox : virtual public DeviceBehaviourSerialBase, virtual public DividedClockedBehaviour {
        public:
            virtual const char *get_label() override {
                return (const char*)"BitBox";
            }

    };

    extern DeviceBehaviour_Bitbox *behaviour_bitbox;
#else
    #include "behaviours/behaviour_simplewrapper.h"
    extern Behaviour_SimpleWrapper<DeviceBehaviourSerialBase,DividedClockedBehaviour> *behaviour_bitbox;
#endif
