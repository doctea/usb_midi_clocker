#ifndef MIDI_OUT_WRAPPER__INCLUDED
#define MIDI_OUT_WRAPPER__INCLUDED

#include <MIDI.h>
#include <USBHost_t36.h>
#include <SdFat.h>

#include "midi/midi_looper.h"
#include "midi/midi_helpers.h"

#define MAX_LENGTH_OUTPUT_WRAPPER_LABEL 30

class MIDITrack;

// generic wrapper around a MIDI output object
// tracks playing notes and provides methods to kill them so can eg kill all notes when transposition is changed
// used by looper tracks, and for echoing beatstep input to the neutron bass output
// DONE: handle transposition here
// TODO: multiple transposition modes:
//          transpose chord? / transpose/quantize within key?
//          transpose key? / outright transposition by semitone amount (ala current midi looper method)
// DONE:    tranpose to target octave (ala current beatstep->neutron bass transposition)
// DONE: handle currently_playing_note / last_played_note stuff here too?
// TODO: differentiate between different sources, so that we can eg kill recorded notes when recording stopped while not cutting off any notes that are being played in live
//          ^^ think this is better handled by the midi matrix manager..?
class MIDIOutputWrapper {
    /*midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output_serialmidi = nullptr;
    MIDIDeviceBase *output_usb = nullptr;
    MIDIDeviceBase **output_usb_pointer = nullptr;  // for late binding usb
    MIDITrack *output_looper = nullptr;*/

    int force_octave = -1;

    public:
        bool debug = false;

        byte default_channel = 0;
        char label[MAX_LENGTH_OUTPUT_WRAPPER_LABEL];

        byte playing_notes[127];

        int last_note = -1, current_note = -1;
        int last_transposed_note = -1, current_transposed_note = -1;

        MIDIOutputWrapper(const char *label, byte channel = 1) {
            strcpy(this->label, label);
            default_channel = channel;
            memset(playing_notes, 0, sizeof(playing_notes));
        }
        virtual ~MIDIOutputWrapper();
        //virtual ~MIDIOutputWrapper();
        /*MIDIOutputWrapper(const char *label, midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *in_output_serialmidi, byte channel = 1) : MIDIOutputWrapper(label, channel) {
            output_serialmidi = in_output_serialmidi;
        }
        MIDIOutputWrapper(const char *label, MIDIDeviceBase *in_output_usb, byte channel = 1) : MIDIOutputWrapper(label, channel) {
            output_usb = in_output_usb;
        }
        MIDIOutputWrapper(const char *label, MIDIDeviceBase **in_output_usb, byte channel = 1) : MIDIOutputWrapper(label, channel) {
            output_usb_pointer = in_output_usb;
        }
        MIDIOutputWrapper(const char *label, MIDITrack *looper, byte channel = 1);*/

        // remap pitch if force octave is on, TODO: other tranposition modes
        virtual int recalculate_pitch(byte note) {
            if (this->force_octave>=0) {
                // send incoming notes from beatstep back out to neutron on serial3, but transposed down
                uint8_t note2 = note % 12;
                note2 += (force_octave*12); //24;
                //Serial.printf("beatstep note on %i : %i : %i\n", BASS_MIDI_CHANNEL, note, velocity);
                //Serial.printf("beatstep note2 is %i\n", note2);
                note = note2;
            }
            return note;
        }

        //virtual void sendNoteOn(byte pitch, byte velocity, byte channel = 0);
        //virtual void sendNoteOff(byte pitch, byte velocity, byte channel = 0);

        virtual void sendNoteOn(byte in_pitch, byte velocity, byte channel) {
            if (this->debug) 
                Serial.printf("sendNoteOn(p=%i, v=%i, c=%i) in %s...\n", in_pitch, velocity, channel, label); Serial.flush();

            current_note = in_pitch;
            int pitch = recalculate_pitch(in_pitch);
            if (!is_valid_note(pitch)) return;

            if (playing_notes[pitch]<8) {
                //if (this->debug) Serial.printf("\tplaying_notes[%i] is already %i -- increasing by 1\n", pitch, playing_notes[pitch]);
                playing_notes[pitch]++;
            } else {
                //if (this->debug) Serial.printf("\talready playing %i notes at pitch %i, so not counting a new one\n", playing_notes[pitch], pitch);
            }

            current_transposed_note = pitch;

            if (channel==0) channel = default_channel;

            this->actual_sendNoteOn(pitch, velocity, channel);
        }

        virtual void sendNoteOff(byte in_pitch, byte velocity, byte channel) {
            this->last_note = in_pitch;
            if (this->current_note==in_pitch) 
                current_note = -1;

            int pitch = recalculate_pitch(in_pitch);

            //if (this->debug) Serial.printf("MIDIOutputWrapper:sendNoteOff(%i, %i, %i) current count is %i\n", pitch, velocity, channel, playing_notes[pitch]);

            if (pitch<0 || pitch>127) return;
            if (playing_notes[pitch]>0) playing_notes[pitch]--;
            if (playing_notes[pitch]!=0) return;

            this->last_transposed_note = pitch;
            if (this->current_transposed_note==pitch)
                current_transposed_note = -1;

            if (channel==0) channel = default_channel;

            this->actual_sendNoteOff(pitch, velocity, channel);

            this->last_transposed_note = pitch;
            if (this->current_transposed_note==pitch)
                current_transposed_note = -1;
        }

        virtual void sendControlChange(byte pitch, byte velocity, byte channel = 0) {
            if (channel == 0 ) channel = this->default_channel;
            this->actual_sendControlChange(pitch, velocity, channel);
        };

        virtual void sendPitchBend(int bend, byte channel = 0) {
            if (channel == 0 ) channel = this->default_channel;
            this->actual_sendPitchBend(bend, channel);
        }

        // these are the parts that actually send using the underlying objects -- split out so that can override in subclasses
        virtual void actual_sendControlChange(byte pitch, byte velocity, byte channel) {};
        virtual void actual_sendNoteOn(byte pitch, byte velocity, byte channel) {};
        virtual void actual_sendNoteOff(byte pitch, byte velocity, byte channel) {};
        virtual void actual_sendPitchBend(int pitch, byte channel) {};

        virtual inline bool is_note_playing(int pitch) {
            pitch = recalculate_pitch(pitch);
            if (!is_valid_note(pitch)) return false;
            return playing_notes[pitch]>0;
        }

        virtual void stop_all_notes(bool force = false) {
            if (this->debug) Serial.printf(F("stop_all_notes in %s...\n"), label);
            sendControlChange(midi::AllNotesOff, 127);
            for (int pitch = 0 ; pitch < 127 ; pitch++) {
                //int pitch = recalculate_pitch(i);

                if (is_note_playing(pitch)) {
                    //if (this->debug) 
                    //if (this->debug) Serial.printf("Got %i notes of pitch %i to stop on channel %i..\n", playing_notes[pitch], pitch, default_channel);
                    if(force) sendNoteOff(pitch, 0, default_channel);
                    playing_notes[pitch] = 0;
                }
            }
        }

        virtual void setForceOctave(int octave) {
            //Serial.printf("Beatstep_Behaviour#setForceOctave(%i)!", octave); Serial.flush();
            if (octave!=this->force_octave) {
                this->stop_all_notes();
                //midi_matrix_manager->stop_all_notes(source_id);
                this->force_octave = octave;
            }
        }
        virtual int getForceOctave() {
            //Serial.println("Beatstep_Behaviour#getForceOctave!"); Serial.flush();
            return this->force_octave;
        }
};

class MIDIOutputWrapper_PC : public MIDIOutputWrapper {
    public:
    byte cable_number = 0;

    MIDIOutputWrapper_PC(const char *label, byte cable_number, byte channel = 1) : MIDIOutputWrapper(label, channel) {
        this->cable_number = cable_number;
    }

    virtual void actual_sendNoteOn(byte pitch, byte velocity, byte channel) override {
        usbMIDI.sendNoteOn(pitch, velocity, channel, this->cable_number);
    }

    virtual void actual_sendNoteOff(byte pitch, byte velocity, byte channel) override {
        usbMIDI.sendNoteOff(pitch, velocity, channel, this->cable_number);
    }

    virtual void actual_sendControlChange(byte pitch, byte velocity, byte channel) override {
        usbMIDI.sendControlChange(pitch, velocity, channel, this->cable_number);
    }

    virtual void actual_sendPitchBend(int bend, byte channel) override {
        usbMIDI.sendPitchBend(bend, channel);
    }
};

class MIDIOutputWrapper_MIDISerial : public MIDIOutputWrapper {
    public:
        midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output = nullptr;
        
        MIDIOutputWrapper_MIDISerial(const char *label, midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output, byte channel = 1) 
            : MIDIOutputWrapper(label, channel) {
            this->output = output;
        }

        virtual void actual_sendNoteOn(byte pitch, byte velocity, byte channel) override {
            output->sendNoteOn(pitch, velocity, channel);
        }

        virtual void actual_sendNoteOff(byte pitch, byte velocity, byte channel) override {  
            output->sendNoteOff(pitch, velocity, channel);
        }

        virtual void actual_sendControlChange(byte pitch, byte velocity, byte channel) override {
            output->sendControlChange(pitch, velocity, channel);
        }

        virtual void actual_sendPitchBend(int bend, byte channel) override {
            output->sendPitchBend(bend, channel);
        }
};

class MIDIOutputWrapper_MIDIUSB : public MIDIOutputWrapper {
    public:
        MIDIDeviceBase *output = nullptr;

        MIDIOutputWrapper_MIDIUSB(const char *label, MIDIDeviceBase *output, byte channel = 1) : MIDIOutputWrapper(label, channel) {
            this->output = output;
        }

        virtual void actual_sendNoteOn(byte pitch, byte velocity, byte channel) override {
            output->sendNoteOn(pitch, velocity, channel);
        }

        virtual void actual_sendNoteOff(byte pitch, byte velocity, byte channel) override {  
            output->sendNoteOff(pitch, velocity, channel);
        }

        virtual void actual_sendControlChange(byte pitch, byte velocity, byte channel) override {
            output->sendControlChange(pitch, velocity, channel);
        }

        virtual void actual_sendPitchBend(int bend, byte channel) override {
            output->sendPitchBend(bend, channel);
        }
};

class MIDIOutputWrapper_LoopTrack : public MIDIOutputWrapper {
    public:
        MIDITrack *output = nullptr;

        MIDIOutputWrapper_LoopTrack(const char *label, MIDITrack *output, byte channel = 1) : MIDIOutputWrapper(label, channel) {
            this->output = output;
        }

        virtual void actual_sendNoteOn(byte pitch, byte velocity, byte channel);
        virtual void actual_sendNoteOff(byte pitch, byte velocity, byte channel);
        virtual void actual_sendControlChange(byte number, byte value, byte channel);
        /*virtual void actual_sendPitchBend(int bend, byte channel) override {
            //output->sendPitchBend(bend, channel);
        }*/
};


#include "behaviours/behaviour_base.h"

class MIDIOutputWrapper_Behaviour : public MIDIOutputWrapper {
    public:
        DeviceBehaviourUltimateBase *output = nullptr;

        MIDIOutputWrapper_Behaviour(const char *label, DeviceBehaviourUltimateBase *output, byte channel = 1) : MIDIOutputWrapper(label, channel) {
            this->output = output;
        }
        //virtual ~MIDIOutputWrapper_Behaviour();

        virtual void actual_sendNoteOn(byte pitch, byte velocity, byte channel) override {
            output->sendNoteOn(pitch, velocity, channel);
        }

        virtual void actual_sendNoteOff(byte pitch, byte velocity, byte channel) override {  
            output->sendNoteOff(pitch, velocity, channel);
        }

        virtual void actual_sendControlChange(byte pitch, byte velocity, byte channel) override {
            output->sendControlChange(pitch, velocity, channel);
        }

        virtual void actual_sendPitchBend(int bend, byte channel) override {
            output->sendPitchBend(bend, channel);
        }
};

MIDIOutputWrapper *make_midioutputwrapper_pcusb(const char *label, byte cable_number, byte channel = 1);
MIDIOutputWrapper *make_midioutputwrapper(const char *label, MIDITrack *output, byte channel = 1);
MIDIOutputWrapper *make_midioutputwrapper(const char *label, MIDIDeviceBase *output, byte channel = 1);
MIDIOutputWrapper *make_midioutputwrapper(const char *label, midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output, byte channel = 1);
MIDIOutputWrapper *make_midioutputwrapper(const char *label, DeviceBehaviourUltimateBase *behaviour, byte channel = 1);

//MIDIOutputWrapper::~MIDIOutputWrapper() {}

#endif