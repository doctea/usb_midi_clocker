#ifndef CONFIGMIDI__INCLUDED
#define CONFIGMIDI__INCLUDED

#include "Config.h"
//#include "ConfigMidi.h"
#include "midi_outs.h"

#include "MidiMappings.h"
//#include "usb.h"

#include "midi_outs.h"

#ifdef ENABLE_LESTRUM
    #undef ENABLE_LESTRUM
    #define ENABLE_LESTRUM          MIDI1
#endif
#ifdef ENABLE_DRUMKIT
    #undef ENABLE_DRUMKIT
    #define ENABLE_DRUMKIT          MIDI2
#endif

#define ENABLE_BITBOX           MIDI1
//#ifdef ENABLE_BASS_TRANSPOSE
//    #undef ENABLE_BASS_TRANSPOSE
    #define ENABLE_BASS_TRANSPOSE   MIDI3
//#endif

//#include "midi_beatstep.h"
//#include "midi_apcmini.h"
//#include "midi_bamble.h"
//#include "midi_mpk49.h"
//#include "midi_keystep.h"

#endif