#ifndef CONFIGMIDI__INCLUDED
#define CONFIGMIDI__INCLUDED

#include "Config.h"
//#include "ConfigMidi.h"
#include "midi/midi_outs.h"

#include "midi/MidiMappings.h"
//#include "usb.h"

#include "midi/midi_outs.h"

//////////////////////////////// SERIAL MIDI DIN INPUTS

#define ENABLE_LESTRUM          MIDI1   // used for input
#define ENABLE_DRUMKIT          MIDI2   // used for input

//////////////////////////////// SERIAL MIDI DIN OUTPUTS

#define ENABLE_BITBOX               MIDI1   // used for output
#define ENABLE_BASS_TRANSPOSE       MIDI3   // used for output
#define ENABLE_BASS                 MIDI3   // bass output on MIDI3   // WTF TODO FIX CRASH ?
//#endif

#define ENABLE_DISTING              MIDI4   // midi_out_serial[3]     // 

#endif