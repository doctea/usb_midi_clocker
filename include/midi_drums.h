#ifndef MIDI_DRUMS__INCLUDED
#define MIDI_DRUMS__INCLUDED
#include "midi_looper.h"

extern MIDIOutputWrapper midi_drums_output;// = MIDIOutputWrapper("Bamble drums", &midi_bamble, 10);
extern MIDITrack drums_loop_track;// = MIDITrack(&midi_drums_output);

#endif