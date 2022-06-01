#ifndef MIDI_DRUMS__INCLUDED
#define MIDI_DRUMS__INCLUDED
#include "midi_looper.h"

MIDIOutputWrapper midi_drums_output = MIDIOutputWrapper("Bamble drums", &midi_bamble, 10);
MIDITrack drum_loop_track = MIDITrack(&midi_drums_output);

#endif