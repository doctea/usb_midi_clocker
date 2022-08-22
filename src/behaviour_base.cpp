#include "behaviour_base.h"

#include "midi_mapper_matrix_manager.h"

// called when a note_on message is received from the device
void DeviceBehaviourBase::note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    midi_matrix_manager->send_note_on(this->source_id, note, velocity); //, channel);
}

// called when a note_off message is received from the device
void DeviceBehaviourBase::note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    midi_matrix_manager->send_note_off(this->source_id, note, velocity); //, channel);
}

// called when a control_change message is received from the device
void DeviceBehaviourBase::control_change (uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    midi_matrix_manager->send_control_change(this->source_id, inNumber, inValue); //, inChannel);
}
