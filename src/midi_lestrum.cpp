#include "Config.h"
#include "midi_out_wrapper.h"
#include "midi_bamble.h"
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
        #ifdef ENABLE_BAMBLE
            //Serial.println("channel 1->midi_out_cv12_poly");
            //midi_out_cv12_poly->sendNoteOn(note, 127, 1); //channel);
            //if (ixBamble!=0xFF && midi_bamble) 
            //    midi_bamble->sendNoteOn(note, 127, 1); //channel);
            lestrum_arp_output->sendNoteOn(note, 127);
        #else
            Serial.println("channel 1, but no output device configured");
        #endif
    } else {
        #ifdef ENABLE_BITBOX
            //Serial.println("channel 3->midi_out_bitbox");
            //if (midi_out_bitbox)
            //    midi_out_bitbox->sendNoteOn(note, 127, 3);
            lestrum_pads_output->sendNoteOn(note, 127);
        #else
            Serial.printf("channel %i, but no output device configured\n", channel);
        #endif
    }
}
void lestrum_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    //Serial.printf("!! lestrum_note_off(\tchannel %i,\tnote %i,\tvelocity %i): \n", channel, note, velocity);
    if (channel==1) {
        #ifdef ENABLE_BAMBLE
            //Serial.println("channel 1->midi_out_cv12_poly");
            //if (ixBamble!=0xFF && midi_bamble) 
            //    midi_bamble->sendNoteOff(note, 127, 1); //channel);
            lestrum_arp_output->sendNoteOff(note, 127);
        #else
            Serial.println("channel 1, but no output device configured");
        #endif
    } else {
        #ifdef ENABLE_BITBOX
            /*Serial.println("channel 3->midi_out_bitbox");
            if (midi_out_bitbox)
                midi_out_bitbox->sendNoteOff(note, 127, 3);*/
            lestrum_pads_output->sendNoteOff(note, 127);
        #else
            Serial.printf("channel %i, but no output device configured\n", channel);
        #endif
    }
}

midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_lestrum      = &ENABLE_LESTRUM;

#endif