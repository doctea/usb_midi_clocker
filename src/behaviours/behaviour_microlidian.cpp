// Microlidian running on XIAO RP2040

#include "Config.h"

#ifdef ENABLE_MICROLIDIAN

#include "behaviours/behaviour_microlidian.h"

DeviceBehaviour_Microlidian *behaviour_microlidian = nullptr; 

void microlidian_control_change(byte channel, byte number, byte value) {
    behaviour_microlidian->receive_control_change(channel, number, value);
}
void microlidian_note_on(byte channel, byte pitch, byte value) {
    behaviour_microlidian->receive_note_on(channel, pitch, value);
}
void microlidian_note_off(byte channel, byte pitch, byte value) {
    behaviour_microlidian->receive_note_off(channel, pitch, value);
}

void DeviceBehaviour_Microlidian::receive_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    //Serial.printf("DeviceBehaviour_Microlidian::receive_note_on(channel=%i, note=%i, velocity=%i)\n", channel, note, velocity);
    if (channel==GM_CHANNEL_DRUMS)
        midi_matrix_manager->processNoteOn(this->source_id, note, velocity);
    else
        midi_matrix_manager->processNoteOn(this->source_id_2, note, velocity); 
}

// called when a note_off message is received from the device; default behaviour is to pass it on to the midi_matrix_manager to route it
void DeviceBehaviour_Microlidian::receive_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (channel==GM_CHANNEL_DRUMS)
        midi_matrix_manager->processNoteOff(this->source_id, note, velocity); 
    else
        midi_matrix_manager->processNoteOff(this->source_id_2, note, velocity); 
}

#endif