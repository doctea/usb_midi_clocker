#ifndef INCLUDED_SEQUENCER
#define INCLUDED_SEQUENCER

#include <Arduino.h>
#include "Config.h"
#include "bpm.h"

#define NUM_STEPS     8

#define SEQUENCER_MAX_VALUE 5

//#ifdef ENABLE_SEQUENCER
#if defined(PROTOTYPE)
const byte cv_out_sequence_pin[NUM_SEQUENCES] = {
  PIN_SEQUENCE_1, PIN_SEQUENCE_2, PIN_SEQUENCE_3, PIN_SEQUENCE_4
};
#endif
//#endif

inline int beat_number_from_ticks(signed long ticks) {  // TODO: move this into midihelpers + make it a macro?
  return (ticks / PPQN) % BEATS_PER_BAR;
}
inline int step_number_from_ticks(signed long ticks) {  // TODO: move this into midihelpers + make it a macro?
  return (ticks / (PPQN)) % NUM_STEPS;
}

void init_sequence();
byte read_sequence(byte row, byte col);
void write_sequence(byte row, byte col, byte value);
void sequencer_press(byte row, byte col, bool shift = false);
bool should_trigger_sequence(unsigned long ticks, byte sequence, int offset = 0);

void cv_out_sequence_pin_on(byte i);
void cv_out_sequence_pin_off(byte i);

void raw_write_pin(int,int);

#endif
