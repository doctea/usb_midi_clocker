#include "behaviours/behaviour_mpk49.h"

#ifdef ENABLE_MPK49

DeviceBehaviour_mpk49 *behaviour_mpk49 = nullptr;

void mpk49_handle_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    if (behaviour_mpk49!=nullptr) behaviour_mpk49->receive_control_change(inChannel, inNumber, inValue);
}

void mpk49_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    //Serial.println("mpk49_handle_note_on");
    if (behaviour_mpk49!=nullptr) behaviour_mpk49->receive_note_on(inChannel, inNumber, inVelocity);
}

void mpk49_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    //Serial.println("mpk49_handle_note_off");
    if (behaviour_mpk49!=nullptr) behaviour_mpk49->receive_note_off(inChannel, inNumber, inVelocity);
}

void mpk49_handle_system_exclusive(uint8_t *data, unsigned int size) {
    //Serial.println("mpk49_handle_system_exclusive");
    if (behaviour_mpk49!=nullptr) behaviour_mpk49->handle_system_exclusive(data, size);
}

#endif