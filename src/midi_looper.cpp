//#include <midi.h>
#include <MIDI.h>

#include "bpm.h"
#include "midi_looper.h"

#include "Config.h"
#include "ConfigMidi.h"
#include "midi_outs.h"

#include "midi_drums.h"

#include "midi_mapper_matrix_manager.h"

// TODO: rewrite all this to work a lot better, ie:-
//      [DONE    >1 event per tick]
//      record as midi file
//      load as midi file
//      [DONE    make stop_all_notes]
//      able to track multiple devices / channels
//      [DONE] transpose
//      [DONE] quantizing (time)
//      quantizing (scale)

MIDITrack mpk49_loop_track;// = MIDITrack(); // = MIDITrack(&midi_out_bitbox_wrapper); //&MIDIOutputWrapper(midi_out_bitbox, 3));

#ifdef ENABLE_DRUM_LOOPER
    MIDITrack drums_loop_track = MIDITrack(); //&midi_drums_output); //midi_bamble);
#endif

// for sending passthrough or recorded noteOns to actual output
void MIDITrack::sendNoteOn(byte pitch, byte velocity, byte channel) {
    midi_matrix_manager->processNoteOn(this->source_id, pitch, velocity); //, channel);
}
// for sending passthrough or recorded noteOffs to actual output
void MIDITrack::sendNoteOff(byte pitch, byte velocity, byte channel) {
    midi_matrix_manager->processNoteOff(this->source_id, pitch, velocity); //, channel);
}

void MIDITrack::stop_all_notes() {
    midi_matrix_manager->stop_all_notes_for_source(this->source_id);

    if (current_note!=-1) 
        last_note = current_note;
    current_note = -1;
}