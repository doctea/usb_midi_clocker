#pragma once

#include "Config.h"
//#include "ConfigMidi.h"
#include "midi/midi_outs.h"

#include "midi/MidiMappings.h"
//#include "usb.h"

#include "midi/midi_outs.h"

//////////////////////////////// SERIAL MIDI DIN INPUTS

#define ENABLE_LESTRUM          MIDI1   // used for input

#ifdef PCB_STUDIO
    #define ENABLE_DRUMKIT          MIDI2   // used for input
#endif

//////////////////////////////// SERIAL MIDI DIN OUTPUTS

#ifdef PCB_GO
    #define ENABLE_BITBOX               MIDI1   // used for output
    #define ENABLE_BITBOX_DEDICATED
#endif
#ifdef PCB_STUDIO
    #define ENABLE_NEUTRON              MIDI3   // bass output on MIDI3   // WTF TODO FIX CRASH - crashes during 'doing addParameters -> behaviour neutron -> ??' when no screen is enabled
    #define ENABLE_DISTING              midi_out_serial[3]     // 
#endif

