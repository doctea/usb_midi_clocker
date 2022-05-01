#ifndef MIDI_LOOPER__INCLUDED
#define MIDI_LOOPER__INCLUDED

#include <LinkedList.h>
#include "Config.h"
#include "ConfigMidi.h"
#include "MidiMappings.h"

//#include <MIDI.h>
//#include "USBHost_t36.h"

#define LOOP_LENGTH (PPQN*4*4)

/*
void recordInstruction(byte instruction_type, byte channel, byte arg0, byte arg1);
void playInstruction(int index);
void clear_recording();
void stop_all_notes();*/


typedef struct midi_message {
    uint8_t message_type;
    uint8_t pitch;
    uint8_t velocity;
    uint8_t channel;
};

class midi_output_wrapper {
    midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output_serialmidi;
    MIDIDevice_BigBuffer *output_usb;
    byte default_channel = 1;

    public:
        midi_output_wrapper(midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *in_output_serialmidi, byte channel = 1) {
            output_serialmidi = in_output_serialmidi;
            default_channel = channel;
        }
        midi_output_wrapper(MIDIDevice_BigBuffer *in_output_usb, byte channel = 1) {
            output_usb = in_output_usb;
            default_channel = channel;
        }

        void sendNoteOn(byte pitch, byte velocity, byte channel = 0) {
            if (channel==0) channel = default_channel;
            if (output_serialmidi) output_serialmidi->sendNoteOn(pitch, velocity, channel);
            if (output_usb)        output_usb->sendNoteOn(pitch, velocity, channel);
        }

        void sendNoteOff(byte pitch, byte velocity, byte channel = 0) {
            if (channel==0) channel = default_channel;
            if (output_serialmidi) output_serialmidi->sendNoteOff(pitch, velocity, channel);
            if (output_usb)        output_usb->sendNoteOff(pitch, velocity, channel);
        }
};

class midi_track {
    //LinkedList<midi_frame> frames = LinkedList<midi_frame> ();
    //midi_frame frames[LOOP_LENGTH];
    LinkedList<midi_message> frames[LOOP_LENGTH];
    midi_output_wrapper *output;

    bool playing_notes[127];    // track what notes are playing so we can turn them off / record ends appropriately
    
    public: 
        midi_track(midi_output_wrapper *default_output) {
            output = default_output;
            //frames[0].time = 0;
            /*for (int i = 0 ; i < LOOP_LENGTH ; i++) {
                frames[i] = new LinkedList<midi_message>();
            }*/
        };

        void record_event(unsigned long time, uint8_t instruction_type, /*uint8_t channel,*/ uint8_t pitch, uint8_t velocity) {
            midi_message m;
            m.message_type = instruction_type;
            //m.channel = 3; //channel;
            m.pitch = pitch;
            m.velocity = velocity;
            frames[time%LOOP_LENGTH].add(m);
        }
        void record_event(unsigned long time, midi_message midi_event) {
            Serial.printf("Recording event at\t%i", time);
            frames[time%LOOP_LENGTH].add(midi_event);
        }

        LinkedList<midi_message> get_frames(unsigned long time) {
            return this->frames[time];
        }

        void play_events(unsigned long time) { //, midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *specified_output = nullptr) {
            //if (!specified_output)
            //    specified_output = output;

            int position = time%LOOP_LENGTH;
            int number_messages = frames[position].size();

            if (frames[position].size()>0) 
                Serial.printf("for frame\t%i got\t%i messages to play\n", position, number_messages);

            for (int i = 0 ; i < number_messages ; i++) {
                midi_message m = frames[position].get(i);
                
                switch (m.message_type) {
                    case midi::NoteOn:
                        output->sendNoteOn(m.pitch, m.velocity); //, m.channel);
                        playing_notes[m.pitch] = true;
                        break;
                    case midi::NoteOff:
                        output->sendNoteOff(m.pitch, m.velocity); //, m.channel);
                        playing_notes[m.pitch] = false;
                        break;
                    default:
                        Serial.printf("\t%i: Unhandled message type %i\n", i, 3); //m.message_type);
                        break;
                }
            }
        }

        void clear_all() {
            stop_all_notes();
            Serial.println("clearing recording");
            for (int i = 0 ; i < LOOP_LENGTH ; i++) {
                // todo: actually free the recorded memory..?
                frames[i].clear();
            }
        }

        void stop_all_notes() {
            // todo: probably move this into the wrapper, so that can have multiple sources playing into the same output?
            for (byte i = 0 ; i < 127 ; i++) {
                if (playing_notes[i]) {
                    output->sendNoteOff(i, 0);
                    playing_notes[i] = false;
                }
            }
        }

        void stop_recording() {
            Serial.println("mpk49 stopped recording");
            // send & record note-offs for all playing notes
            for (byte i = 0 ; i < 127 ; i++) {
                if (playing_notes[i]) {
                    output->sendNoteOff(i, 0);
                    record_event(ticks%LOOP_LENGTH, midi::NoteOff, i, 0);
                    playing_notes[i] = false;
                }
            }
        }

        void start_recording() {
            Serial.println("mpk49 started recording");
            // uhhh nothing to do rn?
        }
        
};

extern midi_track mpk49_loop_track;

/**
 * @var {byte} the maximum number of instructions
 *      its possible to loop around while loops are still playing, if so instructions in the lower loop will be overwritten
 */
const unsigned long MAX_INSTRUCTIONS = (LOOP_LENGTH); //100;
#define MAX_INSTRUCTION_ARGUMENTS 4

/***************************
 * LOOP INSTRUCTION DATA   *
 ***************************/
/**
 * @var {Byte[][]} the instructions for each loop 
 *                 loop_instructions[instruction_index] = byte args[]
 */
extern byte loop_instructions[MAX_INSTRUCTIONS][MAX_INSTRUCTION_ARGUMENTS];

#endif