#ifndef MIDI_OUT_WRAPPER__INCLUDED
#define MIDI_OUT_WRAPPER__INCLUDED

#include "debug.h"

#include <MIDI.h>
#include <USBHost_t36.h>
#include <SdFat.h>

#include "midi/midi_looper.h"
#include "midi_helpers.h"
#include "bpm.h"

#define MAX_LENGTH_OUTPUT_WRAPPER_LABEL 40

//#define DEBUG_MIDI_WRAPPER

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

#ifdef DEBUG_MIDI_WRAPPER
    struct message_history_t {
        uint32_t ticks;
        byte type;
        byte channel;
        byte pitch;
        byte velocity;
    };
#endif

class MIDIOutputWrapper {

    public:
        bool debug = false;

        bool always_force_stop_all = false;

        byte default_channel = 0;
        char label[MAX_LENGTH_OUTPUT_WRAPPER_LABEL];

        byte playing_notes[127];

        int last_note = -1, current_note = -1;
        //int last_transposed_note = -1, current_transposed_note = -1;

        #ifdef DEBUG_MIDI_WRAPPER
            message_history_t *message_history = nullptr;
            const byte message_history_size = 10;
            byte next_message_history_index = 0;
        #endif

        MIDIOutputWrapper(const char *label, byte channel = 1) {
            strcpy(this->label, label);
            default_channel = channel;
            memset(playing_notes, 0, sizeof(playing_notes));
        }
        virtual ~MIDIOutputWrapper();

        #ifdef DEBUG_MIDI_WRAPPER
            virtual void set_log_message_mode(bool status) {
                if (this->message_history!=nullptr) {
                    free(this->message_history);
                    this->message_history = nullptr;
                } else {
                    this->message_history = (message_history_t*)calloc(sizeof(message_history_t), message_history_size);
                }
            }

            virtual void log_message(byte type, byte pitch, byte velocity, byte channel) {
                if (this->debug) Serial.printf("%s#log_message(%02x, %3i, %2i, %2i)\n", this->label, type, pitch, velocity, channel);

                if (this->message_history!=nullptr) {
                    this->message_history[next_message_history_index].ticks = ticks;
                    this->message_history[next_message_history_index].type = type;
                    this->message_history[next_message_history_index].pitch = pitch;
                    this->message_history[next_message_history_index].velocity = velocity;
                    this->message_history[next_message_history_index].channel = channel;
                    next_message_history_index++;
                    if (next_message_history_index>=message_history_size)
                        next_message_history_index = 0;
                }
            }
            virtual void log_message_on(byte pitch, byte velocity, byte channel) {
                log_message(midi::NoteOn, pitch, velocity, channel);
            }
            virtual void log_message_off(byte pitch, byte velocity, byte channel) {
                log_message(midi::NoteOff, pitch, velocity, channel);
            }
        #endif



        //virtual void sendNoteOn(byte pitch, byte velocity, byte channel = 0);
        //virtual void sendNoteOff(byte pitch, byte velocity, byte channel = 0);

        virtual void sendNoteOn(byte in_pitch, byte velocity, byte channel) {
            if (!is_valid_note(in_pitch)) return;

            current_note = in_pitch;
            //int pitch = recalculate_pitch(in_pitch);

            if (this->debug) Serial.printf(F("MIDIOutputWrapper#sendNoteOn\t(p=%3i, v=%3i, c=%2i) in %s...\n"), current_note, velocity, channel, label); Serial_flush();

            if (playing_notes[current_note]<8) {
                //if (this->debug) Serial.printf("\tplaying_notes[%i] is already %i -- increasing by 1\n", pitch, playing_notes[pitch]);
                playing_notes[current_note]++;
            } else {
                //if (this->debug) Serial.printf("\talready playing %i notes at pitch %i, so not counting a new one\n", playing_notes[pitch], pitch);
            }

            //current_transposed_note = current_note;

            if (channel==0) {
                if (this->debug) Serial.printf(F("\t(swapping channel %i for default_channel %i)\n"), channel, default_channel);
                channel = default_channel;
            }

            #ifdef DEBUG_MIDI_WRAPPER
                this->log_message_on(current_note, velocity, channel);
            #endif
            this->actual_sendNoteOn(current_note, velocity, channel);
        }

        virtual void sendNoteOff(byte in_pitch, byte velocity, byte channel) {
            if (!is_valid_note(in_pitch)) return;

            this->last_note = in_pitch;
            if (this->current_note==in_pitch) 
                current_note = -1;

            //int pitch = recalculate_pitch(in_pitch);

            if (this->debug) 
                Serial.printf("MIDIOutputWrapper#sendNoteOff\t(p=%3i, v=%3i, c=%2i)\tcurrent count is\t%i\n", in_pitch, velocity, channel, playing_notes[in_pitch]);

            if (playing_notes[in_pitch]>0) playing_notes[in_pitch]--;
            if (playing_notes[in_pitch]!=0) {
                if (this->debug) Serial.printf("\tMIDIOutputWrapper#sendNoteOff - playing_notes[%3i]: not sending note off because count is\tP%i\n", in_pitch, playing_notes[in_pitch]);
                return;
            }

            //this->last_transposed_note = in_pitch;
            //if (this->current_transposed_note==in_pitch)
            //    current_transposed_note = -1;

            if (channel==0) {
                if (this->debug) Serial.printf(F("\t(swapping channel %2i for default_channel %2i)\n"), channel, default_channel);
                channel = default_channel;
            }

            #ifdef DEBUG_MIDI_WRAPPER
                this->log_message_off(pitch, velocity, channel);
            #endif
            this->actual_sendNoteOff(in_pitch, velocity, channel);

            //this->last_transposed_note = in_pitch;
            //if (this->current_transposed_note==in_pitch)
            //    current_transposed_note = -1;
        }

        virtual void sendControlChange(byte cc_number, byte velocity, byte channel = 0) {
            if (channel == 0) channel = this->default_channel;
            this->actual_sendControlChange(cc_number, velocity, channel);
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
            //pitch = recalculate_pitch(pitch);
            if (!is_valid_note(pitch)) return false;
            return playing_notes[pitch]>0;
        }

        virtual void stop_all_notes(bool force = false) {
            if (this->always_force_stop_all) 
                force = true;
            //this->debug = true;
            if (this->debug) Serial.printf(F("stop_all_notes in %s...\n"), label);
            if (is_valid_note(this->current_note)) {
                this->actual_sendNoteOff(this->current_note,0,default_channel);
                //this->actual_sendNoteOff(this->current_transposed_note,0,default_channel);
            }
            sendControlChange(midi::AllNotesOff, 127);
            for (int pitch = 0 ; pitch < 127 ; pitch++) {
                //int pitch = recalculate_pitch(i);
                //if (is_note_playing(pitch)) {
                    //if (this->debug) 
                if (this->debug) Serial.printf("\tGot %i notes of pitch %i to stop on channel %i\t\n", playing_notes[pitch], pitch, default_channel);
                if(force /*|| is_note_playing(pitch)*/) {
                    if (this->debug) Serial.printf("\t\tforce, so actually sending...\n");
                    sendNoteOff(pitch, 0, default_channel);
                }
                playing_notes[pitch] = 0;
            }
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
            if (channel==0) channel = default_channel;
            output->sendNoteOn(pitch, velocity, channel);
        }

        virtual void actual_sendNoteOff(byte pitch, byte velocity, byte channel) override {  
            if (channel==0) channel = default_channel;
            output->sendNoteOff(pitch, velocity, channel);
        }

        virtual void actual_sendControlChange(byte cc_number, byte value, byte channel) override {
            if (channel==0) channel = default_channel;
            output->sendControlChange(cc_number, value, channel);
        }

        virtual void actual_sendPitchBend(int bend, byte channel) override {
            if (channel==0) channel = default_channel;
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
            if (this->debug) Serial.printf("MIDIOutputWrapper_MIDIUSB\t%s\t#actual_sendNoteOn(pitch=%i,\tvelocity=%i,\tchannel=%i)\n", output->product(), velocity, channel);
            output->sendNoteOn(pitch, velocity, channel);
        }

        virtual void actual_sendNoteOff(byte pitch, byte velocity, byte channel) override {  
            if (channel==0) channel = default_channel;
            output->sendNoteOff(pitch, velocity, channel);
        }

        virtual void actual_sendControlChange(byte pitch, byte velocity, byte channel) override {
            if (channel==0) channel = default_channel;
            output->sendControlChange(pitch, velocity, channel);
        }

        virtual void actual_sendPitchBend(int bend, byte channel) override {
            if (channel==0) channel = default_channel;
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

        virtual void actual_sendNoteOn(byte pitch, byte velocity, byte channel) override {
            if (this->debug) Serial.printf("MIDIOutputWrapper_Behaviour\t%s\t#actual_sendNoteOn(pitch=%i,\tvelocity=%i,\tchannel=%i)\n", output->get_label(), pitch, velocity, channel);
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

#endif