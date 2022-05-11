#ifndef MIDI_OUT_WRAPPER__INCLUDED
#define MIDI_OUT_WRAPPER__INCLUDED

#include <MIDI.h>
#include <USBHost_t36.h>

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
    midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output_serialmidi;
    MIDIDevice_BigBuffer *output_usb;
    byte default_channel = 1;

    bool playing_notes[127];

    public:
        MIDIOutputWrapper(midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *in_output_serialmidi, byte channel = 1) {
            output_serialmidi = in_output_serialmidi;
            default_channel = channel;
        }
        MIDIOutputWrapper(MIDIDevice_BigBuffer *in_output_usb, byte channel = 1) {
            output_usb = in_output_usb;
            default_channel = channel;
        }

        void sendNoteOn(byte pitch, byte velocity, byte channel = 0) {
            if (channel==0) channel = default_channel;
            if (output_serialmidi) output_serialmidi->sendNoteOn(pitch, velocity, channel);
            if (output_usb)        output_usb->sendNoteOn(pitch, velocity, channel);
            playing_notes[pitch] = true;
        }

        void sendNoteOff(byte pitch, byte velocity, byte channel = 0) {
            if (channel==0) channel = default_channel;
            if (output_serialmidi) output_serialmidi->sendNoteOff(pitch, velocity, channel);
            if (output_usb)        output_usb->sendNoteOff(pitch, velocity, channel);
            playing_notes[pitch] = false;
        }

        inline bool is_note_playing(byte note_number) {
            return playing_notes[note_number];
        }

        void stop_all_notes() {
            for (int i = 0 ; i < 127 ; i++) {
                if (is_note_playing(i))
                    sendNoteOff(i, 0, default_channel);
            }
        }

};

#endif