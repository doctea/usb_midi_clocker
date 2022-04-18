#ifndef MIDIMAPPINGS__INCLUDED
#define MIDIMAPPINGS__INCLUDED

#define MIDI_IN_NUMBER_LESTRUM 0

#include "Config.h"
#include "ConfigMidi.h"

//#ifdef ENABLE_BITBOX
extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_bitbox;//      = &ENABLE_BITBOX;
//#endif
//midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_cv12_poly   = &MIDI2;     // output 
//#ifdef ENABLE_BAMBLE
//MIDIDevice *midi_out_cv12_poly   = midi_bamble;
//#endif

//#ifdef ENABLE_LESTRUM
extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_lestrum;//      = &ENABLE_LESTRUM;
//#endif

//#ifdef ENABLE_DRUMKIT
extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_drumkit;//      = &ENABLE_DRUMKIT;
//#endif


#ifdef ENABLE_LESTRUM
// configure incoming lestrum to output to midimuso via bamble
void lestrum_note_on(byte channel, byte note, byte velocity);
void lestrum_note_off(byte channel, byte note, byte velocity);
#endif


#ifdef ENABLE_DRUMKIT
// configure incoming drumkit on input 2 to go out to drums on bamble
void drumkit_note_on(byte channel, byte note, byte velocity);
void drumkit_note_off(byte channel, byte note, byte velocity);
#endif

void setup_midi_serial_devices();

#endif