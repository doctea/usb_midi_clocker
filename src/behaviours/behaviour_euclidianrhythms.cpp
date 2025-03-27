#include "behaviours/behaviour_euclidianrhythms.h"

#ifdef ENABLE_EUCLIDIAN
    VirtualBehaviour_EuclidianRhythms *behaviour_euclidianrhythms;
#endif

void shuffled_track_callback(uint8_t track, uint32_t step) {
    //Serial.printf("at tick %i, shuffled_track_callback(%i, %i)\n", ticks, track, step);
    if (behaviour_euclidianrhythms!=nullptr) {
            behaviour_euclidianrhythms->on_step_shuffled(track, step);
    }
}