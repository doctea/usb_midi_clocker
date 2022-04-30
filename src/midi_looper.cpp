#include <midi.h>

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

byte loop_instructions[MAX_INSTRUCTIONS][MAX_INSTRUCTION_ARGUMENTS];

// from https://github.com/LesserChance/arduino-midi-looper/blob/master/instruction.ino

void stop_all_notes() {
    for (int i = 0 ; i < LOOP_LENGTH ; i++) {
        if (loop_instructions[i][0]==midi::NoteOn) {
            midi_out_bitbox->sendNoteOff(
                loop_instructions[i][2], 
                loop_instructions[i][3], 
                3
            );
        }
    }
}

void clear_recording() {
    for (int i = 0 ; i < LOOP_LENGTH ; i++) {
        // turn off notes that might be playing
        if (loop_instructions[i][0]==midi::NoteOn) {
            midi_out_bitbox->sendNoteOff(
                loop_instructions[i][2], 
                loop_instructions[i][3], 
                3
            );
        }
        // clear the note
        loop_instructions[i][0] = 0;
    }
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
}

/**
 * Playback a specific instruction
 */
void playInstruction(int index) {
    bool debug = false;
    index = index%LOOP_LENGTH;
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
    }
}
