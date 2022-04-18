#include "Config.h"
#include "ConfigMidi.h"
#include "MidiMappings.h"
#include "midi_outs.h"
//#include "midi_bamble.h"

//#ifdef ENABLE_BITBOX
midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_bitbox      = &ENABLE_BITBOX;
//#endif
//midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_cv12_poly   = &MIDI2;     // output 
//#ifdef ENABLE_BAMBLE
//MIDIDevice *midi_out_cv12_poly   = midi_bamble;
//#endif

//#ifdef ENABLE_LESTRUM
midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_lestrum      = &ENABLE_LESTRUM;
//#endif

//#ifdef ENABLE_DRUMKIT
midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_drumkit      = &ENABLE_DRUMKIT;
//#endif


#ifdef ENABLE_LESTRUM
// configure incoming lestrum to output to midimuso via bamble
void lestrum_note_on(byte channel, byte note, byte velocity) {
    Serial.printf("lestrum_note_on(channel %i, note %i, velocity %i): ", channel, note, velocity);
    if (channel==1) {
        Serial.println("channel 1->midi_out_cv12_poly");
        //midi_out_cv12_poly->sendNoteOn(note, 127, 1); //channel);
        if (midi_bamble) 
            midi_bamble->sendNoteOn(note, 127, 1); //channel);
    } else {
        Serial.println("channel 3->midi_out_bitbox");
        if (midi_out_bitbox)
            midi_out_bitbox->sendNoteOn(note, 127, 3);
    }
}
void lestrum_note_off(byte channel, byte note, byte velocity) {
    Serial.printf("lestrum_note_off(channel %i, note %i, velocity %i): ", channel, note, velocity);
    if (channel==1) {
        Serial.println("channel 1->midi_out_cv12_poly");
        if (midi_bamble) 
            midi_bamble->sendNoteOff(note, 127, 1); //channel);
    } else {
        Serial.println("channel 3->midi_out_bitbox");
        if (midi_out_bitbox)
            midi_out_bitbox->sendNoteOff(note, 127, 3);
    }
}
#endif


#ifdef ENABLE_DRUMKIT
// configure incoming drumkit on input 2 to go out to drums on bamble
void drumkit_note_on(byte channel, byte note, byte velocity) {
    Serial.printf("drumkit_note_on(channel %i, note %i, velocity %i): ", channel, note, velocity);
    #ifdef ENABLE_BAMBLE
        if (midi_bamble) {
            Serial.println("sending!");
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
        midi_out_serial[i]->turnThruOff();
    }

    #ifdef ENABLE_LESTRUM
        midi_in_lestrum->setHandleNoteOn(lestrum_note_on);
        midi_in_lestrum->setHandleNoteOff(lestrum_note_off);
    #endif 

    #ifdef ENABLE_DRUMKIT
        midi_in_drumkit->setHandleNoteOn(drumkit_note_on);
        midi_in_drumkit->setHandleNoteOff(drumkit_note_off);
    #endif
}
