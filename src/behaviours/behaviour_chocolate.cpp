#include "Config.h"
#ifdef ENABLE_CHOCOLATEFEET_USB

#include "behaviours/behaviour_chocolate.h"
DeviceBehaviour_Chocolate *behaviour_chocolate; // = new DeviceBehaviour_Chocolate();

/*DeviceBehaviour_Beatstep behaviour_beatstep_actual = DeviceBehaviour_Beatstep();
DeviceBehaviour_Beatstep *behaviour_beatstep = &behaviour_beatstep_actual;*/

/*void chocolate_handle_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    Serial.printf("chocolate_handle_note_off called, %i %i %i\n", inChannel, inNumber, inValue);
    if (behaviour_chocolate!=nullptr) behaviour_chocolate->receive_control_change(inChannel, inNumber, inValue);
}*/

void chocolate_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    //Serial.printf("chocolate_handle_note_on called, %i %i %i\n", inChannel, inNumber, inVelocity);
    if (behaviour_chocolate!=nullptr) behaviour_chocolate->receive_note_on(inChannel, inNumber, inVelocity);
}

void chocolate_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    //Serial.printf("chocolate_handle_note_off called, %i %i %i\n", inChannel, inNumber, inVelocity);
    if (behaviour_chocolate!=nullptr) behaviour_chocolate->receive_note_off(inChannel, inNumber, inVelocity);
}

#endif