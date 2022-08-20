#include "Config.h"
#include "ConfigMidi.h"
#include "MidiMappings.h"
#include "midi_outs.h"
#include "midi_out_wrapper.h"

#include "midi_drums.h"
#include "midi_lestrum.h"
#include "behaviour_bamble.h"
#include "midi_chocolatefeet.h"

#ifdef ENABLE_BITBOX
    midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_bitbox      = &ENABLE_BITBOX;
    //MIDIOutputWrapper midi_out_bitbox_wrapper = MIDIOutputWrapper((char*)"S1 : Bitbox : ch 3", midi_out_bitbox, BITBOX_MIDI_CHANNEL);
#endif
//midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_cv12_poly   = &MIDI2;     // output 
//#ifdef ENABLE_BAMBLE
//MIDIDevice *midi_out_cv12_poly   = midi_bamble;
//#endif

#ifdef ENABLE_BASS_TRANSPOSE
    midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_bass      = &ENABLE_BASS_TRANSPOSE;
    //MIDIOutputWrapper midi_out_bass_wrapper = MIDIOutputWrapper((char*)"S3 : Neutron : ch 4", midi_out_bass, BASS_MIDI_CHANNEL);
#endif

#ifdef ENABLE_DRUMKIT
    midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_drumkit      = &ENABLE_DRUMKIT;
    //MIDIOutputWrapper midi_drums_output;
#endif

#ifdef ENABLE_DRUMKIT
    #include "Drums.h"
    // hardcode incoming drumkit on input 2 to go out to drums on bamble
    void drumkit_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
        Serial.printf("drumkit_note_on(\tchannel %i,\tnote %i,\tvelocity %i): ", channel, note, velocity);
        if (note==GM_NOTE_ACOUSTIC_SNARE) note = GM_NOTE_ELECTRIC_SNARE;
        if (note==GM_NOTE_LOW_FLOOR_TOM) note = GM_NOTE_HI_MID_TOM;
        #ifdef ENABLE_BAMBLE
            if (behaviour_bamble->device) {
                //Serial.println("sending!");
                behaviour_bamble->device->sendNoteOn(note, velocity, GM_CHANNEL_DRUMS);
            }
            //midi_drums_output.sendNoteOn(note, velocity);
        #endif
        drums_loop_track.store_event(ticks, midi::NoteOn, note, velocity);
    }
    void drumkit_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
        if (note==GM_NOTE_ACOUSTIC_SNARE)   note = GM_NOTE_ELECTRIC_SNARE;      // map acoustic to electric so drum2musocv will use it
        if (note==GM_NOTE_LOW_FLOOR_TOM)    note = GM_NOTE_HIGH_TOM;            // remap tom 
        #ifdef ENABLE_BAMBLE
            if (behaviour_bamble->device) {
                behaviour_bamble->device->sendNoteOff(note, velocity, GM_CHANNEL_DRUMS);
            }
        #endif
        drums_loop_track.store_event(ticks, midi::NoteOff, note, velocity);
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

    #ifdef ENABLE_CHOCOLATEFEET_SERIAL
        midi_in_footswitch->setHandleNoteOn(footswitch_note_on);
        midi_in_footswitch->setHandleNoteOff(footswitch_note_off);
    #endif
}
