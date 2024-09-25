#include "Config.h"
#include "ConfigMidi.h"
#include "midi/MidiMappings.h"
#include "midi/midi_outs.h"

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI1);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI2);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial3, MIDI3);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial4, MIDI4);
// todo: make this configurable so that eg we can have pcb_go environment only using first 4 MIDI ports
MIDI_CREATE_INSTANCE(HardwareSerial, Serial5, MIDI5);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial6, MIDI6);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial7, MIDI7);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial8, MIDI8);

midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_serial[NUM_MIDI_OUTS] = {
    // TODO: properly initialise up to the size of NUM_MIDI_OUTS
    &MIDI1,&MIDI2,&MIDI3,&MIDI4,&MIDI5,&MIDI6,&MIDI7,&MIDI8
};
