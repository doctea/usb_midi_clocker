// cv clock and cv sequencer outputs (controlled by APCMini)

#include <Arduino.h>
#include "Config.h"
#include "storage.h"
#include "cv_outs.h"
#include "sequencer.h"
#include "project.h"
#include "interfaces/interfaces.h"

//extern storage::savestate current_state;

using namespace storage;

int duration = PPQN/8;

float clock_multiplier_values[NUM_CLOCK_MULTIPLIER_VALUES] = {
  0.25,     // 0 - 16th notes
  0.5,      // 1 - 8th notes
  1,        // 2 - quarter notes
  2,        // 3 - every two beats
  4,        // 4 - every four beats    (1 bar)
  8,        // 5 - every eight beats   (2 bars)
  16,       // 6 - every sixteen beats (4 bars, 1 phrase)
  32,       // 7 - every thirty two beats (8 bars, 2 phrases)
  64,       // 8 - clock is completely off (would be every 64 beats, except we don't have enough colours to represent this on the apcmini display)
};
#define CLOCK_MULTIPLIER_OFF        64.0  // if clock multipler is set to this value, then actually turn it off completely

void cv_out_clock_pin_off(byte i) {
  set_clock_gate(i, LOW);
}
void cv_out_clock_pin_on(byte i) {
  set_clock_gate(i, HIGH);
}

/*  get / manipulate clock multipliers */
float get_clock_multiplier(byte i) { 
  return clock_multiplier_values[current_state.clock_multiplier[i]];
}
void increase_clock_multiplier(byte i) {
  current_state.clock_multiplier[i]++;
  if (current_state.clock_multiplier[i]>=NUM_CLOCK_MULTIPLIER_VALUES)
    current_state.clock_multiplier[i] = 0;
  cv_out_clock_pin_off(i);
}
void decrease_clock_multiplier(byte i) {
  current_state.clock_multiplier[i]--;
  if (current_state.clock_multiplier[i]>=NUM_CLOCK_MULTIPLIER_VALUES) // ie has wrapped around to 255
    current_state.clock_multiplier[i] = NUM_CLOCK_MULTIPLIER_VALUES-1;
  cv_out_clock_pin_off(i);
}

// check if multiplier is set to 'never trigger'
bool is_clock_off(byte i) {
  return ((byte)get_clock_multiplier(i)>=CLOCK_MULTIPLIER_OFF);
}

// clock offsets offset
byte get_clock_delay(byte i) {
  return current_state.clock_delay[i];
}
void decrease_clock_delay(byte clock_selected, byte amount) {
  current_state.clock_delay[clock_selected] -= amount; // wraps around to 255
  if (current_state.clock_delay[clock_selected]>CLOCK_DELAY_MAX)
    current_state.clock_delay[clock_selected] = CLOCK_DELAY_MAX;  
  Debug_print(F("Decreased selected clock delay to "));
  Debug_println(get_clock_delay(clock_selected));
}
void increase_clock_delay(byte clock_selected, byte amount) {
  current_state.clock_delay[clock_selected] += amount;
  if (current_state.clock_delay[clock_selected]>7)
    current_state.clock_delay[clock_selected] = 0;
  Debug_print(F("Increased selected clock delay to "));
  Debug_println(current_state.clock_delay[clock_selected]);
}

bool should_trigger_clock(unsigned long ticks, byte i, byte offset) {
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

#ifndef SEPARATE_SEQUENCER_AND_CLOCKS
  void update_cv_outs(unsigned long ticks) {
    #if defined(ENABLE_SEQUENCER) || defined(ENABLE_CLOCKS)

      #ifdef PIN_CLOCK_RESET
        if (is_bpm_on_phrase(ticks)) {
          Serial.printf("On phrase! %i\n", ticks);
          cv_out_clock_pin_on(PIN_CLOCK_RESET);
        } else if (is_bpm_on_phrase(ticks,duration)) {
          cv_out_clock_pin_off(PIN_CLOCK_RESET);
        }
      #endif
      
      for (unsigned int i = 0 ; i < NUM_CLOCKS ; i++) {
        bool should_go_high = false;
        bool should_go_low  = false;

        if (
          #ifdef ENABLE_SEQUENCER
                (i < NUM_SEQUENCES && should_trigger_sequence(ticks, i))
          #endif
          #if defined(ENABLE_SEQUENCER) && defined(ENABLE_CLOCKS)
                ||
          #endif
          #ifdef ENABLE_CLOCKS
                (i < NUM_CLOCKS && should_trigger_clock(ticks, i))
          #endif
        ) {
            should_go_high = true;
        } else if (
          #ifdef ENABLE_SEQUENCER
                (i < NUM_SEQUENCES && should_trigger_sequence(ticks, i, duration))
          #endif
          #if defined(ENABLE_SEQUENCER) && defined(ENABLE_CLOCKS)
                ||
          #endif
          #ifdef ENABLE_CLOCKS
                (i < NUM_CLOCKS && should_trigger_clock(ticks, i, duration))
          #endif
        ) {
          should_go_low = true;
        }

        if (should_go_high)       cv_out_clock_pin_on(i);
        else if (should_go_low)   cv_out_clock_pin_off(i);
      }
    #endif

  }
#else
  void update_cv_outs(unsigned long ticks) {
    #ifdef PIN_CLOCK_RESET
      if (is_bpm_on_phrase(ticks)) {
        Serial.printf("On phrase! %i\n", ticks);
        cv_out_clock_pin_on(PIN_CLOCK_RESET);
      } else if (is_bpm_on_phrase(ticks,duration)) {
        cv_out_clock_pin_off(PIN_CLOCK_RESET);
      }
    #endif
        
    #ifdef ENABLE_SEQUENCER
      for (unsigned int i = 0 ; i < NUM_SEQUENCES ; i++) {
        bool should_go_high = false;
        bool should_go_low  = false;

        if (should_trigger_sequence(ticks, i)) {
          should_go_high = true;
        } else if (should_trigger_sequence(ticks, i, duration)) {
          should_go_low = true;
        }

        if (should_go_high)     cv_out_sequence_pin_on(i);
        else if (should_go_low) cv_out_sequence_pin_off(i);
      }
    #endif
    #ifdef ENABLE_CLOCKS
      for (unsigned int i = 0 ; i < NUM_CLOCKS ; i++) {
        bool should_go_high = false;
        bool should_go_low  = false;

        if (should_trigger_clock(ticks, i)) {
          should_go_high = true;
        } else if (should_trigger_clock(ticks, i, duration)) {
          should_go_low = true;
        }

        if (should_go_high)     cv_out_clock_pin_on(i);
        else if (should_go_low) cv_out_clock_pin_off(i);
      }
    #endif
  }

#endif
