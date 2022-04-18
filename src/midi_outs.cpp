#include "Config.h"
#include "ConfigMidi.h"
#include "MidiMappings.h"
#include "midi_outs.h"

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI1);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI2);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI3);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI4);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI5);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI6);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI7);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI8);

midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_serial[NUM_MIDI_OUTS] = {
    // TODO: properly initialise up to the size of NUM_MIDI_OUTS
    &MIDI1,&MIDI2,&MIDI3,&MIDI4,&MIDI5,&MIDI6,&MIDI7,&MIDI8
};

bool midi_out_serial_clock_enabled[NUM_MIDI_OUTS] = {
    true,
    false,
    false,
    false,
    false,
    false,
    false,
    false
};

void send_midi_serial_clocks() {
    for (int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
        if (midi_out_serial_clock_enabled[i]) {
            midi_out_serial[i]->sendRealTime(midi::Clock);
        }
    }
}

void send_midi_serial_stop_start() {
    // send all stops first
    for (int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
        if (midi_out_serial_clock_enabled[i]) {
            midi_out_serial[i]->sendRealTime(midi::Stop);
        }
    }
    // then send all starts
    for (int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
        if (midi_out_serial_clock_enabled[i]) {
            midi_out_serial[i]->sendRealTime(midi::Start);
        }
    }
}

void read_midi_serial_devices() {
    for (int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
        while (midi_out_serial[i]->read());
    }
}