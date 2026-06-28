#include "Config.h"

#ifdef ENABLE_FLEXIARP

#include "behaviours/behaviour_flexiarp.h"

VirtualBehaviour_FlexiArp *behaviour_flexiarp = nullptr;

#ifdef ENABLE_SHUFFLE
    void flexiarp_shuffled_callback(uint32_t step, uint8_t track) {
        if (behaviour_flexiarp!=nullptr) {
            behaviour_flexiarp->on_step_shuffled(track, step);
        }
    }
#endif

#endif
