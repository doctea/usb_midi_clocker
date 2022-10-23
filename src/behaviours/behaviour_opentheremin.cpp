#include "behaviours/behaviour_opentheremin.h"

DeviceBehaviour_OpenTheremin *behaviour_opentheremin = nullptr;

void handle_theremin_control_change(byte inNumber, byte inValue, byte inChannel) {
    Serial.printf("handle_theremin_control_change(\t%i,\t%i,\t%i)!\n", inNumber, inValue, inChannel);
    if (behaviour_opentheremin!=nullptr) behaviour_opentheremin->receive_control_change(inChannel, inNumber, inValue);
}
void handle_theremin_note_on(byte inNumber, byte inVelocity, byte inChannel) {
    Serial.printf("handle_theremin_note_on(\t%i,\t%i,\t%i)!\n", inNumber, inVelocity, inChannel);
    if (behaviour_opentheremin!=nullptr) behaviour_opentheremin->receive_note_on(inChannel, inNumber, inVelocity);
}
void handle_theremin_note_off(byte inNumber, byte inVelocity, byte inChannel) {
    Serial.printf("handle_theremin_note_off(\t%i,\t%i,\t%i)!\n", inNumber, inVelocity, inChannel);
    if (behaviour_opentheremin!=nullptr) behaviour_opentheremin->receive_note_off(inChannel, inNumber, inVelocity);
}
