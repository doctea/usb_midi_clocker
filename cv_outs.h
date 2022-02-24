#include "bpm.h"

#define PIN_CLOCK_1   4
#define PIN_CLOCK_2   5
#define PIN_CLOCK_3   6
#define PIN_CLOCK_4   7

void update_cv_outs(unsigned long ticks) {
    // start bar (every fourth quarter note)
    if (is_bpm_on_bar(ticks)) {
      digitalWrite(PIN_CLOCK_1, HIGH);
    } else if (is_bpm_on_bar(ticks,duration)) {
      digitalWrite(PIN_CLOCK_1, LOW);
    }

    // every two quarter notes
    if (is_bpm_on_half_bar(ticks)) {
      digitalWrite(PIN_CLOCK_2, HIGH);
    } else if (is_bpm_on_half_bar(ticks,duration)) {
      digitalWrite(PIN_CLOCK_2, LOW);
    }

    // every quarter note
    if (is_bpm_on_beat(ticks)) {
      digitalWrite(PIN_CLOCK_3, HIGH);

    } else if (is_bpm_on_beat(ticks, duration)) {
      digitalWrite(PIN_CLOCK_3, LOW);
    }

    // every eighth note
    if (is_bpm_on_eighth(ticks)) { 
      digitalWrite(PIN_CLOCK_4, HIGH);
    } else if (is_bpm_on_eighth(ticks,duration)) {
      digitalWrite(PIN_CLOCK_4, LOW);
    }
}
