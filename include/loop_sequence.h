/*#include <Arduino.h>

#include "bpm.h"

#define LOOP_LENGTH (PPQN*4*4)

#define NUM_POLYPHONY   16

struct time_status {
    byte pitch = 0;
    byte velocity = 0;
};

class LoopSequence {

    public:
        time_status data[LOOP_LENGTH][NUM_POLYPHONY];

        void store_note(uint16_t time, byte pitch, byte velocity) {
            // find first place we can store this note
            for (int i = 0 ; NUM_POLYPHONY ; i++) {
                if (data[time][i].velocity==0) {

                }
            }
        }

        void record_note(uint16_t time, byte pitch, byte velocity) {

        }
};*/