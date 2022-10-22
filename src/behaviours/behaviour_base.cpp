#include "behaviours/behaviour_base.h"

#include "midi/midi_mapper_matrix_manager.h"


// called when a receive_note_on message is received from the device; default behaviour is to pass it on to the midi_matrix_manager to route it
void DeviceBehaviourUltimateBase::receive_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    midi_matrix_manager->processNoteOn(this->source_id, note, velocity); //, channel);
}

// called when a note_off message is received from the device; default behaviour is to pass it on to the midi_matrix_manager to route it
void DeviceBehaviourUltimateBase::receive_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    midi_matrix_manager->processNoteOff(this->source_id, note, velocity); //, channel);
}

// called when a receive_control_change message is received from the device; default behaviour is to pass it on to the midi_matrix_manager to route it
void DeviceBehaviourUltimateBase::receive_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    if (this->debug) {
        Serial.printf(F("%s: DeviceBehaviourUltimateBase::receive_control_change(%i, %i, %i)...\n"), this->get_label(), inChannel, inNumber, inValue);
        if (midi_matrix_manager==nullptr) {
            Serial.println(F("YO, midi_matrix_manager is null?!")); Serial.flush();
        }
    }
    midi_matrix_manager->processControlChange(this->source_id, inNumber, inValue); //, inChannel);
    if (this->debug) Serial.printf(F("...DeviceBehaviourUltimateBase::receive_control_change(%i, %i, %i) done!\n"), this->get_label(), inChannel, inNumber, inValue);
}
