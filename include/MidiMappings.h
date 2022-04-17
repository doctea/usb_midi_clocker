#ifndef MIDIMAPPINGS__INCLUDED
#define MIDIMAPPINGS__INCLUDED

#define MIDI_IN_NUMBER_LESTRUM 0

#include "Config.h"
#include "midi_outs.h"

#include "midi_beatstep.h"
#include "midi_apcmini.h"
#include "midi_bamble.h"

#ifdef ENABLE_LESTRUM
// configure incoming lestrum to output to midimuso via bamble
void lestrum_note_on(byte channel, byte note, byte velocity) {
    if (midi_out_cv12_poly) {
        Serial.printf("lestrum_note_on(channel %i, note %i, velocity %i)\n", channel, note, velocity);
        if (channel==1) {
            midi_out_cv12_poly->sendNoteOn(note, 127, 1); //channel);
        } else {
            midi_bamble->sendNoteOn(note, 127, 3);
        }
    }
}
void lestrum_note_off(byte channel, byte note, byte velocity) {
    if (midi_out_cv12_poly) {
        if (channel==1) {
            midi_out_cv12_poly->sendNoteOff(note, 0, 1); //channel);
        } else {
            midi_bamble->sendNoteOff(note, 127, 3);
        }
    }
}
#endif


#ifdef ENABLE_DRUMKIT
// configure incoming drumkit on input 2 to go out to drums on bamble
void drumkit_note_on(byte channel, byte note, byte velocity) {
    #ifdef ENABLE_BAMBLE
        if (midi_bamble) {
            midi_bamble->sendNoteOn(note, velocity, 10);
        }
    #endif
}
void drumkit_note_off(byte channel, byte note, byte velocity) {
    #ifdef ENABLE_BAMBLE
        if (midi_bamble) {
            midi_bamble->sendNoteOff(note, velocity, 10);
        }
    #endif
}
#endif


void setup_midi_serial_devices() {
    for (int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
        midi_out_serial[i]->begin(MIDI_CHANNEL_OMNI);
    }

    #ifdef ENABLE_LESTRUM
        midi_in_lestrum->setHandleNoteOn(lestrum_note_on);
        midi_in_lestrum->setHandleNoteOff(lestrum_note_off);
    #endif 

    #ifdef ENABLE_DRUMKIT
        midi_in_drumkit->setHandleNoteOn(drumkit_note_on);
        midi_in_drumkit->setHandleNoteOn(drumkit_note_off);
    #endif
}


#endif