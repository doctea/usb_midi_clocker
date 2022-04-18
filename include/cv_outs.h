#ifndef CVOUT_INCLUDED
#define CVOUT_INCLUDED

#include <Arduino.h>
#include "Config.h"
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
extern float clock_multiplier_values[NUM_CLOCK_MULTIPLIER_VALUES];
extern int duration;

#define CLOCK_MULTIPLIER_OFF  32.0
 

float get_clock_multiplier(byte i);
void increase_clock_multiplier(byte i);
void decrease_clock_multiplier(byte i); 
bool is_clock_off(byte i);
byte get_clock_delay(byte i);
void decrease_clock_delay(byte clock_selected, byte amount = 1);
void increase_clock_delay(byte clock_selected, byte amount = 1);
bool should_trigger_clock(unsigned long ticks, byte i, byte offset = 0);
void update_cv_outs(unsigned long ticks);

#endif