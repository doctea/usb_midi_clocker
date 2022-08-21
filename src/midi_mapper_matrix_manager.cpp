#include "midi_mapper_matrix_manager.h"

#include "behaviour_bamble.h"
#include "behaviour_craftsynth.h"

#include "midi_mapper_update_wrapper_menus.h"

#include "midi_pc_usb.h"

MIDIMatrixManager *midi_output_wrapper_manager = nullptr;
MIDIMatrixManager* MIDIMatrixManager::inst_ = nullptr;

MIDIMatrixManager* MIDIMatrixManager::getInstance() {
    if (inst_ == nullptr) {
        inst_ = new MIDIMatrixManager();
    }
    return inst_;
}


// setup stuff... 
void setup_midi_mapper_matrix_manager() {
    // create outputs / initialise to default state...
}