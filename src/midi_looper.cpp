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

/**
 * Record the passed instruction in the current position
 */
void recordInstruction(byte instruction_type, byte channel, byte arg0, byte arg1) {
    Serial.printf("Recording event at %i: ",(ticks%(LOOP_LENGTH)));
    int loop_record_instruction_position = ticks%(LOOP_LENGTH);
    //store the args in the current position
    loop_instructions[loop_record_instruction_position][0] = instruction_type;
    loop_instructions[loop_record_instruction_position][1] = channel;
    loop_instructions[loop_record_instruction_position][2] = arg0;
    loop_instructions[loop_record_instruction_position][3] = arg1;

    mpk49_loop_track.record_event(loop_record_instruction_position, instruction_type, channel, arg0, arg1);
}

/**
 * Playback a specific instruction
 */
void playInstruction(int index) {
    bool debug = false;
    index = index%LOOP_LENGTH;

    mpk49_loop_track.play_events(index, midi_out_bitbox);

    /*
    if (is_bpm_on_beat(index)) {
        if (debug) Serial.printf("Beat for %i\n", step_number_from_ticks(index));
    }
    if (loop_instructions[index][0] == midi::NoteOn) {
        if (debug) Serial.printf("index %i note on\n", index);
        //byte channel  = loop_instructions[index][1];
        byte pitch    = loop_instructions[index][2];
        byte velocity = loop_instructions[index][3];
        
        //Send midi instruction
        //sendMidiNoteOn(channel, pitch, velocity);
        midi_out_bitbox->sendNoteOn(pitch, velocity, 3);
        
        //Send pad instruction
        //sendPadNoteOn(channel, pitch, velocity);
    } else if (loop_instructions[index][0] == midi::NoteOff) {
        //byte channel  = loop_instructions[index][1];
        byte pitch    = loop_instructions[index][2];
        byte velocity = loop_instructions[index][3];
        
        //Send midi instruction
        //sendMidiNoteOff(channel, pitch, velocity);
        midi_out_bitbox->sendNoteOff(pitch, velocity, 3);
        
        //Send pad instruction
        //sendPadNoteOff(channel, pitch, velocity);
    }*/
}
