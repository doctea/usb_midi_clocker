#ifndef STORAGE__INCLUDED
#define STORAGE__INCLUDED
#include <Arduino.h>
#include "Config.h"

#include "SD.h"

namespace storage {

  #define FILEPATH_SEQUENCE_FORMAT          "project%i/sequences/sequence%i.txt"
  #define FILEPATH_PROJECT_SETTINGS_FORMAT  "project%i/project.txt"
  #define FILEPATH_LOOP_FORMAT              "project%i/loops/loop%i" //.txt"
  #define FILEPATH_CALIBRATION_FORMAT       "calibration_voltage_source_%i.txt"

  #define MAX_FILEPATH 255

  //#include <SD.h>
  //#include <SdFat.h>

  //#define NUM_STATES 8    // number of states that we can save..?  possibly should be per-project too

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
    uint8_t clock_multiplier[NUM_CLOCKS] = { 5, 4, 3, 2, 
      #if NUM_CLOCKS>4
      1,
      #endif
      #if NUM_CLOCKS>5
      3,
      #endif
      #if NUM_CLOCKS>6
      4,
      #endif
      #if NUM_CLOCKS>7
      7,
      #endif
    };
    uint8_t sequence_data[NUM_SEQUENCES][NUM_STEPS];
    uint8_t clock_delay[NUM_CLOCKS] = { 0, 0, 0, 0,
      #if NUM_CLOCKS>4
        1,
      #endif
      #if NUM_CLOCKS>5
        3,
      #endif
      #if NUM_CLOCKS>6
        5,
      #endif
      #if NUM_CLOCKS>7
        7
      #endif
    };
  } savestate;

  bool save_sequence(int project_number, uint8_t preset_number, savestate *input);
  bool load_sequence(int project_number, uint8_t preset_number, savestate *input);
  /*void load_state_update();
  void load_state_start(uint8_t preset_number, savestate *input);*/
  void load_sequence_parse_line(String line, savestate *output);
  FLASHMEM void setup_storage();

  void make_project_folders(int project_number);

  bool copy_file(char *src, char *dst);

  extern savestate current_state;
}
#endif
