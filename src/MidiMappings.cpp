#include "Config.h"
#include "ConfigMidi.h"
#include "MidiMappings.h"
#include "midi_outs.h"
#include "midi_out_wrapper.h"

#include "midi_drums.h"
//#include "midi_lestrum.h"
#include "behaviour_bamble.h"
#include "midi_chocolatefeet.h"

#include "behaviour_lestrum.h"

#ifdef ENABLE_BITBOX
    midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_bitbox      = &ENABLE_BITBOX;
#endif

#ifdef ENABLE_BASS_TRANSPOSE
    midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_bass        = &ENABLE_BASS_TRANSPOSE;
#endif

#ifdef ENABLE_DRUMKIT
    midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_in_drumkit      = &ENABLE_DRUMKIT;
#endif

source_id_t drumkit_source_id = -1;

#ifdef ENABLE_DRUMKIT
    #include "Drums.h"
    // hardcode incoming drumkit on input 2 to go out to drums on bamble
    void drumkit_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
        Serial.printf("drumkit_note_on(\tchannel %i,\tnote %i,\tvelocity %i): ", channel, note, velocity);
        if (note==GM_NOTE_ACOUSTIC_SNARE)   note = GM_NOTE_ELECTRIC_SNARE;
        if (note==GM_NOTE_LOW_FLOOR_TOM)    note = GM_NOTE_HI_MID_TOM;

        midi_matrix_manager->processNoteOn(drumkit_source_id, note, velocity);
    }
    void drumkit_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
        if (note==GM_NOTE_ACOUSTIC_SNARE)   note = GM_NOTE_ELECTRIC_SNARE;      // map acoustic to electric so drum2musocv will use it
        if (note==GM_NOTE_LOW_FLOOR_TOM)    note = GM_NOTE_HIGH_TOM;            // remap tom 

        midi_matrix_manager->processNoteOff(drumkit_source_id, note, velocity);
    }
#endif

void setup_midi_serial_devices() {
    // TODO: midi -- move this to behaviourserialbase setup
    for (int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
        midi_out_serial[i]->begin(MIDI_CHANNEL_OMNI);
        midi_out_serial[i]->turnThruOff();
    }

    /*#ifdef ENABLE_LESTRUM
        ENABLE_LESTRUM.setHandleNoteOn(lestrum_note_on);
        ENABLE_LESTRUM.setHandleNoteOff(lestrum_note_off);
    #endif */

    #ifdef ENABLE_DRUMKIT
        midi_in_drumkit->setHandleNoteOn(drumkit_note_on);
        midi_in_drumkit->setHandleNoteOff(drumkit_note_off);
    #endif

    #ifdef ENABLE_CHOCOLATEFEET_SERIAL
        midi_in_footswitch->setHandleNoteOn(footswitch_note_on);
        midi_in_footswitch->setHandleNoteOff(footswitch_note_off);
    #endif
}
