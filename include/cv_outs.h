#include "bpm.h"

#define PIN_CLOCK_START  30
#define PIN_CLOCK_1   30
#define PIN_CLOCK_2   31
#define PIN_CLOCK_3   32
#define PIN_CLOCK_4   33
#define PIN_CLOCK_5   36  // carefully avoiding TX8+RX8, as those are needed for the 8th MIDI in/outs
#define PIN_CLOCK_6   37
#define PIN_CLOCK_7   38
#define PIN_CLOCK_8   39

const byte clock_pin[NUM_CLOCKS] = {
    PIN_CLOCK_1, PIN_CLOCK_2, PIN_CLOCK_3, PIN_CLOCK_4,
    #if NUM_CLOCKS == 8
      PIN_CLOCK_5, PIN_CLOCK_6, PIN_CLOCK_7, PIN_CLOCK_8
    #endif
  };

#define CLOCK_MULTIPLIER_MIN  0.25
#define CLOCK_MULTIPLIER_MAX  16.0
#define CLOCK_DELAY_MAX 15

#define NUM_CLOCK_MULTIPLIER_VALUES 8
float clock_multiplier_values[NUM_CLOCK_MULTIPLIER_VALUES] = {
  0.25,     // 0
  0.5,      // 1
  1,        // 2
  2,        // 3
  4,        // 4
  8,        // 5
  16,       // 6
  32,       // 7
};
#define CLOCK_MULTIPLIER_OFF  32.0

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
bool is_clock_off(byte i) {
  return ((byte)get_clock_multiplier(i)>=CLOCK_MULTIPLIER_OFF);
}

byte get_clock_delay(byte i) {
  return current_state.clock_delay[i];
}
void decrease_clock_delay(byte clock_selected, byte amount = 1) {
  current_state.clock_delay[clock_selected] -= amount; // wraps around to 255
  if (current_state.clock_delay[clock_selected]>CLOCK_DELAY_MAX)
    current_state.clock_delay[clock_selected] = CLOCK_DELAY_MAX;  
  Serial.print(F("Decreased selected clock delay to "));
  Serial.println(get_clock_delay(clock_selected));
}
void increase_clock_delay(byte clock_selected, byte amount = 1) {
  current_state.clock_delay[clock_selected] += amount;
  if (current_state.clock_delay[clock_selected]>7)
    current_state.clock_delay[clock_selected] = 0;
  Serial.print(F("Increased selected clock delay to "));
  Serial.println(current_state.clock_delay[clock_selected]);
}

bool should_trigger_clock(unsigned long ticks, byte i, byte offset = 0) {
  byte clock_delay = get_clock_delay(i);
  return !is_clock_off(i) && 
    is_bpm_on_multiplier(
      (long)ticks - (PPQN*clock_delay), 
      get_clock_multiplier(i),
      offset
    );
}

#ifdef ENABLE_SEQUENCER
#include "sequencer.h"
#endif

void update_cv_outs(unsigned long ticks) {
#if defined(ENABLE_SEQUENCER) || defined(ENABLE_CLOCKS)
  // start bar (every fourth quarter note)
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
      should_trigger_clock(ticks, i)
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
      should_trigger_clock(ticks, i, duration)
#endif
    ) {
      should_go_low = true;
    }

    if (should_go_high)       digitalWrite(clock_pin[i], HIGH);
    else if (should_go_low)   digitalWrite(clock_pin[i], LOW);
  }
#endif

}
