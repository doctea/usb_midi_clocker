// NOTE: not sure if this serial version works -- didn't when I tried it, but might be a problem with my cable more than anything else

#include "Config.h"
#include "midi_out_wrapper.h"
#include "midi_chocolatefeet.h"

#include "midi_mapper_manager.h"

#include "ConfigMidi.h"

#ifdef ENABLE_CHOCOLATEFEET_SERIAL

//MIDIOutputWrapper *lestrum_arp_output  = nullptr; //midi_output_wrapper_manager->find(7);
//MIDIOutputWrapper *lestrum_pads_output = nullptr; //midi_output_wrapper_manager->find(6);

/*void lestrum_pads_setOutputWrapper(MIDIOutputWrapper *output) {
    lestrum_pads_output->stop_all_notes();
    lestrum_pads_output = output;
};
void lestrum_arp_setOutputWrapper(MIDIOutputWrapper *output) {
    lestrum_arp_output->stop_all_notes();
    lestrum_arp_output = output;
};*/

// configure incoming lestrum to output to midimuso via bamble
void footswitch_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    Serial.printf("!! footswitch_note_on(\tchannel %i,\tnote %i,\tvelocity %i): \n", channel, note, velocity);
    /*if (channel==1) {
        lestrum_arp_output->sendNoteOn(note, 127);
    } else {
        lestrum_pads_output->sendNoteOn(note, 127);
    }*/
}
void footswitch_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    Serial.printf("!! footswitch_note_off(\tchannel %i,\tnote %i,\tvelocity %i): \n", channel, note, velocity);
    /*if (channel==1) {
        lestrum_arp_output->sendNoteOff(note, 127);
    } else {
        lestrum_pads_output->sendNoteOff(note, 127);
    }*/
}

midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_footswitch      = &ENABLE_CHOCOLATEFEET_SERIAL;

#endif