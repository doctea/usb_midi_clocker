#include "Config.h"

#ifdef ENABLE_TURINGMACHINE

#include "behaviours/behaviour_turingmachine.h"

VirtualBehaviour_TuringMachine *behaviour_turingmachine = nullptr;

#ifdef ENABLE_SHUFFLE
    void turingmachine_shuffled_callback(uint32_t step, uint8_t track) {
        if (behaviour_turingmachine!=nullptr) {
            behaviour_turingmachine->on_step_shuffled(track, step);
        }
    }
#endif

#endif