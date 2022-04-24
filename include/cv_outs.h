#ifndef CVOUT_INCLUDED
#define CVOUT_INCLUDED

#include <Arduino.h>
#include "Config.h"
#include "bpm.h"

#ifdef ENABLE_CLOCKS
const byte cv_out_clock_pin[NUM_CLOCKS] = {
    PIN_CLOCK_1, PIN_CLOCK_2, PIN_CLOCK_3, PIN_CLOCK_4,
    #if NUM_CLOCKS > 4
      PIN_CLOCK_5, 
    #endif
    #if NUM_CLOCKS > 5
      PIN_CLOCK_6, 
    #endif
    #if NUM_CLOCKS > 6
      PIN_CLOCK_7,
    #endif
    #if NUM_CLOCKS > 7
      PIN_CLOCK_8
    #endif
};
#endif

#define CLOCK_MULTIPLIER_MIN  0.25
#define CLOCK_MULTIPLIER_MAX  16.0
#define CLOCK_DELAY_MAX 15

#define NUM_CLOCK_MULTIPLIER_VALUES 8
#define CLOCK_MULTIPLIER_OFF        32.0  // if clock multipler is set to this value then actually turn it off completely
extern float clock_multiplier_values[NUM_CLOCK_MULTIPLIER_VALUES];
extern int duration;

 
void setup_cv();
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