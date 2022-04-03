#include <EEPROM.h>

#define NUM_CLOCKS    4
#define NUM_SEQUENCES 4
#define NUM_STEPS     8

#define SAVE_ID_BYTE_V0 0xD0
#define SAVE_ID_BYTE_V1 0xD1

typedef struct savestate {
  byte id = SAVE_ID_BYTE_V1;
  byte size_clocks    = NUM_CLOCKS;
  byte size_sequences = NUM_SEQUENCES;
  byte size_steps     = NUM_STEPS;
  byte clock_multiplier[NUM_CLOCKS] = { 5, 4, 3, 2 };
  byte sequence_data[NUM_SEQUENCES][NUM_STEPS];
  byte clock_delay[NUM_CLOCKS] = { 0, 0, 0, 0 };
};

savestate current_state;

void save_state(byte preset_number, savestate *input) {
  int eeAddress = 16 + (preset_number * sizeof(savestate));
  Serial.print(F("save_state at "));
  Serial.println(eeAddress);
  EEPROM.put(eeAddress, *input);
}

void load_state(byte preset_number, savestate *output) {
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
