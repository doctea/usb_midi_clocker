#include "Config.h"

#ifdef ENABLE_OPENTHEREMIN

#include "behaviours/behaviour_opentheremin.h"

DeviceBehaviour_OpenTheremin *behaviour_opentheremin = nullptr;

void handle_theremin_control_change(byte inChannel, byte inNumber, byte inValue) {
    //Serial.printf("handle_theremin_control_change(\t%i,\t%i,\t%i)!\n", inNumber, inValue, inChannel);
    if (behaviour_opentheremin!=nullptr) behaviour_opentheremin->receive_control_change(inChannel, inNumber, inValue);
}
void handle_theremin_note_on(byte inChannel, byte inNumber, byte inVelocity) {
    //Serial.printf("handle_theremin_note_on (\t%i,\t%i,\t%i)!\n", inNumber, inVelocity, inChannel);
    if (behaviour_opentheremin!=nullptr) behaviour_opentheremin->receive_note_on(inChannel, inNumber, inVelocity);
}
void handle_theremin_note_off(byte inChannel, byte inNumber, byte inVelocity) {
    //Serial.printf("handle_theremin_note_off(\t%i,\t%i,\t%i)!\n", inNumber, inVelocity, inChannel);
    if (behaviour_opentheremin!=nullptr) behaviour_opentheremin->receive_note_off(inChannel, inNumber, inVelocity);
}
void handle_theremin_pitch_bend(byte channel, int bend) {
    if (behaviour_opentheremin!=nullptr) behaviour_opentheremin->receive_pitch_bend(channel, bend);
}


#endif