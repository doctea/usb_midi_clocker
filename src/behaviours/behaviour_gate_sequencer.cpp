// this is for the cv gate sequencer, that is controlled by the APCMini

#include "storage.h"
#include "bpm.h"
#include "project.h"

#include "interfaces/interfaces.h"

#include "behaviours/behaviour_gate_sequencer.h"

//#define byte uint8_t

FLASHMEM void VirtualBehaviour_SequencerGates::init_sequence() {
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

void VirtualBehaviour_SequencerGates::sequencer_clear_pattern() {
  for (int i = 0 ; i < NUM_SEQUENCES ; i++) {
    sequencer_clear_row(i);
  }
}

void VirtualBehaviour_SequencerGates::sequencer_clear_row(byte row) {
  for (int i = 0 ; i < NUM_STEPS ; i++) {
    write_sequence(row, i, 0);
  }
}

/*void VirtualBehaviour_SequencerGates::cv_out_sequence_pin_off(byte i) {
  //raw_write_pin(cv_out_sequence_pin[i], LOW);  // TODO: MCP23017 version of this
  set_sequence_gate(i, LOW);
}
void VirtualBehaviour_SequencerGates::cv_out_sequence_pin_on(byte i) {
  //raw_write_pin(cv_out_sequence_pin[i], HIGH); // TODO: MCP23017 version of this
  set_sequence_gate(i, HIGH);
}*/

byte VirtualBehaviour_SequencerGates::read_sequence(byte row, byte col) {
  return current_state.sequence_data[row][col];
}
void VirtualBehaviour_SequencerGates::write_sequence(byte row, byte col, byte value) {
  /*ATOMIC(
    sequence_data[row][col] = !sequence_data[row][col];
  )*/
  current_state.sequence_data[row][col] = value;
  if (current_state.sequence_data[row][col]==255)
    current_state.sequence_data[row][col] = SEQUENCER_MAX_VALUE;
  else if (current_state.sequence_data[row][col]>SEQUENCER_MAX_VALUE)
    current_state.sequence_data[row][col] = 0;
}

void VirtualBehaviour_SequencerGates::sequencer_press(byte row, byte col, bool shift) { 
  //sequence_data[row][col] = !sequence_data[row][col];
  if (shift) 
    write_sequence(row, col, read_sequence(row, col)-1);
  else 
    write_sequence(row, col, read_sequence(row, col)+1);
}

bool VirtualBehaviour_SequencerGates::is_tick_for_step_type(byte type, unsigned long ticks, int offset) {
  switch(type) {
      case 0:
        return false;
        break;
      case 1:
        if (is_bpm_on_beat(ticks, offset)) return true; 
        break;
      case 2:
        if (is_bpm_on_eighth(ticks, offset)) return true; 
        break;
      case 3:
        if (is_bpm_on_sixteenth(ticks, offset)) return true; 
        break;
      case 4:
        if (!is_bpm_on_beat(ticks, offset) && is_bpm_on_eighth(ticks, offset)) return true; 
        break;
      case 5:
        if (!is_bpm_on_beat(ticks, offset) && !is_bpm_on_eighth(ticks, offset) && is_bpm_on_sixteenth(ticks, offset)) return true; 
        break;
      case 6:
        if (is_bpm_on_beat(ticks, offset) || (is_bpm_on_sixteenth(ticks, offset) && ticks%PPQN >= (PPQN/4)*3)) return true; 
        break;       
      default:
        return false;
  }
  return false;
}

bool VirtualBehaviour_SequencerGates::should_trigger_sequence(unsigned long ticks, byte sequence, int offset) {
  // todo: option to disable sequencer so that can just send eg midi drums without confusion
  // return false;  // for testing, disable sequencer entirely
  if (!this->sequencer_enabled)
    return false;

  if (!this->is_track_active(sequence))
    return false;

  byte step = step_number_from_ticks(ticks);
  byte step_type = read_sequence(sequence, step);

  return is_tick_for_step_type(step_type, ticks, offset);
}


void VirtualBehaviour_SequencerGates::process_sequencer(unsigned long ticks) {
    // todo: investigate whether we can use uClock's shuffle to process these instead..
    if (!this->sequencer_enabled)
        return;

    #ifdef ENABLE_SEQUENCER
        for (unsigned int i = 0 ; i < NUM_SEQUENCES ; i++) {
            bool should_go_high = false;
            bool should_go_low  = false;

            if (should_trigger_sequence(ticks, i)) {
                should_go_high = true;
            } else if (should_trigger_sequence(ticks, i, duration)) {
                should_go_low = true;
            }

            if (should_go_high)     cv_out_sequence_pin_on (i);
            else if (should_go_low) cv_out_sequence_pin_off(i);
        }

        /*#ifdef ENABLE_MORE_CLOCKS
        for (unsigned int i = 0 ; i < 4 ; i++) {
            bool should_go_high = false;
            bool should_go_low  = false;

            float m;
            if (i<2) 
            m = clock_multiplier_values[i];
            else 
            m = clock_multiplier_values[i+4];
            
            if (is_bpm_on_multiplier(ticks, m)) {
            //Serial.printf("%3i: extra clock %i has multiplier %f - on!\n", ticks, i, m);
            should_go_high = true;
            } else if (is_bpm_on_multiplier(ticks, m, duration)) {
            //Serial.printf("%3i: extra clock %i has multiplier %f - off!\n", ticks, i, m);
            should_go_low = true;
            }

            // actually use the pins after the sequencer output pins for the extra clocks
            if (should_go_high)     cv_out_sequence_pin_on (NUM_SEQUENCES + i);
            else if (should_go_low) cv_out_sequence_pin_off(NUM_SEQUENCES + i);
        }
        #endif*/
    #endif
}

VirtualBehaviour_SequencerGates *behaviour_sequencer_gates;