#include "behaviours/behaviour_keystep.h"

DeviceBehaviour_Keystep *behaviour_keystep = nullptr;

void keystep_handle_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    //Serial.printf("keystep_handle_control_change(%i, %i, %i)...\n", inChannel, inNumber, inValue); Serial.flush();
    if (behaviour_keystep!=nullptr) behaviour_keystep->receive_control_change(inChannel, inNumber, inValue);
    //Serial.printf("...did keystep_handle_control_change(%i, %i, %i)!\n", inChannel, inNumber, inValue); Serial.flush();
}

void keystep_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_keystep!=nullptr) behaviour_keystep->receive_note_on(inChannel, inNumber, inVelocity);
}

void keystep_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_keystep!=nullptr) behaviour_keystep->receive_note_off(inChannel, inNumber, inVelocity);
}

/*MIDIOutputWrapper *keystep_output = nullptr; //&midi_out_bitbox_wrapper;
void keystep_setOutputWrapper(MIDIOutputWrapper *wrapper) {
    if (keystep_output!=nullptr)
        keystep_output->stop_all_notes();
    keystep_output = wrapper;    
}
*/