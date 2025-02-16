#pragma once

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
// TODO: probably can use templating to implement the different wrapper types and make this more generic

#ifdef DEBUG_MIDI_WRAPPER
    struct message_history_t {
        uint32_t ticks;
        int8_t type;
        int8_t channel;
        int8_t pitch;
        int8_t velocity;
    };
#endif

class MIDIOutputWrapper : virtual public IMIDINoteAndCCTarget {

    public:
        bool debug = false;

        bool always_force_stop_all = false;

        int8_t default_channel = 0;
        char label[MAX_LENGTH_OUTPUT_WRAPPER_LABEL];

        int8_t playing_notes[MIDI_NUM_NOTES];

        int last_note = -1, current_note = -1;
        //int last_transposed_note = -1, current_transposed_note = -1;

        #ifdef DEBUG_MIDI_WRAPPER
            message_history_t *message_history = nullptr;
            const int8_t message_history_size = 10;
            int8_t next_message_history_index = 0;
        #endif

        MIDIOutputWrapper(const char *label, int8_t channel = 1) {
            strncpy(this->label, label, MAX_LENGTH_OUTPUT_WRAPPER_LABEL);
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

            virtual void log_message(int8_t type, int8_t pitch, int8_t velocity, int8_t channel) {
                if (this->debug) Serial_printf("%s#log_message(%02x, %3i, %2i, %2i)\n", this->label, type, pitch, velocity, channel);

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
            virtual void log_message_on(int8_t pitch, int8_t velocity, int8_t channel) {
                log_message(midi::NoteOn, pitch, velocity, channel);
            }
            virtual void log_message_off(int8_t pitch, int8_t velocity, int8_t channel) {
                log_message(midi::NoteOff, pitch, velocity, channel);
            }
        #endif

        //virtual void sendNoteOn(int8_t pitch, int8_t velocity, int8_t channel = 0);
        //virtual void sendNoteOff(int8_t pitch, int8_t velocity, int8_t channel = 0);

        virtual void sendNoteOn(uint8_t in_pitch, uint8_t velocity, uint8_t channel) override {
            if (!is_valid_note(in_pitch)) return;

            current_note = in_pitch;
            //int pitch = recalculate_pitch(in_pitch);

            if (this->debug) { Serial_printf(F("MIDIOutputWrapper#sendNoteOn\t(p=%3i, v=%3i, c=%2i) in %s...\n"), current_note, velocity, channel, label); Serial_flush(); }

            if (playing_notes[current_note]<8) {
                //if (this->debug) Serial_printf("\tplaying_notes[%i] is already %i -- increasing by 1\n", pitch, playing_notes[pitch]);
                playing_notes[current_note]++;
            } else {
                //if (this->debug) Serial_printf("\talready playing %i notes at pitch %i, so not counting a new one\n", playing_notes[pitch], pitch);
            }

            //current_transposed_note = current_note;

            if (channel==0) {
                if (this->debug) Serial_printf(F("\t(swapping channel %i for default_channel %i)\n"), channel, default_channel);
                channel = default_channel;
            }

            #ifdef DEBUG_MIDI_WRAPPER
                this->log_message_on(current_note, velocity, channel);
            #endif
            this->actual_sendNoteOn(current_note, velocity, channel);
        }

        virtual void sendNoteOff(uint8_t in_pitch, uint8_t velocity, uint8_t channel) override {
            if (!is_valid_note(in_pitch)) return;

            this->last_note = in_pitch;
            if (this->current_note==in_pitch) 
                current_note = -1;

            //int pitch = recalculate_pitch(in_pitch);

            if (this->debug) 
                Serial_printf("MIDIOutputWrapper#sendNoteOff\t(p=%3i, v=%3i, c=%2i)\tcurrent count is\t%i\n", in_pitch, velocity, channel, playing_notes[in_pitch]);

            
            if (playing_notes[in_pitch]>0) playing_notes[in_pitch]--;
            if (playing_notes[in_pitch]<0) playing_notes[in_pitch] = 0;
            if (playing_notes[in_pitch]>0) {
                if (this->debug) Serial_printf("\tMIDIOutputWrapper#sendNoteOff - playing_notes[%3i]: not sending note off because count is\tP%i\n", in_pitch, playing_notes[in_pitch]);
                return;
            }
            
            //this->last_transposed_note = in_pitch;
            //if (this->current_transposed_note==in_pitch)
            //    current_transposed_note = -1;

            if (channel==0) {
                if (this->debug) Serial_printf(F("\t(swapping channel %2i for default_channel %2i)\n"), channel, default_channel);
                channel = default_channel;
            }

            #ifdef DEBUG_MIDI_WRAPPER
                this->log_message_off(in_pitch, velocity, channel);
            #endif
            this->actual_sendNoteOff(in_pitch, velocity, channel);

            //this->last_transposed_note = in_pitch;
            //if (this->current_transposed_note==in_pitch)
            //    current_transposed_note = -1;
        }

        virtual void sendControlChange(uint8_t cc_number, uint8_t velocity, uint8_t channel = 0) override {
            if (channel == 0) channel = this->default_channel;
            this->actual_sendControlChange(cc_number, velocity, channel);
        };

        virtual void sendPitchBend(int bend, int8_t channel = 0) {
            if (channel == 0 ) channel = this->default_channel;
            this->actual_sendPitchBend(bend, channel);
        }

        // these are the parts that actually send using the underlying objects -- split out so that can override in subclasses
        virtual void actual_sendControlChange(int8_t pitch, int8_t velocity, int8_t channel) {};
        virtual void actual_sendNoteOn(int8_t pitch, int8_t velocity, int8_t channel) {};
        virtual void actual_sendNoteOff(int8_t pitch, int8_t velocity, int8_t channel) {};
        virtual void actual_sendPitchBend(int pitch, int8_t channel) {};

        virtual bool is_note_playing(int pitch) {
            //pitch = recalculate_pitch(pitch);
            if (!is_valid_note(pitch)) return false;
            return playing_notes[pitch]>0;
        }

        virtual void stop_all_notes(bool force = false) {
            if (this->always_force_stop_all) 
                force = true;
            //this->debug = true;
            if (this->debug) Serial_printf(F("stop_all_notes in %s...\n"), label);
            if (is_valid_note(this->current_note)) {
                this->actual_sendNoteOff(this->current_note,0,default_channel);
                //this->actual_sendNoteOff(this->current_transposed_note,0,default_channel);
            }
            sendControlChange(midi::AllNotesOff, MIDI_MAX_VELOCITY);
            for (uint_fast8_t pitch = 0 ; pitch < MIDI_NUM_NOTES ; pitch++) {
                //int pitch = recalculate_pitch(i);
                //if (is_note_playing(pitch)) {
                    //if (this->debug) 
                if (this->debug) Serial_printf("\tGot %i notes of pitch %i to stop on channel %i\t\n", playing_notes[pitch], pitch, default_channel);
                if(force /*|| is_note_playing(pitch)*/) {
                    if (this->debug) Serial_printf("\t\tforce, so actually sending...\n");
                    sendNoteOff(pitch, 0, default_channel);
                }
                playing_notes[pitch] = 0;
            }
        }
};

class MIDIOutputWrapper_PC : public MIDIOutputWrapper {
    public:
    int8_t cable_number = 0;

    MIDIOutputWrapper_PC(const char *label, int8_t cable_number, int8_t channel = 1) : MIDIOutputWrapper(label, channel) {
        this->cable_number = cable_number;
    }

    virtual void actual_sendNoteOn(int8_t pitch, int8_t velocity, int8_t channel) override {
        usbMIDI.sendNoteOn(pitch, velocity, channel, this->cable_number);
    }

    virtual void actual_sendNoteOff(int8_t pitch, int8_t velocity, int8_t channel) override {
        usbMIDI.sendNoteOff(pitch, velocity, channel, this->cable_number);
    }

    virtual void actual_sendControlChange(int8_t pitch, int8_t velocity, int8_t channel) override {
        usbMIDI.sendControlChange(pitch, velocity, channel, this->cable_number);
    }

    virtual void actual_sendPitchBend(int bend, int8_t channel) override {
        usbMIDI.sendPitchBend(bend, channel);
    }
};

class MIDIOutputWrapper_MIDISerial : public MIDIOutputWrapper {
    public:
        midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output = nullptr;
        
        MIDIOutputWrapper_MIDISerial(const char *label, midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output, int8_t channel = 1) 
            : MIDIOutputWrapper(label, channel) {
            this->output = output;
        }

        virtual void actual_sendNoteOn(int8_t pitch, int8_t velocity, int8_t channel) override {
            if (channel==0) channel = default_channel;
            output->sendNoteOn(pitch, velocity, channel);
        }

        virtual void actual_sendNoteOff(int8_t pitch, int8_t velocity, int8_t channel) override {  
            if (channel==0) channel = default_channel;
            output->sendNoteOff(pitch, velocity, channel);
        }

        virtual void actual_sendControlChange(int8_t cc_number, int8_t value, int8_t channel) override {
            if (channel==0) channel = default_channel;
            output->sendControlChange(cc_number, value, channel);
        }

        virtual void actual_sendPitchBend(int bend, int8_t channel) override {
            if (channel==0) channel = default_channel;
            output->sendPitchBend(bend, channel);
        }
};

class MIDIOutputWrapper_MIDIUSB : public MIDIOutputWrapper {
    public:
        MIDIDeviceBase *output = nullptr;

        MIDIOutputWrapper_MIDIUSB(const char *label, MIDIDeviceBase *output, int8_t channel = 1) : MIDIOutputWrapper(label, channel) {
            this->output = output;
        }

        virtual void actual_sendNoteOn(int8_t pitch, int8_t velocity, int8_t channel) override {
            if (this->debug) Serial_printf("MIDIOutputWrapper_MIDIUSB\t%s\t#actual_sendNoteOn(pitch=%i,\tvelocity=%i,\tchannel=%i)\n", output->product(), velocity, channel);
            output->sendNoteOn(pitch, velocity, channel);
        }

        virtual void actual_sendNoteOff(int8_t pitch, int8_t velocity, int8_t channel) override {  
            if (channel==0) channel = default_channel;
            output->sendNoteOff(pitch, velocity, channel);
        }

        virtual void actual_sendControlChange(int8_t pitch, int8_t velocity, int8_t channel) override {
            if (channel==0) channel = default_channel;
            output->sendControlChange(pitch, velocity, channel);
        }

        virtual void actual_sendPitchBend(int bend, int8_t channel) override {
            if (channel==0) channel = default_channel;
            output->sendPitchBend(bend, channel);
        }
};

#ifdef ENABLE_LOOPER
class MIDIOutputWrapper_LoopTrack : public MIDIOutputWrapper {
    public:
        MIDITrack *output = nullptr;

        MIDIOutputWrapper_LoopTrack(const char *label, MIDITrack *output, int8_t channel = 1) : MIDIOutputWrapper(label, channel) {
            this->output = output;
        }

        virtual void actual_sendNoteOn(int8_t pitch, int8_t velocity, int8_t channel);
        virtual void actual_sendNoteOff(int8_t pitch, int8_t velocity, int8_t channel);
        virtual void actual_sendControlChange(int8_t number, int8_t value, int8_t channel);
        /*virtual void actual_sendPitchBend(int bend, int8_t channel) override {
            //output->sendPitchBend(bend, channel);
        }*/
};
#endif

#include "behaviours/behaviour_base.h"

class MIDIOutputWrapper_Behaviour : public MIDIOutputWrapper {
    public:
        DeviceBehaviourUltimateBase *output = nullptr;

        MIDIOutputWrapper_Behaviour(const char *label, DeviceBehaviourUltimateBase *output, int8_t channel = 1) : MIDIOutputWrapper(label, channel) {
            this->output = output;
        }

        virtual void actual_sendNoteOn(int8_t pitch, int8_t velocity, int8_t channel) override {
            if (this->debug) Serial_printf("MIDIOutputWrapper_Behaviour\t%s\t#actual_sendNoteOn(pitch=%i,\tvelocity=%i,\tchannel=%i)\n", output->get_label(), pitch, velocity, channel);
            output->sendNoteOn(pitch, velocity, channel);
        }

        virtual void actual_sendNoteOff(int8_t pitch, int8_t velocity, int8_t channel) override {  
            output->sendNoteOff(pitch, velocity, channel);
        }

        virtual void actual_sendControlChange(int8_t pitch, int8_t velocity, int8_t channel) override {
            output->sendControlChange(pitch, velocity, channel);
        }

        virtual void actual_sendPitchBend(int bend, int8_t channel) override {
            output->sendPitchBend(bend, channel);
        }
};

MIDIOutputWrapper *make_midioutputwrapper_pcusb(const char *label, int8_t cable_number, int8_t channel = 1);
MIDIOutputWrapper *make_midioutputwrapper(const char *label, MIDITrack *output, int8_t channel = 1);
MIDIOutputWrapper *make_midioutputwrapper(const char *label, MIDIDeviceBase *output, int8_t channel = 1);
MIDIOutputWrapper *make_midioutputwrapper(const char *label, midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output, int8_t channel = 1);
MIDIOutputWrapper *make_midioutputwrapper(const char *label, DeviceBehaviourUltimateBase *behaviour, int8_t channel = 1);

