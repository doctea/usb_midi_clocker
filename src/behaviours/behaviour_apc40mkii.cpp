#include "behaviours/behaviour_apc40mkii.h"

#ifdef ENABLE_APC40MKII

DeviceBehaviour_APC40MKII *behaviour_apc40mkii;

void apc40mkii_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    if (behaviour_apc40mkii!=nullptr) behaviour_apc40mkii->receive_control_change(inChannel, inNumber, inValue);
}

void apc40mkii_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_apc40mkii!=nullptr) behaviour_apc40mkii->receive_note_on(inChannel, inNumber, inVelocity);
}

void apc40mkii_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_apc40mkii!=nullptr) behaviour_apc40mkii->receive_note_off(inChannel, inNumber, inVelocity);
}

#endif