#ifdef ENABLE_EUCLIDIAN
    #include "behaviours/behaviour_euclidianrhythms.h"

    VirtualBehaviour_EuclidianRhythms *behaviour_euclidianrhythms;

    #ifdef ENABLE_SHUFFLE
        #ifdef SHUFFLE_MULTIPLE_TRACKS
            void shuffled_track_callback(uint8_t track, uint32_t step) {
                //Serial.printf("at tick %i, shuffled_track_callback(%i, %i)\n", ticks, track, step);
                if (behaviour_euclidianrhythms!=nullptr) {
                        behaviour_euclidianrhythms->on_step_shuffled(track, step);
                }
            }
        #else
            void shuffled_callback(uint32_t step) {
                //Serial.printf("at tick %i, shuffled_callback(%i)\n", ticks, step);
                if (behaviour_euclidianrhythms!=nullptr) {
                    behaviour_euclidianrhythms->on_step_shuffled(0, step);
                }
            }
        #endif
    #endif
#endif
