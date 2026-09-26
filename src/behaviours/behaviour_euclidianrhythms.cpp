#ifdef ENABLE_EUCLIDIAN
    #include "behaviours/behaviour_euclidianrhythms.h"

    VirtualBehaviour_EuclidianRhythms *behaviour_euclidianrhythms;

    #ifdef ENABLE_SHUFFLE
        void shuffled_callback(uint32_t step, uint8_t track) {
            Serial.printf("\n&&&&>>>at tick %i, shuffled_track_callback(%i, %i) calling on_step_shuffled\n", ticks, track, step);
            if (behaviour_euclidianrhythms!=nullptr) {
                behaviour_euclidianrhythms->on_step_shuffled(track, step);
            }
            Serial.printf("\n&&&&<<<at tick %i, shuffled_track_callback(%i, %i) finished\n\n", ticks, track, step);
        }
    #endif

    void debug_euclidian_lock() {
        behaviour_euclidianrhythms->debug_lock();
    }
    void debug_euclidian_shuffle(uint8_t index) {
        behaviour_euclidianrhythms->debug_shuffle(index);
    }
    void debug_euclidian_simples() {
        behaviour_euclidianrhythms->debug_simples();
    }

#endif
