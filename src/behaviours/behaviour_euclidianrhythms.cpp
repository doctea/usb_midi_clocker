#ifdef ENABLE_EUCLIDIAN
    #include "behaviours/behaviour_euclidianrhythms.h"

    VirtualBehaviour_EuclidianRhythms *behaviour_euclidianrhythms;

    #ifdef ENABLE_SHUFFLE
        void shuffled_callback(uint32_t step, uint8_t track) {
            //Serial.printf("at tick %i, shuffled_track_callback(%i, %i)\n", ticks, track, step);
            if (behaviour_euclidianrhythms!=nullptr) {
                behaviour_euclidianrhythms->on_step_shuffled(track, step);
            }
        }
    #endif
#endif
