#include "Config.h"
#include "midi_out_wrapper.h"

#ifdef ENABLE_LESTRUM
    extern MIDIOutputWrapper *lestrum_pads_output; 
    extern MIDIOutputWrapper *lestrum_arp_output;
    void lestrum_pads_setOutputWrapper(MIDIOutputWrapper *output);
    void lestrum_arp_setOutputWrapper(MIDIOutputWrapper *output);

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