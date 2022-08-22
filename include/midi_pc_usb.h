//#include <USBHost_t36.h>
#include "midi_out_wrapper.h"

#include "midi_mapper_matrix_manager.h"

void setup_pc_usb();
void read_usb_from_computer();

bool check_and_unset_pc_usb_midi_clock_ticked();

#define NUM_PC_INPUTS 4

/*void pc_usb_1_setOutputWrapper(MIDIOutputWrapper *wrapper);
void pc_usb_2_setOutputWrapper(MIDIOutputWrapper *wrapper);
void pc_usb_3_setOutputWrapper(MIDIOutputWrapper *wrapper);
void pc_usb_4_setOutputWrapper(MIDIOutputWrapper *wrapper);*/

//extern MIDIOutputWrapper *pc_usb_outputs[NUM_PC_INPUTS];
extern source_id_t pc_usb_outputs[NUM_PC_INPUTS];

//extern MIDIOutputWrapper *pc_usb_1_output;
//extern MIDIOutputWrapper *pc_usb_2_output;

