#include "behaviour_keystep.h"

/*extern int current_keystep_note;
extern int last_keystep_note;

extern int bass_transpose_octave;

int current_keystep_note = -1;
int last_keystep_note = -1;

int bass_transpose_octave = 2;*/

DeviceBehaviour_Keystep *behaviour_keystep = nullptr;

/*void keystep_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    if (behaviour_keystep!=nullptr) behaviour_keystep->control_change(inChannel, inNumber, inValue);
}*/

void keystep_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_keystep!=nullptr) behaviour_keystep->note_on(inChannel, inNumber, inVelocity);
}

void keystep_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_keystep!=nullptr) behaviour_keystep->note_off(inChannel, inNumber, inVelocity);
}

MIDIOutputWrapper *keystep_output = &midi_out_bass_wrapper;
void keystep_setOutputWrapper(MIDIOutputWrapper *wrapper) {
  keystep_output->stop_all_notes();
  keystep_output = wrapper;    
}

