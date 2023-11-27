#include "behaviours/behaviour_apcmini.h"

#ifdef ENABLE_APCMINI

DeviceBehaviour_APCMini *behaviour_apcmini;

void apcmini_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    if (behaviour_apcmini!=nullptr) behaviour_apcmini->receive_control_change(inChannel, inNumber, inValue);
}

void apcmini_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_apcmini!=nullptr) behaviour_apcmini->receive_note_on(inChannel, inNumber, inVelocity);
}

void apcmini_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_apcmini!=nullptr) behaviour_apcmini->receive_note_off(inChannel, inNumber, inVelocity);
}

#endif