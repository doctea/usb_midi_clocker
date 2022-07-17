#ifndef MIDIMAPPINGS__INCLUDED
#define MIDIMAPPINGS__INCLUDED

#define MIDI_IN_NUMBER_LESTRUM 0

#include "Config.h"
#include "ConfigMidi.h"
#include "midi_out_wrapper.h"

//#ifdef ENABLE_BITBOX
extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_bitbox;//      = &ENABLE_BITBOX;
extern MIDIOutputWrapper midi_out_bitbox_wrapper;
#define BITBOX_MIDI_CHANNEL 3
//#endif
//midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_cv12_poly   = &MIDI2;     // output 


//#ifdef ENABLE_LESTRUM
extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_lestrum;//      = &ENABLE_LESTRUM;
//#endif

//#ifdef ENABLE_DRUMKIT
extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_drumkit;//      = &ENABLE_DRUMKIT;
//#endif

extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_bass;//      = &ENABLE_BASS_TRANSPOSE;
extern MIDIOutputWrapper midi_out_bass_wrapper;
#define BASS_MIDI_CHANNEL   4

#ifdef ENABLE_DRUMKIT
    // configure incoming drumkit on input 2 to go out to drums on bamble
    void drumkit_note_on(byte channel, byte note, byte velocity);
    void drumkit_note_off(byte channel, byte note, byte velocity);
#endif

void setup_midi_serial_devices();

#endif