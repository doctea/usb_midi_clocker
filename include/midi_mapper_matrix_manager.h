#ifndef MIDI_MAPPER_MATRIX__INCLUDED
#define MIDI_MAPPER_MATRIX__INCLUDED

#include "Config.h"
#include "midi_outs.h"
#include "midi_out_wrapper.h"

//#include "behaviour_bamble.h"

class MIDIMatrixManager {
    public:
    static MIDIMatrixManager* getInstance();

    // so we wanna do something like:-
    //      for each source
    //          store a list of targets it is connected to
    //      this requires a concept of a 'source', so like an 'source wrapper'.. 
    //          devicebehaviours might be a candidate for these?
    //          but there are also serial-midi sources, and sources from loopers...
    //          each source needs to be able to talk to this matrix manager, to know which targets to send notes to
    //      then we have targets (outputwrappers?)
    //          they don't need to know what their sources are?
    //      sometimes a source can also be a target, eg, looper.

    private:
        static MIDIMatrixManager* inst_;
        MIDIMatrixManager() {
            //setup_midi_output_wrapper_manager();
        }
        MIDIMatrixManager(const MIDIMatrixManager&);
        MIDIMatrixManager& operator=(const MIDIMatrixManager&);
};

extern MIDIMatrixManager *midi_matrix_manager;

#endif