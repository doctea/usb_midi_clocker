#include <Arduino.h>

#include "Config.h"
#include "ConfigMidi.h"
#include "behaviours/behaviour_base_serial.h"

#include "behaviours/behaviour_lestrum.h"

DeviceBehaviour_LeStrum *behaviour_lestrum = new DeviceBehaviour_LeStrum(); //(&ENABLE_LESTRUM, (midi::MidiInterface<midi::SerialMIDI<HardwareSerial>>*)nullptr);

// callback
void lestrum_note_on(byte pitch, byte velocity, byte channel) {
    if (behaviour_lestrum->debug) Serial.printf(F("lestrum_note_on(%i, %i, %i)\n"), pitch, velocity, channel);
    behaviour_lestrum->receive_note_on(pitch, velocity, channel);
}
// callback
void lestrum_note_off(byte pitch, byte velocity, byte channel) {
    if (behaviour_lestrum->debug) Serial.printf(F("lestrum_note_off(%i, %i, %i)\n"), pitch, velocity, channel);
    behaviour_lestrum->receive_note_off(pitch, velocity, channel);
}