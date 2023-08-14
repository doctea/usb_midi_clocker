// this is for the cv gate sequencer, that is controlled by the APCMini

#include "storage.h"
#include "sequencer.h"
#include "bpm.h"
#include "project.h"

#include "interfaces/interfaces.h"

//#define byte uint8_t

FLASHMEM void init_sequence() {
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

void cv_out_sequence_pin_off(byte i) {
  //raw_write_pin(cv_out_sequence_pin[i], LOW);  // TODO: MCP23017 version of this
  set_sequence_gate(i, LOW);
}
void cv_out_sequence_pin_on(byte i) {
  //raw_write_pin(cv_out_sequence_pin[i], HIGH); // TODO: MCP23017 version of this
  set_sequence_gate(i, HIGH);
}

byte read_sequence(byte row, byte col) {
  return current_state.sequence_data[row][col];
}
void write_sequence(byte row, byte col, byte value) {
  /*ATOMIC(
    sequence_data[row][col] = !sequence_data[row][col];
  )*/
  current_state.sequence_data[row][col] = value;
  if (current_state.sequence_data[row][col]==255)
    current_state.sequence_data[row][col] = SEQUENCER_MAX_VALUE;
  else if (current_state.sequence_data[row][col]>SEQUENCER_MAX_VALUE)
    current_state.sequence_data[row][col] = 0;
}

void sequencer_press(byte row, byte col, bool shift) { 
  //sequence_data[row][col] = !sequence_data[row][col];
  if (shift) 
    write_sequence(row, col, read_sequence(row, col)-1);
  else 
    write_sequence(row, col, read_sequence(row, col)+1);
}

bool should_trigger_sequence(unsigned long ticks, byte sequence, int offset) {
  byte step = step_number_from_ticks(ticks); //(ticks / (PPQN)) % NUM_STEPS;
  /*if (offset==0 && is_bpm_on_beat(ticks)) {
    Serial.print(F("On step "));
    Serial.print(step);
    Serial.println(F("!"));
  }*/

  byte v = read_sequence(sequence, step);
  if (v) {
    /*if (is_bpm_on_beat(ticks, offset) 
        || (v==2 && is_bpm_on_eighth(ticks, offset))      // ratchetting
        || (v==3 && is_bpm_on_sixteenth(ticks, offset))   // ratchetting
    ) {*/
    switch(v) {
      case 1:
        if (is_bpm_on_beat(ticks, offset)) return true; break;
      case 2:
        if (is_bpm_on_eighth(ticks, offset)) return true; break;
      case 3:
        if (is_bpm_on_sixteenth(ticks, offset)) return true; break;
      case 4:
        if (!is_bpm_on_beat(ticks, offset) && is_bpm_on_eighth(ticks, offset)) return true; break;
      case 5:
        if (!is_bpm_on_beat(ticks, offset) && !is_bpm_on_eighth(ticks, offset) && is_bpm_on_sixteenth(ticks, offset)) return true; break;
      /*case 6:
        if (!is_bpm_on_beat(ticks, offset) && !is_bpm_on_eighth(ticks, offset) && ) return true; break;       */
      default:
        return false;
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
      //return true;
      //digitalWrite(PIN_CLOCK_START+i, HIGH);
    } 
  }
  return false;
}
