//#include <midi.h>
#include <MIDI.h>

#include "bpm.h"
#include "midi_looper.h"

#include "Config.h"
#include "ConfigMidi.h"
#include "midi_outs.h"

// TODO: rewrite all this to work a lot better, ie:-
//      [DONE    >1 event per tick]
//      record as midi file
//      load as midi file
//      [DONE    make stop_all_notes]
//      able to track multiple devices / channels
//      [DONE] transpose
//      [DONE] quantizing (time)
//      quantizing (scale)

/*typedef struct midi_frame {
    //unsigned long time;
    LinkedList<midi_message> messages = LinkedList<midi_message>();
};*/

MIDITrack mpk49_loop_track = MIDITrack(&midi_out_bitbox_wrapper); //&MIDIOutputWrapper(midi_out_bitbox, 3));


// from https://github.com/LesserChance/arduino-midi-looper/blob/master/instruction.ino

void stop_all_notes() {
    mpk49_loop_track.stop_all_notes();
}

void clear_recording() {
    mpk49_loop_track.stop_all_notes();
    mpk49_loop_track.clear_all();
}