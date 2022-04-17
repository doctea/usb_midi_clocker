#ifndef MIDIMAPPINGS_INCLUDED
#define MIDIMAPPINGS_INCLUDED

#include "midi_outs.h"

// configure incoming lestrum to output to midimuso via bamble
void lestrum_note_on(byte channel, byte note, byte velocity) {
    if (midi_out_cv12_poly) {
        Serial.printf("lestrum_note_on(channel %i, note %i, velocity %i)\n", channel, note, velocity);
        midi_out_cv12_poly->sendNoteOn(note, 127, 1); //channel);
    }
}
void lestrum_note_off(byte channel, byte note, byte velocity) {
    if (midi_out_cv12_poly) {
        midi_out_cv12_poly->sendNoteOff(note, 0, 1); //channel);
    }
}

void setup_midi_serial_devices() {
    for (int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
        midi_outs[i]->begin();
    }

    midi_in_lestrum->setHandleNoteOn(lestrum_note_on);
}


#endif