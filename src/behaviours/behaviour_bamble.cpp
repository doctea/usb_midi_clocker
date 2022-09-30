#include "behaviours/behaviour_bamble.h"

DeviceBehaviour_Bamble *behaviour_bamble = new DeviceBehaviour_Bamble(); //nullptr;

void bamble_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    if (behaviour_bamble!=nullptr) behaviour_bamble->receive_control_change(inChannel, inNumber, inValue);
}

void bamble_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_bamble!=nullptr) behaviour_bamble->receive_note_on(inChannel, inNumber, inVelocity);
}

void bamble_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_bamble!=nullptr) behaviour_bamble->receive_note_off(inChannel, inNumber, inVelocity);
}
