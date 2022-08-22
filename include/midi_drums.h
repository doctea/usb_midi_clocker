#ifndef MIDI_DRUMS__INCLUDED
#define MIDI_DRUMS__INCLUDED
#include "midi_looper.h"

#include "midi_mapper_matrix_types.h"

//extern MIDIOutputWrapper midi_drums_output;// = MIDIOutputWrapper("Bamble drums", &midi_bamble, 10);
#ifdef ENABLE_DRUM_LOOPER
    extern MIDITrack drums_loop_track;// = MIDITrack(&midi_drums_output);
#endif

extern source_id_t drumkit_source_id;

#endif