#ifndef BEHAVIOUR_MPK49__INCLUDED
#define BEHAVIOUR_MPK49__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviours/behaviour_base_usb.h"
#include "behaviours/behaviour_clocked.h"

#include "multi_usb_handlers.h"

#include "midi/midi_looper.h"

extern MIDIOutputWrapper *mpk49_output;

void mpk49_handle_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void mpk49_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void mpk49_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void mpk49_handle_system_exclusive(uint8_t *data, unsigned int size);

class DeviceBehaviour_mpk49 : virtual public DeviceBehaviourUSBBase, virtual public ClockedBehaviour {
    public:
        MIDITrack *loop_track = nullptr;

        uint16_t vid = 0x09E8, pid = 0x006B;
        virtual uint32_t get_packed_id () override { return (this->vid<<16 | this->pid); }

        virtual const char *get_label() override {
            return (char*)"MPK49";
        }
        virtual bool has_input() { return true; }

        //FLASHMEM 
        virtual void setup_callbacks() override {
            Serial.println(F("Setting up callbacks for the MPK49"));
            this->device->setHandleNoteOn(mpk49_handle_note_on);
            this->device->setHandleNoteOff(mpk49_handle_note_off);
            this->device->setHandleControlChange(mpk49_handle_control_change);
            this->device->setHandleSystemExclusive(mpk49_handle_system_exclusive);
        }

        // TODO: move this elsewhere, since the looper is no longer explicitly tied to the mpk49..
        #ifdef ENABLE_LOOPER
            virtual void on_pre_clock(unsigned long ticks) {
                this->loop_track->process_tick(ticks);
            }
        #endif

        virtual void handle_system_exclusive(uint8_t *data, unsigned int size) {
            if (this->debug) {
                Serial.printf(F("mpk_handle_system_exclusive of size %i: ["), size);
                for (unsigned int i = 0 ; i < size ; i++) {
                    Serial.printf("%02x ", data[i]);
                }
                Serial.println(F("]"));
            }

            if (data[3]==0x06) {
                if (data[4]==0x06) { // record pressed
                    Serial.printf(F("%s: calling handle_mmc_record()\n"), this->get_label()); Serial.flush();
                    handle_mmc_record();
                } else if (data[4]==0x01) { // stop pressed
                    Serial.printf(F("%s: calling handle_mmc_stop()\n"), this->get_label()); Serial.flush();
                    handle_mmc_stop();
                } else if (data[4]==0x02) { // start pressed
                    Serial.printf(F("%s: calling handle_mmc_start()\n"), this->get_label()); Serial.flush();
                    handle_mmc_start();
                }
            }
        }

        virtual void handle_mmc_record() {
            if (loop_track!=nullptr)
                loop_track->toggle_recording();
        }
        virtual void handle_mmc_start() {
            if (loop_track!=nullptr) {
                loop_track->start_playing();
                playing = true;
            }
        }
        virtual void handle_mmc_stop() {
            if (loop_track!=nullptr) {
                loop_track->stop_recording();
                loop_track->stop_playing();
            }
        }

};

extern DeviceBehaviour_mpk49 *behaviour_mpk49;

#endif