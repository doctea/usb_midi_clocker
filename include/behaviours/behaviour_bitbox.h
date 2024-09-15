#pragma once

#include <Arduino.h>

#include "Config.h"

//#define ENABLE_BITBOX_DEDICATED // todo: remove this so that config is honoured 

#ifdef ENABLE_BITBOX_DEDICATED
    #include "behaviours/behaviour_base_serial.h"
    #include "behaviours/behaviour_clocked.h"
    #include "bpm.h"

    class DeviceBehaviour_Bitbox : virtual public DeviceBehaviourSerialBase, virtual public DividedClockedBehaviour {
        public:
            virtual const char *get_label() override {
                return (const char*)"BitBox";
            }

            bool already_initialised = false;
            FLASHMEM virtual LinkedList<FloatParameter*> *initialise_parameters() override {
                //Serial.printf(F("DeviceBehaviour_CraftSynth#initialise_parameters()..."));
                if (already_initialised)
                    return this->parameters;

                already_initialised = true;

                DeviceBehaviourSerialBase::initialise_parameters();
                DividedClockedBehaviour::initialise_parameters();
                //ModwheelReceiver::initialise_parameters();

                //Serial.println(F("\tAdding parameters..."));
                //parameters->clear();
                // todo: read these from a configuration file
                // todo: add the rest of the available parameters
                //this->add_parameters();
                #define NUM_MIDI_CC_PARAMETERS 10
                for (int i = 0 ; i < NUM_MIDI_CC_PARAMETERS ; i++) {
                    parameters->add(
                        new MIDICCParameter<>(
                            (String("Output ") + String((char)('A' + i))).c_str(),         
                            this,   
                            (byte)1+i,    
                            (byte)1,
                            true
                        )
                    );
                }

                return parameters;
            }
    };

    extern DeviceBehaviour_Bitbox *behaviour_bitbox;
#else
    #include "behaviours/behaviour_simplewrapper.h"
    extern Behaviour_SimpleWrapper<DeviceBehaviourSerialBase,DividedClockedBehaviour> *behaviour_bitbox;
#endif
