#define NUM_CLOCKS    4
#define NUM_SEQUENCES 4
#define NUM_STEPS     8

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

#if defined(__arm__) && defined(CORE_TEENSY)
// ...
void save_state(uint8_t preset_number, savestate *input) {
  Serial.println("save_state not implemented on teensy");
}
void load_state(uint8_t preset_number, savestate *output) {
  Serial.println("load_state not implemented on teensy");
}
#else
#include <EEPROM.h>

void save_state(uint8_t preset_number, savestate *input) {
  int eeAddress = 16 + (preset_number * sizeof(savestate));
  Serial.print(F("save_state at "));
  Serial.println(eeAddress);
  EEPROM.put(eeAddress, *input);
}

void load_state(uint8_t preset_number, savestate *output) {
  int eeAddress = 16 + (preset_number * sizeof(savestate));
  byte id = EEPROM.read(eeAddress);
  if (id==SAVE_ID_BYTE_V0 || id==SAVE_ID_BYTE_V1) {
    Serial.print(F("Found ID "));
    Serial.print(id);
    Serial.print(F(" at "));
    Serial.print(eeAddress);
    Serial.println(F(" - loading! :D"));
    EEPROM.get(eeAddress, *output);
    if (id==SAVE_ID_BYTE_V0) {  // v0 didn't have clock delays included, so zero them out
      output->clock_delay[0] = output->clock_delay[1] = output->clock_delay[2] = output->clock_delay[3] = 0;
    }
  } else {
    Serial.print(F("Didn't find a magic id byte at "));
    //Serial.print(0xD0);
    //Serial.print(F(" at "));
    Serial.print(eeAddress);
    Serial.print(F(" - found "));
    Serial.print(id);
    Serial.println(F(" instead :("));
  }    
}
#endif