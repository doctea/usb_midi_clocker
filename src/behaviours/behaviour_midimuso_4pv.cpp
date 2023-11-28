#include "Config.h"

#ifdef ENABLE_MIDIMUSO_4PV

#include "behaviours/behaviour_base.h"
#include "behaviours/behaviour_base_serial.h"
#include "behaviours/behaviour_simplewrapper.h"

Behaviour_SimpleWrapper<DividedClockedBehaviour,DeviceBehaviourSerialBase> *behaviour_midimuso_4pv = nullptr; //new DeviceBehaviour_MIDIMuso();

// midimuso's idea of 1/v oct is 1V = A.
// everything else's is 1V = C.
// so -- when we play an C, we actually need to play an A.
// so we need to subtract 3 semitones.

#endif