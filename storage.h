#include <EEPROM.h>

#define NUM_CLOCKS    4
#define NUM_SEQUENCES 4
#define NUM_STEPS     8

typedef struct savestate {
  byte id = 0xD0;
  byte size_clocks = NUM_CLOCKS;
  byte size_sequences = NUM_SEQUENCES;
  byte size_steps = NUM_STEPS;
  byte clock_multiplier[NUM_CLOCKS] = { 5, 4, 3, 2 };
  byte sequence_data[NUM_SEQUENCES][NUM_STEPS];
  byte clock_delay[NUM_CLOCKS] = { 0, 0, 0, 0 };
};

savestate current_state;

void save_state(byte preset_number, savestate *input) {
  int eeAddress = 16 + (preset_number * sizeof(savestate));
  Serial.print("save_state at ");
  Serial.println(eeAddress);
  EEPROM.put(eeAddress, *input);
}

void load_state(byte preset_number, savestate *output) {
  int eeAddress = 16 + (preset_number * sizeof(savestate));
  byte id = EEPROM.read(eeAddress);
  if (id!=0xD0) {
    Serial.print(F("Didn't find magic id "));
    Serial.print(0xD0);
    Serial.print(F(" at "));
    Serial.print(eeAddress);
    Serial.print(F(" - found "));
    Serial.print(id);
    Serial.println(F(" instead :("));
  } else {
    Serial.print(F("Found ID at "));
    Serial.print(eeAddress);
    Serial.println(F(" - loading!"));
    EEPROM.get(eeAddress, *output);
  }    
}
