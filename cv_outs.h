#include "bpm.h"


#define PIN_CLOCK_START  4
#define PIN_CLOCK_1   4
#define PIN_CLOCK_2   5
#define PIN_CLOCK_3   6
#define PIN_CLOCK_4   7

#define NUM_CLOCKS 4
#define CLOCK_MULTIPLIER_MIN  0.5
#define CLOCK_MULTIPLIER_MAX 16.0

float clock_multiplier[NUM_CLOCKS] = {
  4.0,
  2.0,
  1.0,
  0.5
};

#ifdef SEQUENCER
#include "sequencer.h"
#endif

int clock_delay[NUM_CLOCKS] = {
  0, 0, 0, 0
};

void update_cv_outs(unsigned long ticks) {
  // start bar (every fourth quarter note)
  /*if (is_bpm_on_bar(ticks)) {
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
  }*/

#ifdef SEQUENCER
  trigger_sequence(ticks);
#else
  for (int i = 0 ; i < NUM_CLOCKS ; i++) {
    if (is_bpm_on_multiplier(
      (long)ticks - (PPQN*clock_delay[i]), 
      clock_multiplier[i]
    )) {
      digitalWrite(PIN_CLOCK_START+i, HIGH);
    } else if (is_bpm_on_multiplier(
      (long)ticks - (PPQN*clock_delay[i]), 
      clock_multiplier[i], 
      duration                      //+((clock_delay[i]%8)*PPQN))
    )) {
      digitalWrite(PIN_CLOCK_START+i, LOW);
    }
  }
#endif

}
