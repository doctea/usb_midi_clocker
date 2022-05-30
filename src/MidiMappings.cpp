#include "Config.h"
#include "ConfigMidi.h"
#include "MidiMappings.h"
#include "midi_outs.h"
#include "midi_out_wrapper.h"
//#include "midi_bamble.h"

#ifdef ENABLE_BITBOX
midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_bitbox      = &ENABLE_BITBOX;
MIDIOutputWrapper midi_out_bitbox_wrapper = MIDIOutputWrapper("bitbox [ch3]", midi_out_bitbox, BITBOX_MIDI_CHANNEL);
#endif
//midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_cv12_poly   = &MIDI2;     // output 
//#ifdef ENABLE_BAMBLE
//MIDIDevice *midi_out_cv12_poly   = midi_bamble;
//#endif

#ifdef ENABLE_BASS_TRANSPOSE
midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_bass      = &ENABLE_BASS_TRANSPOSE;
MIDIOutputWrapper midi_out_bass_wrapper = MIDIOutputWrapper("neutron [ch4]", midi_out_bass, BASS_MIDI_CHANNEL);
#endif

#ifdef ENABLE_LESTRUM
midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_lestrum      = &ENABLE_LESTRUM;
#endif

#ifdef ENABLE_DRUMKIT
midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_drumkit      = &ENABLE_DRUMKIT;
#endif


#ifdef ENABLE_LESTRUM
// configure incoming lestrum to output to midimuso via bamble
void lestrum_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    Serial.printf("lestrum_note_on(\tchannel %i,\tnote %i,\tvelocity %i): ", channel, note, velocity);
    if (channel==1) {
        #ifdef ENABLE_BAMBLE
            Serial.println("channel 1->midi_out_cv12_poly");
            //midi_out_cv12_poly->sendNoteOn(note, 127, 1); //channel);
            if (ixBamble!=0xFF && midi_bamble) 
                midi_bamble->sendNoteOn(note, 127, 1); //channel);
        #else
            Serial.println("channel 1, but no output device configured");
        #endif
    } else {
        #ifdef ENABLE_BITBOX
            Serial.println("channel 3->midi_out_bitbox");
            if (midi_out_bitbox)
                midi_out_bitbox->sendNoteOn(note, 127, 3);
        #else
            Serial.printf("channel %i, but no output device configured\n", channel);
        #endif
    }
}
void lestrum_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    Serial.printf("!! lestrum_note_off(\tchannel %i,\tnote %i,\tvelocity %i): ", channel, note, velocity);
    if (channel==1) {
        #ifdef ENABLE_BAMBLE
            Serial.println("channel 1->midi_out_cv12_poly");
            if (ixBamble!=0xFF && midi_bamble) 
                midi_bamble->sendNoteOff(note, 127, 1); //channel);
        #else
            Serial.println("channel 1, but no output device configured");
        #endif
    } else {
        #ifdef ENABLE_BITBOX
            Serial.println("channel 3->midi_out_bitbox");
            if (midi_out_bitbox)
                midi_out_bitbox->sendNoteOff(note, 127, 3);
        #else
            Serial.printf("channel %i, but no output device configured\n", channel);
        #endif
    }
}
#endif


#ifdef ENABLE_DRUMKIT
// configure incoming drumkit on input 2 to go out to drums on bamble
void drumkit_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    Serial.printf("drumkit_note_on(\tchannel %i,\tnote %i,\tvelocity %i): ", channel, note, velocity);
    #ifdef ENABLE_BAMBLE
        if (midi_bamble) {
            Serial.println("sending!");
            midi_bamble->sendNoteOn(note&0b01111111, velocity&0b01111111, 10);
        }
    #endif
}
void drumkit_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    #ifdef ENABLE_BAMBLE
        if (midi_bamble) {
            midi_bamble->sendNoteOff(note&0b01111111, velocity&0b01111111, 10);
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
