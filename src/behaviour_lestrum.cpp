#include <Arduino.h>

#include "Config.h"
#include "ConfigMidi.h"
#include "behaviour_base_serial.h"

#include "behaviour_lestrum.h"

DeviceBehaviour_LeStrum behaviour_lestrum = DeviceBehaviour_LeStrum(); //(&ENABLE_LESTRUM, (midi::MidiInterface<midi::SerialMIDI<HardwareSerial>>*)nullptr);

// callback
void lestrum_note_on(byte pitch, byte velocity, byte channel) {
    Serial.println("lestrum_note_on()");
    behaviour_lestrum.note_on(pitch, velocity, channel);
}
// callback
void lestrum_note_off(byte pitch, byte velocity, byte channel) {
    Serial.println("lestrum_note_off()");
    behaviour_lestrum.note_off(pitch, velocity, channel);
}