#include "midi/midi_out_wrapper.h"

#include "midi/midi_mapper_matrix_manager.h"

FLASHMEM void setup_pc_usb();
void read_usb_from_computer();

#define NUM_PC_SOURCES 4

extern source_id_t pc_usb_sources[NUM_PC_SOURCES];
