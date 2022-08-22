#include "Config.h"
#include "midi_out_wrapper.h"
#include "midi_lestrum.h"

#include "midi_mapper_matrix_manager.h"

#include "ConfigMidi.h"

#ifdef ENABLE_LESTRUM

//MIDIOutputWrapper *lestrum_arp_output  = &available_outputs[7];
//MIDIOutputWrapper *lestrum_pads_output = &available_outputs[6];
//MIDIOutputWrapper *lestrum_arp_output  = nullptr; //midi_output_wrapper_manager->find(7);
//MIDIOutputWrapper *lestrum_pads_output = nullptr; //midi_output_wrapper_manager->find(6);
source_id_t lestrum_arp_source = -1;
source_id_t lestrum_pads_source = -1;

/*void lestrum_pads_setOutputWrapper(MIDIOutputWrapper *output) {
    lestrum_pads_output->stop_all_notes();
    lestrum_pads_output = output;
};
void lestrum_arp_setOutputWrapper(MIDIOutputWrapper *output) {
    lestrum_arp_output->stop_all_notes();
    lestrum_arp_output = output;
};*/

// configure incoming lestrum to output to midimuso via bamble
void lestrum_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    //Serial.printf("lestrum_note_on(\tchannel %i,\tnote %i,\tvelocity %i): \n", channel, note, velocity);
    if (channel==1) {
        midi_matrix_manager->send_note_on(lestrum_arp_source, note, 127);
        //lestrum_arp_output->sendNoteOn(note, 127);
    } else {
        midi_matrix_manager->send_note_on(lestrum_pads_source, note, 127);
        //lestrum_pads_output->sendNoteOn(note, 127);
    }
}
void lestrum_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    //Serial.printf("!! lestrum_note_off(\tchannel %i,\tnote %i,\tvelocity %i): \n", channel, note, velocity);
    if (channel==1) {
        midi_matrix_manager->send_note_off(lestrum_arp_source, note, 0);
        //lestrum_arp_output->sendNoteOff(note, 0);
    } else {
        midi_matrix_manager->send_note_off(lestrum_pads_source, note, 0);
        //lestrum_pads_output->sendNoteOff(note, 0);
    }
}

midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_lestrum      = &ENABLE_LESTRUM;

#endif