#pragma once

#include "Config.h"

// MIDI MUSO CV12 4 voice poly mode (let the MUSO handle the voice allocation)

#ifdef ENABLE_MIDIMUSO_4PV

//#include "behaviours/behaviour_base.h";
//class DeviceBehaviourUltimateBase;

#include "behaviours/behaviour_base.h"
#include "behaviours/behaviour_base_serial.h"
#include "behaviours/behaviour_simplewrapper.h"

extern Behaviour_SimpleWrapper<DividedClockedBehaviour,DeviceBehaviourSerialBase> *behaviour_midimuso_4pv;

#endif