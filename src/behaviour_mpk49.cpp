#include "behaviour_mpk49.h"

DeviceBehaviour_mpk49 *behaviour_mpk49 = nullptr;

/*void mpk49_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    if (behaviour_mpk49!=nullptr) behaviour_mpk49->control_change(inChannel, inNumber, inValue);
}*/

void mpk49_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    //Serial.println("mpk49_handle_note_on");
    if (behaviour_mpk49!=nullptr) behaviour_mpk49->note_on(inChannel, inNumber, inVelocity);
}

void mpk49_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    //Serial.println("mpk49_handle_note_off");
    if (behaviour_mpk49!=nullptr) behaviour_mpk49->note_off(inChannel, inNumber, inVelocity);
}

void mpk49_handle_system_exclusive(uint8_t *data, unsigned int size) {
    //Serial.println("mpk49_handle_system_exclusive");
    if (behaviour_mpk49!=nullptr) behaviour_mpk49->handle_system_exclusive(data, size);
}

/*MIDIOutputWrapper *mpk49_output = nullptr; //&midi_out_bitbox_wrapper;
void mpk49_setOutputWrapper(MIDIOutputWrapper *wrapper) {
    if (mpk49_output!=nullptr)
        mpk49_output->stop_all_notes();
    mpk49_output = wrapper;    
}*/
