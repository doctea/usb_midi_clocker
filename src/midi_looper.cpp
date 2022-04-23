#include <midi.h>

#include "bpm.h"
#include "midi_looper.h"

#include "Config.h"
#include "ConfigMidi.h"
#include "midi_outs.h"

byte loop_instructions[MAX_INSTRUCTIONS][MAX_INSTRUCTION_ARGUMENTS];

// from https://github.com/LesserChance/arduino-midi-looper/blob/master/instruction.ino

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
    
    //store the time the event occurred
    //loop_instruction_times[loop_record_instruction_position] = (ticks%(LOOP_LENGTH)); // - loop_start[0]; //getRecordingPosition();
    
    //go to the next instruction position
    /*if (loop_record_instruction_position == MAX_INSTRUCTIONS - 1) {
        loop_record_instruction_position = 0;
    } else {
        loop_record_instruction_position++;
    }*/
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
        byte channel  = loop_instructions[index][1];
        byte pitch    = loop_instructions[index][2];
        byte velocity = loop_instructions[index][3];
        
        //Send midi instruction
        //sendMidiNoteOn(channel, pitch, velocity);
        midi_out_bitbox->sendNoteOn(pitch, velocity, 3);
        
        //Send pad instruction
        //sendPadNoteOn(channel, pitch, velocity);
    } else if (loop_instructions[index][0] == midi::NoteOff) {
        byte channel  = loop_instructions[index][1];
        byte pitch    = loop_instructions[index][2];
        byte velocity = loop_instructions[index][3];
        
        //Send midi instruction
        //sendMidiNoteOff(channel, pitch, velocity);
        midi_out_bitbox->sendNoteOff(pitch, velocity, 3);
        
        //Send pad instruction
        //sendPadNoteOff(channel, pitch, velocity);
    }
}

/*
 * Playback a specific loop
 */
/*void playLoop(int index) {
    //unsigned int last_playback_position = loop_playback_position[index];
    unsigned int playback_position = (ticks%(LOOP_LENGTH)); //getPlaybackPosition(index);

    //play any commands between the last time this was checked and now
    if (playback_position != last_playback_position) {
        byte i = loop_instruction_index_start[index];
        boolean done_checking = false;

        while (!done_checking) {
            unsigned int check_time = loop_instruction_times[i];
            if (check_time >= last_playback_position && check_time < playback_position) {
                playInstruction(i);
            }

            if (i == loop_instruction_index_end[index] || check_time >= playback_position) {
                //no need to check any further
                done_checking = true;
            } else {
                //ready the check for the next instruction
                i++;
                if (i == MAX_INSTRUCTIONS) {
                    i = 0;
                }
            }
        }

        loop_playback_position[index] = playback_position;
    }

}*/