#ifndef MIDI_INCLUDED
#define MIDI_INCLUDED

#include <MIDI.h>

#include "Config.h"
//#include "MidiMappings.h"

#define NUM_MIDI_OUTS   8

// Create the Serial MIDI ports
#if NUM_MIDI_OUTS>0
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI1);
#endif
#if NUM_MIDI_OUTS>1
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI2);
#endif
#if NUM_MIDI_OUTS>2
MIDI_CREATE_INSTANCE(HardwareSerial, Serial3, MIDI3);
#endif
#if NUM_MIDI_OUTS>3
MIDI_CREATE_INSTANCE(HardwareSerial, Serial4, MIDI4);
#endif
#if NUM_MIDI_OUTS>4
MIDI_CREATE_INSTANCE(HardwareSerial, Serial5, MIDI5);
#endif
#if NUM_MIDI_OUTS>5
MIDI_CREATE_INSTANCE(HardwareSerial, Serial6, MIDI6);
#endif
#if NUM_MIDI_OUTS>6
MIDI_CREATE_INSTANCE(HardwareSerial, Serial7, MIDI7);
#endif
#if NUM_MIDI_OUTS>7
MIDI_CREATE_INSTANCE(HardwareSerial, Serial8, MIDI8);
#endif

midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_outs[NUM_MIDI_OUTS] = {
    &MIDI1,&MIDI2,&MIDI3,&MIDI4,&MIDI5,&MIDI6,&MIDI7,&MIDI8
};

bool midi_out_clock_enabled[NUM_MIDI_OUTS] = {
    true,
    false,
    false,
    false,
    false,
    false,
    false,
    false
};

void send_midi_device_clocks() {
    for (int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
        if (midi_out_clock_enabled[i]) {
            midi_outs[i]->sendRealTime(midi::Clock);
        }
    }
}

void send_midi_device_stop_start() {
    // send all stops first
    for (int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
        if (midi_out_clock_enabled[i]) {
            midi_outs[i]->sendRealTime(midi::Stop);
        }
    }
    // then send all starts
    for (int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
        if (midi_out_clock_enabled[i]) {
            midi_outs[i]->sendRealTime(midi::Start);
        }
    }
}

void update_midi_serial_devices() {
    for (int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
        while (midi_outs[i]->read());
    }
}


midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_bitbox      = &MIDI1;
midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_cv12_poly   = &MIDI2;

midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_lestrum      = &MIDI1;

#endif