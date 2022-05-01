#include <midi.h>
#include <MIDI.h>

#include "bpm.h"
#include "midi_looper.h"

#include "Config.h"
#include "ConfigMidi.h"
#include "midi_outs.h"

// TODO: rewrite all this to work a lot better, ie:-
//      >1 event per tick
//      record as midi file
//      load as midi file
//      make stop_all_notes
//      able to track multiple devices / channels
//      transpose
//      quantizing (time)
//      quantizing (scale)

/*typedef struct midi_frame {
    //unsigned long time;
    LinkedList<midi_message> messages = LinkedList<midi_message>();
};*/

midi_track mpk49_loop_track = midi_track();

// from https://github.com/LesserChance/arduino-midi-looper/blob/master/instruction.ino

void stop_all_notes() {
    mpk49_loop_track.stop_all_notes();
}

void clear_recording() {
    mpk49_loop_track.stop_all_notes();
    mpk49_loop_track.clear_all();
}