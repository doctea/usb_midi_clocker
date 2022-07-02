#ifndef MIDIOUT__INCLUDED
#define MIDIOUT__INCLUDED

#include <MIDI.h>

#include "Config.h"
#include "ConfigMidi.h"
#include "MidiMappings.h"

#define NUM_MIDI_OUTS   8

// Create the Serial MIDI ports
#if NUM_MIDI_OUTS>0
//MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI1);
extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> MIDI1;
#endif
#if NUM_MIDI_OUTS>1
//MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI2);
extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> MIDI2;
#endif
#if NUM_MIDI_OUTS>2
//MIDI_CREATE_INSTANCE(HardwareSerial, Serial3, MIDI3);
extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> MIDI3;
#endif
#if NUM_MIDI_OUTS>3
//MIDI_CREATE_INSTANCE(HardwareSerial, Serial4, MIDI4);
extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> MIDI4;
#endif
#if NUM_MIDI_OUTS>4
//MIDI_CREATE_INSTANCE(HardwareSerial, Serial5, MIDI5);
extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> MIDI5;
#endif
#if NUM_MIDI_OUTS>5
//MIDI_CREATE_INSTANCE(HardwareSerial, Serial6, MIDI6);
extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> MIDI6;
#endif
#if NUM_MIDI_OUTS>6
//MIDI_CREATE_INSTANCE(HardwareSerial, Serial7, MIDI7);
extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> MIDI7;
#endif
#if NUM_MIDI_OUTS>7
//MIDI_CREATE_INSTANCE(HardwareSerial, Serial8, MIDI8);
extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> MIDI8;
#endif

extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_serial[NUM_MIDI_OUTS];
extern bool midi_out_serial_clock_enabled[NUM_MIDI_OUTS];


void send_midi_serial_clocks();
void send_midi_serial_stop_start();
void read_midi_serial_devices();

void loop_midi_serial_devices();

#include "ConfigMidi.h"

#endif