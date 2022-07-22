// Modal CraftSynth v2.0 vid=04D8 pid=EE1F

#include "Config.h"

#ifdef ENABLE_CRAFTSYNTH_USB

#include "behaviour_craftsynth.h"

//DeviceBehaviour_CraftSynth behaviour_craftsynth_actual = DeviceBehaviour_CraftSynth();
DeviceBehaviour_CraftSynth *behaviour_craftsynth = new DeviceBehaviour_CraftSynth(); //new DeviceBehaviour_CraftSynth(); //nullptr;

/*void craftsynth_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    if (behaviour_craftsynth!=nullptr) behaviour_craftsynth->control_change(inChannel, inNumber, inValue);
}*/

/*void craftsynth_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_craftsynth!=nullptr) behaviour_craftsynth->note_on(inChannel, inNumber, inVelocity);
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