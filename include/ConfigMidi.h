#ifndef CONFIGMIDI__INCLUDED
#define CONFIGMIDI__INCLUDED

#include "Config.h"
//#include "ConfigMidi.h"
#include "midi_outs.h"

#include "MidiMappings.h"
//#include "usb.h"

#include "midi_outs.h"

//////////////////////////////// SERIAL MIDI DIN INPUTS

//#ifdef ENABLE_LESTRUM
    //#undef ENABLE_LESTRUM
    #define ENABLE_LESTRUM          MIDI1   // used for input
//#endif
//#ifdef ENABLE_DRUMKIT
//    #undef ENABLE_DRUMKIT
    #define ENABLE_DRUMKIT          MIDI2   // used for input
//#endif

//#define ENABLE_CHOCOLATEFEET_SERIAL        MIDI3   // used for input -- not working?


//////////////////////////////// SERIAL MIDI DIN OUTPUTS

#define ENABLE_BITBOX               MIDI1   // used for output
//#ifdef ENABLE_BASS_TRANSPOSE
//    #undef ENABLE_BASS_TRANSPOSE
    #define ENABLE_BASS_TRANSPOSE   MIDI3   // used for output
//#endif


#endif