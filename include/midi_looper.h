#ifndef MIDI_LOOPER__INCLUDED
#define MIDI_LOOPER__INCLUDED

#include <MIDI.h>
#include "USBHost_t36.h"

#define LOOP_LENGTH (PPQN*4*4)

void recordInstruction(byte instruction_type, byte channel, byte arg0, byte arg1);
void playInstruction(int index);
void clear_recording();
void stop_all_notes();

/**
 * @var {byte} the maximum number of instructions
 *      its possible to loop around while loops are still playing, if so instructions in the lower loop will be overwritten
 */
const unsigned long MAX_INSTRUCTIONS = (LOOP_LENGTH); //100;
#define MAX_INSTRUCTION_ARGUMENTS 4

/***************************
 * LOOP INSTRUCTION DATA   *
 ***************************/
/**
 * @var {Byte[][]} the instructions for each loop 
 *                 loop_instructions[instruction_index] = byte args[]
 */
extern byte loop_instructions[MAX_INSTRUCTIONS][MAX_INSTRUCTION_ARGUMENTS];

#endif