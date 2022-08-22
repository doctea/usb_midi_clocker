#ifndef MIDI_OUT_WRAPPER__INCLUDED
#define MIDI_OUT_WRAPPER__INCLUDED

#include <MIDI.h>
#include <USBHost_t36.h>
#include <SdFat.h>

#include "midi_looper.h"

#define MAX_LENGTH_OUTPUT_WRAPPER_LABEL 30

class MIDITrack;

// generic wrapper around a MIDI output object
// tracks playing notes and provides methods to kill them so can eg kill all notes when transposition is changed
// used by looper tracks, and for echoing beatstep input to the neutron bass output
// TODO: handle transposition here
//      multiple transposition modes:
//          transpose chord? / transpose/quantize within key?
//          transpose key? / outright transposition by semitone amount (ala current midi looper method)
//          tranpose to target octave (ala current beatstep->neutron bass transposition)
// TODO: handle currently_playing_note / last_played_note stuff here too?
// TODO: differentiate between different sources, so that we can eg kill recorded notes when recording stopped while not cutting off any notes that are being played in live
class MIDIOutputWrapper {
    midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output_serialmidi = nullptr;
    MIDIDeviceBase *output_usb = nullptr;
    MIDIDeviceBase **output_usb_pointer = nullptr;  // for late binding usb
    MIDITrack *output_looper = nullptr;
    byte default_channel = 1;

    byte playing_notes[127];

    public:
        bool debug = false;
        char label[MAX_LENGTH_OUTPUT_WRAPPER_LABEL];

        MIDIOutputWrapper(const char *label, midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *in_output_serialmidi, byte channel = 1) {
            strcpy(this->label, label);
            output_serialmidi = in_output_serialmidi;
            default_channel = channel;
        }
        MIDIOutputWrapper(const char *label, MIDIDeviceBase *in_output_usb, byte channel = 1) {
            strcpy(this->label, label);
            output_usb = in_output_usb;
            default_channel = channel;
        }
        MIDIOutputWrapper(const char *label, MIDIDeviceBase **in_output_usb, byte channel = 1) {
            strcpy(this->label, label);
            output_usb_pointer = in_output_usb;
            default_channel = channel;
        }
        MIDIOutputWrapper(const char *label, MIDITrack *looper, byte channel = 1);

        void sendNoteOn(byte pitch, byte velocity, byte channel = 0);
        void sendNoteOff(byte pitch, byte velocity, byte channel = 0);
        void sendControlChange(byte pitch, byte velocity, byte channel = 0);

        inline bool is_note_playing(byte note_number) {
            return playing_notes[note_number]>0;
        }

        void stop_all_notes() {
            Serial.printf("stop_all_notes in %s...\n", label);
            for (int i = 0 ; i < 127 ; i++) {
                while (is_note_playing(i)) {
                    if (this->debug) Serial.printf("stopping %i notes of pitch %i on channel %i..\n", playing_notes[i], i, default_channel);
                    sendNoteOff(i, 0, default_channel);
                }
            }
        }
};

#endif