#ifndef BEHAVIOUR_BEATSTEP__INCLUDED
#define BEHAVIOUR_BEATSTEP__INCLUDED

#include <Arduino.h>

#include "Config.h"

#ifdef ENABLE_BEATSTEP

#include "behaviour_base.h"

#include "multi_usb_handlers.h"

extern int current_beatstep_note;
extern int last_beatstep_note;

//extern int bass_transpose_octave;

extern MIDIOutputWrapper *beatstep_output;
void beatstep_setOutputWrapper(MIDIOutputWrapper *);

//void beatstep_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void beatstep_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void beatstep_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);

class DeviceBehaviour_Beatstep : public ClockedBehaviour {
    public:
        #define NUM_PATTERNS 16
        bool auto_advance_pattern = false;   // todo: make configurable!

        uint16_t vid = 0x1c75, pid = 0x0206;
        virtual uint32_t get_packed_id () override { return (this->vid<<16 | this->pid); }

        void setup_callbacks() override {
            this->device->setHandleNoteOn(beatstep_handle_note_on);
            this->device->setHandleNoteOff(beatstep_handle_note_off);
        }

        void set_auto_advance_pattern(bool auto_advance_pattern) {
            this->auto_advance_pattern = auto_advance_pattern;
        }
        bool is_auto_advance_pattern() {
            return this->auto_advance_pattern;
        }

        #ifdef ENABLE_BEATSTEP_SYSEX
            void on_phrase(uint32_t phrase) override {
                if (this->device==nullptr) return;

                if (this->auto_advance_pattern) {
                    on_restart(); //TODO: which of these is actually doing the work??

                    uint8_t phrase_number = (uint8_t)(phrase % NUM_PATTERNS);
                    this->send_preset_change(phrase_number);

                    on_restart(); //TODO: which of these is actually doing the work??
                    //Serial.printf("sending sysex to switch to phrase_number %i?\n", phrase_number);
                }
            }

            void send_preset_change(int phrase_number) {
                if (this->device==nullptr) return;
                Serial.printf("beatstep#send_preset_change switching to pattern %i\n", phrase_number % NUM_PATTERNS);

                uint8_t data[] = {
                    0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x05, (uint8_t)(phrase_number % NUM_PATTERNS), 0xF7
                };
                this->device->sendSysEx(sizeof(data), data, true);
            }
        #endif

        void note_on(uint8_t channel, uint8_t note, uint8_t velocity) override {
            current_beatstep_note = note;
            //Serial.printf("beatstep got note on %i\n", note); Serial.flush();

            // send incoming notes from beatstep back out to neutron on serial3, but transposed down

            #ifdef ENABLE_BASS_TRANSPOSE
                //if(midi_out_bass_wrapper!=nullptr) {
                //int note2 = note - 24;
                //if (note2<=0) 
                //    note2 += 12;
                uint8_t note2 = note % 12;
                note2 += (bass_transpose_octave*12); //24;
                //Serial.printf("beatstep note on %i : %i : %i\n", BASS_MIDI_CHANNEL, note, velocity);
                //Serial.printf("beatstep note2 is %i\n", note2);
                note = note2;
                //beatstep_output->sendNoteOn((uint8_t)note2, 127); //, BASS_MIDI_CHANNEL);
                //}
            #endif
            if (beatstep_output!=nullptr)
                beatstep_output->sendNoteOn((uint8_t)note, 127); 
        }

        void note_off(uint8_t channel, uint8_t note, uint8_t velocity) override {
            if (current_beatstep_note==note) 
                current_beatstep_note = -1;
            last_beatstep_note = note;
            //Serial.printf("beatstep got note off %i\n", note); Serial.flush();

            #ifdef ENABLE_BASS_TRANSPOSE
                //if(midi_out_bass_wrapper!=nullptr) {
                //int note2 = note - 24;
                //if (note2<=0) 
                //    note2 += 12;
                uint8_t note2 = note % 12;
                note2 += (bass_transpose_octave*12);// note2 += 24;
                note = note2;
                //}
            #endif
            if (beatstep_output!=nullptr)
                beatstep_output->sendNoteOff((uint8_t)note, velocity); //, BASS_MIDI_CHANNEL);
        }


        #ifdef ENABLE_BASS_TRANSPOSE
            int bass_transpose_octave = 2;  // absolute octave to transpose all notes to
            void setTransposeOctave(int octave) {
                //Serial.printf("Beatstep_Behaviour#setTransposeOctave(%i)!", octave); Serial.flush();
                //if (midi_out_bass_wrapper!=nullptr) // isn't a pointer!
                if (octave!=this->bass_transpose_octave) {
                    //midi_out_bass_wrapper.stop_all_notes();
                    if (beatstep_output!=nullptr)
                        beatstep_output->stop_all_notes();
                    this->bass_transpose_octave = octave;
                }
            }
            int getTransposeOctave() {
                //Serial.println("Beatstep_Behaviour#getTransposeOctave!"); Serial.flush();
                return this->bass_transpose_octave;
            }
        #endif
};

extern DeviceBehaviour_Beatstep *behaviour_beatstep;

#endif

#endif