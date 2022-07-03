#ifndef BEHAVIOUR_MPK49__INCLUDED
#define BEHAVIOUR_MPK49__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviour_base.h"

#include "multi_usb_handlers.h"

#include "midi_looper.h"

extern MIDIOutputWrapper *mpk49_output;
void mpk49_setOutputWrapper(MIDIOutputWrapper *);

//void mpk49_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void mpk49_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void mpk49_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void mpk49_handle_system_exclusive(uint8_t *data, unsigned int size);

class DeviceBehaviour_mpk49 : public ClockedBehaviour {
    public:
        MIDITrack *loop_track = nullptr;

        uint16_t vid = 0x09E8, pid = 0x006B;
        virtual uint32_t get_packed_id () override { return (this->vid<<16 | this->pid); }

        bool recording = false;
        bool playing = false;

        void setup_callbacks() override {
            Serial.println("Setting up callbacks for the MPK49");
            this->device->setHandleNoteOn(mpk49_handle_note_on);
            this->device->setHandleNoteOff(mpk49_handle_note_off);
            this->device->setHandleSystemExclusive(mpk49_handle_system_exclusive);
        }

        void note_on(uint8_t channel, uint8_t note, uint8_t velocity) override {
            #ifdef ENABLE_LOOPER
                if (recording)
                    this->loop_track->record_event(ticks%LOOP_LENGTH, midi::NoteOn, /*channel,*/ note, velocity);
            #endif

            mpk49_output->sendNoteOn(note, velocity);
        }

        void note_off(uint8_t channel, uint8_t note, uint8_t velocity) override {
            #ifdef ENABLE_LOOPER
                if (recording)
                    this->loop_track->record_event(ticks%LOOP_LENGTH, midi::NoteOff, /*channel,*/ note, velocity);
            #endif

            mpk49_output->sendNoteOff(note, velocity);
        }

        void on_tick(unsigned long ticks) override {
            ClockedBehaviour::on_tick(ticks);
            #ifdef ENABLE_LOOPER
                if (playing)
                    this->loop_track->play_events(ticks); //, midi_out_bitbox); //%(LOOP_LENGTH));
            #endif
        }

        void handle_system_exclusive(uint8_t *data, unsigned int size) {
            Serial.printf("mpk_handle_system_exclusive of size %i: [",size);
            for (unsigned int i = 0 ; i < size ; i++) {
                Serial.printf("%02x ", data[i]);
            }
            Serial.println("]");

            if (data[3]==0x06) {
                if (data[4]==0x06) { // record pressed
                    handle_mmc_record();
                } else if (data[4]==0x01) { // stop pressed
                    handle_mmc_stop();
                } else if (data[4]==0x02) { // start pressed
                    handle_mmc_start();
                }
            }
        }

        void handle_mmc_record() {
            recording = !recording;
            if (recording) {
                loop_track->start_recording();
            } else {
                loop_track->stop_recording();
            }
        }

        void handle_mmc_start() {
            playing = true;
        }

        void handle_mmc_stop() {
            playing = false;

            if (!playing) {
                loop_track->stop_all_notes();
                //mpk49_loop_track.stop_recording();
                //midi_out_bitbox->sendControlChange(123,0,3);
            }
        }

};

extern DeviceBehaviour_mpk49 *behaviour_mpk49;

#endif