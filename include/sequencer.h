/*#ifndef INCLUDED_SEQUENCER
#define INCLUDED_SEQUENCER

#include <Arduino.h>
#include "Config.h"
#include "bpm.h"

#define NUM_STEPS     8

#define SEQUENCER_MAX_VALUE 6

//#ifdef ENABLE_SEQUENCER
#if defined(PROTOTYPE)
const byte cv_out_sequence_pin[NUM_SEQUENCES] = {
  PIN_SEQUENCE_1, PIN_SEQUENCE_2, PIN_SEQUENCE_3, PIN_SEQUENCE_4
};
#endif
//#endif

void sequencer_clear_pattern();
void sequencer_clear_row(byte row);

void init_sequence();
byte read_sequence(byte row, byte col);
void write_sequence(byte row, byte col, byte value);
void sequencer_press(byte row, byte col, bool shift = false);
bool should_trigger_sequence(unsigned long ticks, byte sequence, int offset = 0);

void cv_out_sequence_pin_on(byte i);
void cv_out_sequence_pin_off(byte i);

void raw_write_pin(int,int);

#endif
*/