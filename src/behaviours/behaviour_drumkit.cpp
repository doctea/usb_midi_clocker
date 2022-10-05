#include <Arduino.h>

#include "Config.h"
#include "ConfigMidi.h"
#include "behaviours/behaviour_base_serial.h"

#include "behaviours/behaviour_drumkit.h"

DeviceBehaviour_DrumKit *behaviour_drumkit = new DeviceBehaviour_DrumKit(); //(&ENABLE_LESTRUM, (midi::MidiInterface<midi::SerialMIDI<HardwareSerial>>*)nullptr);

// callback
void drumkit_note_on(byte pitch, byte velocity, byte channel) {
    Serial.println("drumkit_note_on()");
    behaviour_drumkit->receive_note_on(pitch, velocity, channel);
}
// callback
void drumkit_note_off(byte pitch, byte velocity, byte channel) {
    Serial.println("drumkit_note_off()");
    behaviour_drumkit->receive_note_off(pitch, velocity, channel);
}