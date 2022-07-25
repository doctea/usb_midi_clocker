//#include <USBHost_t36.h>
#include "midi_out_wrapper.h"

void setup_pc_usb();
void read_usb_from_computer();

void pc_usb_1_setOutputWrapper(MIDIOutputWrapper *wrapper);
void pc_usb_2_setOutputWrapper(MIDIOutputWrapper *wrapper);

bool check_and_unset_pc_usb_midi_clock_ticked();

extern MIDIOutputWrapper *pc_usb_1_output;
extern MIDIOutputWrapper *pc_usb_2_output;
