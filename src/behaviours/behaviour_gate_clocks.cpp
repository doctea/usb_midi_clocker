// cv clock and cv sequencer outputs (controlled by APCMini)

#include <Arduino.h>
#include "Config.h"
#include "storage.h"
#include "project.h"
#include "interfaces/interfaces.h"

#include "behaviours/behaviour_gate_clocks.h"

/*  get / manipulate clock multipliers */
float VirtualBehaviour_ClockGates::get_clock_multiplier(byte i) { 
  return clock_multiplier_values[current_state.clock_multiplier[i]];
}
void VirtualBehaviour_ClockGates::increase_clock_multiplier(byte i) {
  current_state.clock_multiplier[i]++;
  if (current_state.clock_multiplier[i]>=NUM_CLOCK_MULTIPLIER_VALUES)
    current_state.clock_multiplier[i] = 0;
  cv_out_clock_pin_off(i);
}
void VirtualBehaviour_ClockGates::decrease_clock_multiplier(byte i) {
  current_state.clock_multiplier[i]--;
  if (current_state.clock_multiplier[i]>=NUM_CLOCK_MULTIPLIER_VALUES) // ie has wrapped around to 255
    current_state.clock_multiplier[i] = NUM_CLOCK_MULTIPLIER_VALUES-1;
  cv_out_clock_pin_off(i);
}

// check if multiplier is set to 'never trigger'
bool VirtualBehaviour_ClockGates::is_clock_off(byte i) {
  return ((byte)get_clock_multiplier(i)>=CLOCK_MULTIPLIER_OFF);
}

// clock offsets offset
byte VirtualBehaviour_ClockGates::get_clock_delay(byte i) {
  //if (i>=NUM_CLOCKS) return 0;
  return current_state.clock_delay[i];
}
void VirtualBehaviour_ClockGates::decrease_clock_delay(byte clock_selected, byte amount) {
  current_state.clock_delay[clock_selected] -= amount; // wraps around to 255
  if (current_state.clock_delay[clock_selected]>CLOCK_DELAY_MAX)
    current_state.clock_delay[clock_selected] = CLOCK_DELAY_MAX;  
  Debug_print(F("Decreased selected clock delay to "));
  Debug_println(get_clock_delay(clock_selected));
}
void VirtualBehaviour_ClockGates::increase_clock_delay(byte clock_selected, byte amount) {
  current_state.clock_delay[clock_selected] += amount;
  if (current_state.clock_delay[clock_selected]>7)
    current_state.clock_delay[clock_selected] = 0;
  Debug_print(F("Increased selected clock delay to "));
  Debug_println(current_state.clock_delay[clock_selected]);
}

bool VirtualBehaviour_ClockGates::should_trigger_clock(unsigned long ticks, byte i, byte offset) {
  byte clock_delay = get_clock_delay(i);
  return !is_clock_off(i) && 
    is_bpm_on_multiplier(
      (long)ticks - (PPQN*clock_delay), 
      get_clock_multiplier(i),
      offset
    );
}

#ifndef SEPARATE_SEQUENCER_AND_CLOCKS
  void process_clocks(unsigned long ticks) {
    #if defined(ENABLE_SEQUENCER) || defined(ENABLE_CLOCKS)

      #ifdef PIN_CLOCK_RESET_PHRASE
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
  void VirtualBehaviour_ClockGates::process_clocks(unsigned long ticks) {
    // todo: investigate whether we can use uClock's shuffle to process these instead..
    #ifdef ENABLE_CLOCKS
      /*#ifdef PIN_CLOCK_RESET_PHRASE
        if (is_bpm_on_phrase(ticks)) {
          //Serial.printf("On phrase! %i\n", ticks);
          cv_out_clock_pin_on(PIN_CLOCK_RESET_PHRASE);
        } else if (is_bpm_on_phrase(ticks,duration)) {
          cv_out_clock_pin_off(PIN_CLOCK_RESET_PHRASE);
        }
      #endif
      #ifdef PIN_CLOCK_RESET_HALF_PHRASE
        if (is_bpm_on_half_phrase(ticks)) {
          //Serial.printf("On phrase! %i\n", ticks);
          cv_out_clock_pin_on(PIN_CLOCK_RESET_HALF_PHRASE);
        } else if (is_bpm_on_half_phrase(ticks,duration)) {
          cv_out_clock_pin_off(PIN_CLOCK_RESET_HALF_PHRASE);
        }
      #endif
      #ifdef PIN_CLOCK_RESET_BAR
        if (is_bpm_on_bar(ticks)) {
          //Serial.printf("On phrase! %i\n", ticks);
          cv_out_clock_pin_on(PIN_CLOCK_RESET_BAR);
        } else if (is_bpm_on_bar(ticks,duration)) {
          cv_out_clock_pin_off(PIN_CLOCK_RESET_BAR);
        }
      #endif
      #ifdef PIN_CLOCK_RESET_BEAT
        if (is_bpm_on_beat(ticks)) {
          //Serial.printf("On phrase! %i\n", ticks);
          cv_out_clock_pin_on(PIN_CLOCK_RESET_BEAT);
        } else if (is_bpm_on_beat(ticks,duration)) {
          cv_out_clock_pin_off(PIN_CLOCK_RESET_BEAT);
        }
      #endif*/

      for (unsigned int i = 0 ; i < NUM_CLOCKS ; i++) {
        bool should_go_high = false;
        bool should_go_low  = false;

        if (should_trigger_clock(ticks, i)) {
          should_go_high = true;
        } else if (should_trigger_clock(ticks, i, duration)) {
          should_go_low = true;
        }

        if (should_go_high)     cv_out_clock_pin_on (i);
        else if (should_go_low) cv_out_clock_pin_off(i);
      }
    #endif

    /*#ifdef ENABLE_SEQUENCER
      #ifdef ENABLE_MORE_CLOCKS
        for (unsigned int i = 0 ; i < 4 ; i++) {
          bool should_go_high = false;
          bool should_go_low  = false;

          float m;
          if (i<2) 
            m = clock_multiplier_values[i];
          else 
            m = clock_multiplier_values[i+4];
          
          if (is_bpm_on_multiplier(ticks, m)) {
            //Serial.printf("%3i: extra clock %i has multiplier %f - on!\n", ticks, i, m);
            should_go_high = true;
          } else if (is_bpm_on_multiplier(ticks, m, duration)) {
            //Serial.printf("%3i: extra clock %i has multiplier %f - off!\n", ticks, i, m);
            should_go_low = true;
          }

          // actually use the pins after the sequencer output pins for the extra clocks
          if (should_go_high)     cv_out_sequence_pin_on (NUM_SEQUENCES + i);
          else if (should_go_low) cv_out_sequence_pin_off(NUM_SEQUENCES + i);
        }
      #endif
    #endif*/
  }

#endif

VirtualBehaviour_ClockGates *behaviour_clock_gates = nullptr;