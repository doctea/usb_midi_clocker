#include "Config.h"

#ifdef ENABLE_TD3
    #include "behaviours/behaviour_td3.h"

    Behaviour_SimpleWrapperUSB<DividedClockedBehaviour> *behaviour_td3 = nullptr;
#endif