#ifndef CONFIGMIDI__INCLUDED
#define CONFIGMIDI__INCLUDED

#include "Config.h"
#include "ConfigMidi.h"
#include "MidiMappings.h"
#include "usb.h"

#include "midi_beatstep.h"
#include "midi_apcmini.h"
#include "midi_bamble.h"

#ifdef ENABLE_BITBOX
midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_bitbox      = &ENABLE_BITBOX;
#endif
//midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_cv12_poly   = &MIDI2;     // output 
#ifdef ENABLE_BAMBLE
MIDIDevice *midi_out_cv12_poly   = midi_bamble;
#endif

#ifdef ENABLE_LESTRUM
midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_lestrum      = &ENABLE_LESTRUM;
#endif

#ifdef ENABLE_DRUMKIT
midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_drumkit      = &ENABLE_DRUMKIT;
#endif

#endif