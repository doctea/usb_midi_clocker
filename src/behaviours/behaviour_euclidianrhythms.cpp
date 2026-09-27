#ifdef ENABLE_EUCLIDIAN
    #include "behaviours/behaviour_euclidianrhythms.h"

    VirtualBehaviour_EuclidianRhythms *behaviour_euclidianrhythms;

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
