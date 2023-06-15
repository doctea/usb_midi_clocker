// XIAO 

#include "Config.h"

#ifdef ENABLE_MIDILIGHTS

#include "behaviours/behaviour_MIDILights.h"

//DeviceBehaviour_CraftSynth behaviour_craftsynth_actual = DeviceBehaviour_CraftSynth();
DeviceBehaviour_MIDILights *behaviour_midilights = nullptr; //new DeviceBehaviour_CraftSynth(); //new DeviceBehaviour_CraftSynth(); //nullptr;
/*
void xiao_control_change(byte channel, byte number, byte value) {
    behaviour_xiao->receive_control_change(channel, number, value);
}
void xiao_note_on(byte channel, byte pitch, byte value) {
    behaviour_xiao->receive_note_on(channel, pitch, value);
}
void xiao_note_off(byte channel, byte pitch, byte value) {
    behaviour_xiao->receive_note_off(channel, pitch, value);
}

void DeviceBehaviour_XIAO::receive_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    //Serial.printf("DeviceBehaviour_XIAO::receive_note_on(channel=%i, note=%i, velocity=%i)\n", channel, note, velocity);
    if (channel==10)
        midi_matrix_manager->processNoteOn(this->source_id, note, velocity); //, channel);
    else
        midi_matrix_manager->processNoteOn(this->source_id_2, note, velocity); //, channel);
}

// called when a note_off message is received from the device; default behaviour is to pass it on to the midi_matrix_manager to route it
void DeviceBehaviour_XIAO::receive_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (channel==10)
        midi_matrix_manager->processNoteOff(this->source_id, note, velocity); //, channel);
    else
        midi_matrix_manager->processNoteOff(this->source_id_2, note, velocity); //, channel);
}
*/

/*void craftsynth_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    if (behaviour_craftsynth!=nullptr) behaviour_craftsynth->receive_control_change(inChannel, inNumber, inValue);
}*/

/*void craftsynth_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_craftsynth!=nullptr) behaviour_craftsynth->receive_note_on(inChannel, inNumber, inVelocity);
}
void craftsynth_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_craftsynth!=nullptr) behaviour_craftsynth->note_off(inChannel, inNumber, inVelocity);
}*/

/*MIDIOutputWrapper *craftsynth_output = &midi_out_bitbox_wrapper;
void craftsynth_setOutputWrapper(MIDIOutputWrapper *wrapper) {
  craftsynth_output->stop_all_notes();
  craftsynth_output = wrapper;    
}*/

#endif