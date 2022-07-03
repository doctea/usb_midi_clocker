#include "behaviour_beatstep.h"

extern int current_beatstep_note;
extern int last_beatstep_note;

extern int bass_transpose_octave;

int current_beatstep_note = -1;
int last_beatstep_note = -1;

int bass_transpose_octave = 2;

DeviceBehaviour_Beatstep *behaviour_beatstep = nullptr;

/*void beatstep_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    if (behaviour_beatstep!=nullptr) behaviour_beatstep->control_change(inChannel, inNumber, inValue);
}*/

void beatstep_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_beatstep!=nullptr) behaviour_beatstep->note_on(inChannel, inNumber, inVelocity);
}

void beatstep_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_beatstep!=nullptr) behaviour_beatstep->note_off(inChannel, inNumber, inVelocity);
}

MIDIOutputWrapper *beatstep_output = &midi_out_bass_wrapper;
void beatstep_setOutputWrapper(MIDIOutputWrapper *wrapper) {
  beatstep_output->stop_all_notes();
  beatstep_output = wrapper;
}

