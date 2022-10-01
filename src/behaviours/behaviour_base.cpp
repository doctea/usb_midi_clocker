#include "behaviours/behaviour_base.h"

#include "midi/midi_mapper_matrix_manager.h"

// called when a receive_note_on message is received from the device
void DeviceBehaviourUltimateBase::receive_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    midi_matrix_manager->processNoteOn(this->source_id, note, velocity); //, channel);
}

// called when a note_off message is received from the device
void DeviceBehaviourUltimateBase::receive_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    midi_matrix_manager->processNoteOff(this->source_id, note, velocity); //, channel);
}

// called when a receive_control_change message is received from the device
void DeviceBehaviourUltimateBase::receive_control_change (uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    midi_matrix_manager->processControlChange(this->source_id, inNumber, inValue); //, inChannel);
}
