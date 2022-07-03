#include "behaviour_apcmini.h"

DeviceBehaviour_APCMini *behaviour_apcmini;

void apcmini_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    if (behaviour_apcmini!=nullptr) behaviour_apcmini->control_change(inChannel, inNumber, inValue);
}

void apcmini_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_apcmini!=nullptr) behaviour_apcmini->note_on(inChannel, inNumber, inVelocity);
}

void apcmini_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_apcmini!=nullptr) behaviour_apcmini->note_off(inChannel, inNumber, inVelocity);
}
