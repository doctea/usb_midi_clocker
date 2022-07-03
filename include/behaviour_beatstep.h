#ifndef BEHAVIOUR_BEATSTEP__INCLUDED
#define BEHAVIOUR_BEATSTEP__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviour_base.h"

#include "multi_usb_handlers.h"

extern int current_beatstep_note;
extern int last_beatstep_note;

extern int bass_transpose_octave;

extern MIDIOutputWrapper *beatstep_output;
void beatstep_setOutputWrapper(MIDIOutputWrapper *);

//void beatstep_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void beatstep_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void beatstep_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);

class DeviceBehaviour_Beatstep : public ClockedBehaviour {
    public:
        uint16_t vid = 0x1c75, pid = 0x0206;
        virtual uint32_t get_packed_id () override { return (this->vid<<16 | this->pid); }

        void setup_callbacks() override {
            this->device->setHandleNoteOn(beatstep_handle_note_on);
            this->device->setHandleNoteOff(beatstep_handle_note_off);
        }

        void note_on(uint8_t channel, uint8_t note, uint8_t velocity) override {
            current_beatstep_note = note;
            //Serial.printf("beatstep got note on %i\n", note);

            // send incoming notes from beatstep back out to neutron on serial3, but transposed down

            #ifdef ENABLE_BASS_TRANSPOSE
                //if(midi_out_bass_wrapper!=nullptr) {
                /*int note2 = note - 24;
                if (note2<=0) 
                    note2 += 12;*/
                uint8_t note2 = note & 12;
                note2 += (bass_transpose_octave*12); //24;
                //Serial.printf("beatstep note on %i : %i : %i\n", BASS_MIDI_CHANNEL, note, velocity);
                //Serial.printf("beatstep note2 is %i\n", note2);
                beatstep_output->sendNoteOn((uint8_t)note2, 127); //, BASS_MIDI_CHANNEL);
                //}
            #endif
        }

        void note_off(uint8_t channel, uint8_t note, uint8_t velocity) override {
            if (current_beatstep_note==note) 
                current_beatstep_note = -1;
            last_beatstep_note = note;
            //Serial.printf("beatstep got note off %i\n", note);

            #ifdef ENABLE_BASS_TRANSPOSE
                //if(midi_out_bass_wrapper!=nullptr) {
                /*int note2 = note - 24;
                if (note2<=0) 
                    note2 += 12;*/
                uint8_t note2 = note & 12;
                note2 += (bass_transpose_octave*12);// note2 += 24;
                beatstep_output->sendNoteOff((uint8_t)note2, velocity); //, BASS_MIDI_CHANNEL);
                //}
            #endif
        }

};

extern DeviceBehaviour_Beatstep *behaviour_beatstep;

#endif