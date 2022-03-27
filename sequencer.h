// work in progress -- doesn't work yet
#ifndef INCLUDED_SEQUENCER
#define INCLUDED_SEQUENCER

#define SEQUENCER_BYTES // define this to use more memory-hungry internal layout of data

#define NUM_SEQUENCES 4
#define NUM_STEPS     8

#ifdef SEQUENCER_BYTES

#define SEQUENCER_MAX_VALUE 3

volatile byte sequence_data[NUM_SEQUENCES][NUM_STEPS] = {
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 1, 0, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 1, 0, 0, 0, 0 }
};
#else
volatile byte sequence_data[NUM_SEQUENCES] = { // order is reversed because bit 0 = rightmost bit
  0b00000000,
  0b00000000,
  0b00000101,   
  0b00001000
};
#endif

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
  return (ticks / PPQN) % BEATS_PER_BAR;
}
inline int step_number_from_ticks(signed long ticks) {
  return (ticks / (PPQN)) % NUM_STEPS;
}

#ifdef SEQUENCER_BYTES
inline byte read_sequence(byte row, byte col) {
  return sequence_data[row][col];
}
inline void write_sequence(byte row, byte col, byte value) {
  /*ATOMIC(
    sequence_data[row][col] = !sequence_data[row][col];
  )*/
  sequence_data[row][col] = value;
  if (sequence_data[row][col]==255)
    sequence_data[row][col] = SEQUENCER_MAX_VALUE;
  else if (sequence_data[row][col]>SEQUENCER_MAX_VALUE)
    sequence_data[row][col] = 0;
}

void sequencer_press(byte row, byte col, bool shift = false) {
  //sequence_data[row][col] = !sequence_data[row][col];
  if (shift) 
    write_sequence(row, col, read_sequence(row, col)-1);
  else 
    write_sequence(row, col, read_sequence(row, col)+1);
}
#else
inline bool read_sequence(byte row, byte col) {
  return bitRead(sequence_data[row], /*NUM_STEPS -*/ col);
}
inline void write_sequence(byte row, byte col, byte value) {
  ATOMIC(
    bitWrite(sequence_data[row], /*NUM_STEPS -*/ col, value);
  )
}
void sequencer_press(byte row, byte col, bool shift = false) {
  //sequence_data[row][col] = !sequence_data[row][col];
  write_sequence(row, col, !read_sequence(row, col));
}
#endif

bool should_trigger_sequence(unsigned long ticks, byte sequence, int offset = 0) {
  byte step = step_number_from_ticks(ticks); //(ticks / (PPQN)) % NUM_STEPS;
  /*if (offset==0 && is_bpm_on_beat(ticks)) {
    Serial.print(F("On step "));
    Serial.print(step);
    Serial.println(F("!"));
  }*/

  byte v = read_sequence(sequence, step);
  if (v) {
    if (is_bpm_on_beat(ticks, offset) 
        || (v==2 && is_bpm_on_eighth(ticks, offset))
        || (v==3 && is_bpm_on_sixteenth(ticks, offset))
    ) {
#ifdef DEBUG_SEQUENCER
      if (offset==0) {
        Serial.print(F("For tick "));
        Serial.print(ticks);
        Serial.print(F(" got step_number "));
        Serial.print(step);
        Serial.print(F(", trigger sequence #"));
        Serial.print(sequence);
        Serial.print(F(" on step "));
        Serial.print(step);
        Serial.println(F("!"));
      } 
#endif
      return true;
      //digitalWrite(PIN_CLOCK_START+i, HIGH);
    } 
  }
  return false;
}


#endif
