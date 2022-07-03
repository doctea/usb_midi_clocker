#include "Config.h"
#include "midi_out_wrapper.h"
//#include "midi_bamble.h"
#include "midi_lestrum.h"

#include "midi_mapper.h"

#include "ConfigMidi.h"

#ifdef ENABLE_LESTRUM

//MIDIOutputWrapper *lestrum_pads_output  = &MIDIOutputWrapper((char*)"USB : Bamble : ch 1", &midi_bamble, 1); //midi_out_bitbox_wrapper;
//MIDIOutputWrapper *lestrum_arp_output   = &MIDIOutputWrapper((char*)"USB : Bamble : ch 2", &midi_bamble, 2);
MIDIOutputWrapper *lestrum_arp_output  = &available_outputs[7];
MIDIOutputWrapper *lestrum_pads_output = &available_outputs[6];

void lestrum_pads_setOutputWrapper(MIDIOutputWrapper *output) {
    lestrum_pads_output->stop_all_notes();
    lestrum_pads_output = output;
};
void lestrum_arp_setOutputWrapper(MIDIOutputWrapper *output) {
    lestrum_arp_output->stop_all_notes();
    lestrum_arp_output = output;
};

// configure incoming lestrum to output to midimuso via bamble
void lestrum_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    //Serial.printf("lestrum_note_on(\tchannel %i,\tnote %i,\tvelocity %i): \n", channel, note, velocity);
    if (channel==1) {
        lestrum_arp_output->sendNoteOn(note, 127);
    } else {
        lestrum_pads_output->sendNoteOn(note, 127);
    }
}
void lestrum_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    //Serial.printf("!! lestrum_note_off(\tchannel %i,\tnote %i,\tvelocity %i): \n", channel, note, velocity);
    if (channel==1) {
        lestrum_arp_output->sendNoteOff(note, 127);
    } else {
        lestrum_pads_output->sendNoteOff(note, 127);
    }
}

midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_lestrum      = &ENABLE_LESTRUM;

#endif