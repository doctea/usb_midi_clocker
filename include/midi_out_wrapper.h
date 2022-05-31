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
    midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output_serialmidi = nullptr;
    MIDIDevice_BigBuffer *output_usb = nullptr;
    MIDIDevice_BigBuffer **output_usb_pointer = nullptr;  // for late binding usb
    byte default_channel = 1;

    int playing_notes[127];

    public:
        char label[20];

        MIDIOutputWrapper(char *label, midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *in_output_serialmidi, byte channel = 1) {
            strcpy(this->label, label);
            output_serialmidi = in_output_serialmidi;
            default_channel = channel;
        }
        MIDIOutputWrapper(char *label, MIDIDevice_BigBuffer *in_output_usb, byte channel = 1) {
            strcpy(this->label, label);
            output_usb = in_output_usb;
            default_channel = channel;
        }
        MIDIOutputWrapper(char *label, MIDIDevice_BigBuffer **in_output_usb, byte channel = 1) {
            strcpy(this->label, label);
            output_usb_pointer = in_output_usb;
            default_channel = channel;
        }

        void sendNoteOn(byte pitch, byte velocity, byte channel = 0) {
            Serial.printf("sendNoteOn(p=%i, v=%i, c=%i) in %s...\n", pitch, velocity, channel, label); Serial.flush();
            if (channel==0) channel = default_channel;
            if (output_serialmidi)    {
                Serial.println("sendNoteOn using non-null output_serialmidi!"); Serial.flush();
                output_serialmidi->sendNoteOn(pitch, velocity, channel);
                Serial.println("sendNoteOn SENT!"); Serial.flush();
            }
            Serial.println("about to do output_usb test.."); Serial.flush();
            if (output_usb)           {
                Serial.println("sendNoteOn using non-null output_usb!"); Serial.flush();
                output_usb->sendNoteOn(pitch, velocity, channel);
                Serial.println("sendNoteOn SENT!"); Serial.flush();
            }
            Serial.println("did output_usb test!"); Serial.flush();
            Serial.println("about to do output_usb_pointer test.."); Serial.flush();
            if (output_usb_pointer!=nullptr && (*output_usb_pointer)!=nullptr)   {
                Serial.println("sendNoteOn using non-null output_usb_pointer!"); Serial.flush();
                (*output_usb_pointer)->sendNoteOn(pitch, velocity, channel);
                Serial.println("sendNoteOn SENT!"); Serial.flush();
            }
            Serial.println("did output_usb_pointer test.."); Serial.flush();
            Serial.printf("incrementing playing_notes pitch %i\n", pitch); Serial.flush();
            playing_notes[pitch]++;
            Serial.printf("incremented playing_notes pitch %i\n", pitch); Serial.flush(); 
        }

        void sendNoteOff(byte pitch, byte velocity, byte channel = 0) {
            if (channel==0) channel = default_channel;
            if (output_serialmidi!=nullptr)     output_serialmidi->sendNoteOff(pitch, velocity, channel);
            if (output_usb!=nullptr)            output_usb->sendNoteOff(pitch, velocity, channel);
            if (output_usb_pointer!=nullptr && (*output_usb_pointer)!=nullptr)   {
                Serial.println("sendNoteOff using non-null output_usb_pointer!"); Serial.flush();
                (*output_usb_pointer)->sendNoteOff(pitch, velocity, channel);
            }
            if (playing_notes[pitch]>0) playing_notes[pitch]--;
        }

        inline bool is_note_playing(byte note_number) {
            return playing_notes[note_number]>0;
        }

        void stop_all_notes() {
            Serial.printf("stop_all_notes in %s...\n", label);
            for (int i = 0 ; i < 127 ; i++) {
                while (is_note_playing(i)) {
                    Serial.printf("stopping %i notes of pitch %i on channel %i..\n", playing_notes[i], i, default_channel);
                    sendNoteOff(i, 0, default_channel);
                }
            }
        }

};

#endif