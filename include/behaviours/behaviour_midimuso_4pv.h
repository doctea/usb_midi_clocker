#pragma once

#include "Config.h"

#ifdef ENABLE_MIDIMUSO_4PV

//#include "behaviours/behaviour_base.h";
//class DeviceBehaviourUltimateBase;

#include "behaviours/behaviour_base.h"
#include "behaviours/behaviour_base_serial.h"
#include "behaviours/behaviour_simplewrapper.h"

extern Behaviour_SimpleWrapper<DividedClockedBehaviour,DeviceBehaviourSerialBase> *behaviour_midimuso_4pv;

#endif