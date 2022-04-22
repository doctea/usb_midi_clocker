#ifndef STORAGE__INCLUDED
#define STORAGE__INCLUDED

#include <Arduino.h>
#include "Config.h"

/*#define NUM_CLOCKS    4
#define NUM_SEQUENCES 4
#define NUM_STEPS     8*/

#define SAVE_ID_BYTE_V0 0xD0
#define SAVE_ID_BYTE_V1 0xD1

typedef struct savestate {
  uint8_t id = SAVE_ID_BYTE_V1;
  uint8_t size_clocks    = NUM_CLOCKS;
  uint8_t size_sequences = NUM_SEQUENCES;
  uint8_t size_steps     = NUM_STEPS;
  uint8_t clock_multiplier[NUM_CLOCKS] = { 5, 4, 3, 2 };
  uint8_t sequence_data[NUM_SEQUENCES][NUM_STEPS];
  uint8_t clock_delay[NUM_CLOCKS] = { 0, 0, 0, 0 };
} savestate;

void save_state(uint8_t preset_number, savestate *input);
void load_state(uint8_t preset_number, savestate *input);
void load_state_update();
void load_state_start(uint8_t preset_number, savestate *input);
void load_state_parse_line(String line, savestate *output);
void setup_storage();

extern savestate current_state;

#endif
