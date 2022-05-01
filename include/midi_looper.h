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

class midi_track {
    //LinkedList<midi_frame> frames = LinkedList<midi_frame> ();
    //midi_frame frames[LOOP_LENGTH];
    LinkedList<midi_message> frames[LOOP_LENGTH];

    public: 
        midi_track() {
            //frames[0].time = 0;
            /*for (int i = 0 ; i < LOOP_LENGTH ; i++) {
                frames[i] = new LinkedList<midi_message>();
            }*/
        };

        void record_event(unsigned long time, uint8_t instruction_type, uint8_t channel, uint8_t arg0, uint8_t arg1) {
            midi_message m;
            m.message_type = instruction_type;
            m.channel = 3; //channel;
            m.pitch = arg0;
            m.velocity = arg1;
            frames[time%LOOP_LENGTH].add(m);
        }
        void record_event(unsigned long time, midi_message midi_event) {
            Serial.printf("Recording event at\t%i", time);
            frames[time%LOOP_LENGTH].add(midi_event);
        }

        LinkedList<midi_message> get_frames(unsigned long time) {
            return this->frames[time];
        }

        void play_events(unsigned long time, midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output) {
            if (frames[time%LOOP_LENGTH].size()>0) 
                Serial.printf("for frame\t%i got\t%i messages to play\n", time%LOOP_LENGTH, frames[time%LOOP_LENGTH].size());

            for (int i = 0 ; i < frames[time%LOOP_LENGTH].size() ; i++) {
                midi_message m = frames[time%LOOP_LENGTH].get(i);
                
                if (m.message_type==midi::NoteOn)
                    output->sendNoteOn(m.pitch, m.velocity, m.channel);
                else if (m.message_type==midi::NoteOff)
                    output->sendNoteOff(m.pitch, m.velocity, m.channel);
                else
                    Serial.printf("\t%i: Unknown message type %i\n", i, 3); //m.message_type);
            }
        }

        void clear_all() {
            for (int i = 0 ; i < LOOP_LENGTH ; i++) {
                frames[i].clear();
            }
        }

        void stop_all_notes() {
            for (int i = 0 ; i < LOOP_LENGTH ; i++) {
                LinkedList<midi_message> messages = frames[i];
                for (int x = 0 ; x < frames[i].size() ; x++) {
                    midi_message m = frames[i].get(i);
                    midi_out_bitbox->sendNoteOff(
                        m.pitch, 
                        m.velocity, 
                        m.channel
                    );
                }
            }
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