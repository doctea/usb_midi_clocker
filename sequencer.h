#ifndef INCLUDED_SEQUENCER
#define INCLUDED_SEQUENCER

#define NUM_SEQUENCES 4
#define NUM_STEPS     8

byte sequence[NUM_SEQUENCES][NUM_STEPS];

void init_sequence() {
  for (int seq = 0 ; seq < NUM_SEQUENCES ; seq++) {
    for (int ste = 0 ; ste < NUM_STEPS ; ste++) {
      if (ste%(byte)clock_multiplier[seq]==0 || clock_multiplier[seq]<1.0) {
        sequence[seq][ste] = 1;
      } else {
        sequence[seq][ste] = 0;
      }
    }
  }
}

void trigger_sequence(unsigned long ticks) {
  int step = (ticks / (PPQN)) % NUM_STEPS;
  Serial.print(F("On step "));
  Serial.print(step);
  Serial.println(F("!"));

  for (int i = 0 ; i < NUM_SEQUENCES ; i++) {
    if (is_bpm_on_eighth(ticks)) {
      if (sequence[i][step]>0) {
        digitalWrite(PIN_CLOCK_START+i, HIGH);
      } 
    } else { //if (sequence[(i-1)%NUM_STEPS]>0) {
      digitalWrite(PIN_CLOCK_START+i, LOW);
    }
  }
}


#endif
