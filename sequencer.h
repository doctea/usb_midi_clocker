// work in progress -- doesn't work yet
#ifndef INCLUDED_SEQUENCER
#define INCLUDED_SEQUENCER

#define NUM_SEQUENCES 4
#define NUM_STEPS     8

byte sequence_data[NUM_SEQUENCES][NUM_STEPS] = {
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
  { 1, 0, 1, 0 },
  { 0, 0, 0, 1 }
};

void init_sequence() {
  /*for (int seq = 0 ; seq < NUM_SEQUENCES ; seq++) {
    for (int ste = 0 ; ste < NUM_STEPS ; ste++) {
      if (ste%(byte)clock_multiplier[seq]==0 || clock_multiplier[seq]<1.0) {
        sequence[seq][ste] = 1;
      } else {
        sequence[seq][ste] = 0;
      }
    }
  }*/
}

inline int beat_number_from_ticks(signed long ticks) {
  return (ticks / PPQN) % 4;
}
inline int step_number_from_ticks(signed long ticks) {
  return (ticks / (PPQN)) % NUM_STEPS;
}

bool should_trigger_sequence(unsigned long ticks, int sequence, int offset = 0) {
  int step = step_number_from_ticks(ticks); //(ticks / (PPQN)) % NUM_STEPS;
  /*Serial.print(F("On step "));
  Serial.print(step);
  Serial.println(F("!"));*/

  if (is_bpm_on_beat(ticks, offset)) {
    if (sequence_data[sequence][step]>0) {
      Serial.print(F("Trigger sequence "));
      Serial.print(sequence);
      Serial.print(F(" on step "));
      Serial.print(step);
      Serial.println(F("!"));
      return true;
      //digitalWrite(PIN_CLOCK_START+i, HIGH);
    } 
  }
  return false;
}


#endif
