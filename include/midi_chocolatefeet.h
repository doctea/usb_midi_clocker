#include "Config.h"
#include "midi_out_wrapper.h"

#ifdef ENABLE_CHOCOLATEFEET_SERIAL
    /*extern MIDIOutputWrapper *lestrum_pads_output; 
    extern MIDIOutputWrapper *lestrum_arp_output;
    void lestrum_pads_setOutputWrapper(MIDIOutputWrapper *output);
    void lestrum_arp_setOutputWrapper(MIDIOutputWrapper *output);*/

    // configure incoming footswitch to turn on looping, etc
    void footswitch_note_on(uint8_t channel, uint8_t note, uint8_t velocity);
    void footswitch_note_off(uint8_t channel, uint8_t note, uint8_t velocity);

    extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_footswitch; //      = &ENABLE_LESTRUM;

#endif
