#ifndef MIDI_LESTRUM__INCLUDED
#define MIDI_LESTRUM__INCLUDED

#include "Config.h"
#include "midi_out_wrapper.h"

#include "midi_mapper_matrix_types.h"

#ifdef ENABLE_LESTRUM
    extern source_id_t lestrum_arp_source;
    extern source_id_t lestrum_pads_source;

    // configure incoming lestrum to output to midimuso via bamble
    void lestrum_note_on(uint8_t channel, uint8_t note, uint8_t velocity);
    void lestrum_note_off(uint8_t channel, uint8_t note, uint8_t velocity);

    extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_lestrum; //      = &ENABLE_LESTRUM;
#endif

/*
#ifdef ENABLE_LESTRUM
// configure incoming lestrum to output to midimuso via bamble
void lestrum_note_on(byte channel, byte note, byte velocity);
void lestrum_note_off(byte channel, byte note, byte velocity);
extern MIDIOutputWrapper *lestrum_pads_output, *lestrum_arp_output;
void lestrum_pads_setOutputWrapper(MIDIOutputWrapper *);
void lestrum_arp_setOutputWrapper(MIDIOutputWrapper *);
#endif
*/

#endif