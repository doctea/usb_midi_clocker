#include "bpm.h"

#define PIN_CLOCK_START  4
#define PIN_CLOCK_1   4
#define PIN_CLOCK_2   5
#define PIN_CLOCK_3   6
#define PIN_CLOCK_4   7

#define CLOCK_MULTIPLIER_MIN  0.25
#define CLOCK_MULTIPLIER_MAX  16.0
#define CLOCK_DELAY_MAX 15

#define NUM_CLOCK_MULTIPLIER_VALUES 7

float clock_multiplier_values[NUM_CLOCK_MULTIPLIER_VALUES] = {
  0.25,     // 0
  0.5,      // 1
  1,        // 2
  2,        // 3
  4,        // 4
  8,        // 5
  16        // 6
};

/*byte clock_multiplier[NUM_CLOCKS] = {
  5,
  4,
  3,
  2,
};*/
//byte *clock_multiplier[NUM_CLOCKS] = &current_state.clock_multiplier;

float get_clock_multiplier(byte i) {
  return clock_multiplier_values[current_state.clock_multiplier[i]];
}
void increase_clock_multiplier(byte i) {
  current_state.clock_multiplier[i]++;
  if (current_state.clock_multiplier[i]>=NUM_CLOCK_MULTIPLIER_VALUES)
    current_state.clock_multiplier[i] = 0;
}
void decrease_clock_multiplier(byte i) {
  current_state.clock_multiplier[i]--;
  if (current_state.clock_multiplier[i]>=NUM_CLOCK_MULTIPLIER_VALUES) // ie has wrapped around to 255
    current_state.clock_multiplier[i] = NUM_CLOCK_MULTIPLIER_VALUES-1;
}

#ifdef ENABLE_SEQUENCER
#include "sequencer.h"
#endif

/*byte clock_delay[NUM_CLOCKS] = {
  0, 0, 0, 0
};*/

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

  for (int i = 0 ; i < NUM_CLOCKS ; i++) {
    bool should_go_high = false;
    bool should_go_low  = false;

    if (
#ifdef ENABLE_SEQUENCER
      should_trigger_sequence(ticks, i)
#endif
#if defined(ENABLE_SEQUENCER) && defined(ENABLE_CLOCKS)
      ||
#endif
#ifdef ENABLE_CLOCKS
      is_bpm_on_multiplier(
        (long)ticks - (PPQN*current_state.clock_delay[i]), 
        get_clock_multiplier(i)
      )
#endif
    ) {
        should_go_high = true;
    } else if (
#ifdef ENABLE_SEQUENCER
      should_trigger_sequence(ticks, i, duration)
#endif
#if defined(ENABLE_SEQUENCER) && defined(ENABLE_CLOCKS)
      ||
#endif
#ifdef ENABLE_CLOCKS
      is_bpm_on_multiplier(
        (long)ticks - (PPQN*current_state.clock_delay[i]), 
        get_clock_multiplier(i), 
        duration                      //+((clock_delay[i]%8)*PPQN))
      )
#endif
    ) {
      should_go_low = true;
    }

    if (should_go_high) digitalWrite(PIN_CLOCK_START+i, HIGH);
    else if (should_go_low)  digitalWrite(PIN_CLOCK_START+i, LOW);
        
  }


}
