#ifndef INCLUDED_SEQUENCER
#define INCLUDED_SEQUENCER

#include <Arduino.h>
#include "Config.h"
#include "bpm.h"

#define NUM_SEQUENCES 4
#define NUM_STEPS     8

#define SEQUENCER_MAX_VALUE 3

inline int beat_number_from_ticks(signed long ticks) {
  return (ticks / PPQN) % BEATS_PER_BAR;
}
inline int step_number_from_ticks(signed long ticks) {
  return (ticks / (PPQN)) % NUM_STEPS;
}

void init_sequence();
byte read_sequence(byte row, byte col);
void write_sequence(byte row, byte col, byte value);
void sequencer_press(byte row, byte col, bool shift = false);
bool should_trigger_sequence(unsigned long ticks, byte sequence, int offset = 0);

#endif