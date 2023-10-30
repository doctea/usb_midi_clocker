#include "Config.h"

#ifdef ENABLE_MIDIMUSO_4PV

#include "behaviours/behaviour_base.h"
#include "behaviours/behaviour_base_serial.h"
#include "behaviours/behaviour_simplewrapper.h"

Behaviour_SimpleWrapper<DividedClockedBehaviour,DeviceBehaviourSerialBase> *behaviour_midimuso_4pv = nullptr; //new DeviceBehaviour_MIDIMuso();

#endif