#include <Arduino.h>

#include "Config.h"
#include "ConfigMidi.h"
#include "behaviour_base_serial.h"

#include "behaviour_drumkit.h"

DeviceBehaviour_DrumKit behaviour_drumkit = DeviceBehaviour_DrumKit(); //(&ENABLE_LESTRUM, (midi::MidiInterface<midi::SerialMIDI<HardwareSerial>>*)nullptr);

// callback
void drumkit_note_on(byte pitch, byte velocity, byte channel) {
    Serial.println("drumkit_note_on()");
    behaviour_drumkit.note_on(pitch, velocity, channel);
}
// callback
void drumkit_note_off(byte pitch, byte velocity, byte channel) {
    Serial.println("drumkit_note_off()");
    behaviour_drumkit.note_off(pitch, velocity, channel);
}