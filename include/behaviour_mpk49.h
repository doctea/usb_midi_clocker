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

        void setup_callbacks() override {
            Serial.println("Setting up callbacks for the MPK49");
            this->device->setHandleNoteOn(mpk49_handle_note_on);
            this->device->setHandleNoteOff(mpk49_handle_note_off);
            this->device->setHandleSystemExclusive(mpk49_handle_system_exclusive);
        }

        /*void note_on(uint8_t channel, uint8_t note, uint8_t velocity) override {
            mpk49_output->sendNoteOn(note, velocity);            
            #ifdef ENABLE_LOOPER
                this->loop_track->in_event(ticks, midi::NoteOn,  note, velocity);
            #endif
        }

        void note_off(uint8_t channel, uint8_t note, uint8_t velocity) override {
            mpk49_output->sendNoteOff(note, velocity);

            #ifdef ENABLE_LOOPER
                this->loop_track->in_event(ticks, midi::NoteOff,note, velocity);
            #endif
        }*/

        #ifdef ENABLE_LOOPER
            void on_pre_clock(unsigned long ticks) {
                this->loop_track->process_tick(ticks);
            }
        #endif

        void handle_system_exclusive(uint8_t *data, unsigned int size) {
            if (this->debug) {
                Serial.printf("mpk_handle_system_exclusive of size %i: [",size);
                for (unsigned int i = 0 ; i < size ; i++) {
                    Serial.printf("%02x ", data[i]);
                }
                Serial.println("]");
            }

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
            loop_track->toggle_recording();
        }

        void handle_mmc_start() {
            loop_track->start_playing();
            playing = true;
        }

        void handle_mmc_stop() {
            loop_track->stop_recording();
            loop_track->stop_playing();
        }

};

extern DeviceBehaviour_mpk49 *behaviour_mpk49;

#endif