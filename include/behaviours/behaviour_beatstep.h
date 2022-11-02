#ifndef BEHAVIOUR_BEATSTEP__INCLUDED
#define BEHAVIOUR_BEATSTEP__INCLUDED

#include <Arduino.h>

#include "Config.h"

#ifdef ENABLE_BEATSTEP

#include "behaviours/behaviour_base_usb.h"
#include "behaviours/behaviour_clocked.h"

#include "multi_usb_handlers.h"

extern MIDIOutputWrapper *beatstep_output;

void beatstep_setOutputWrapper(MIDIOutputWrapper *);

//void beatstep_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void beatstep_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void beatstep_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);

class DeviceBehaviour_Beatstep : public DeviceBehaviourUSBBase, public DividedClockedBehaviour {
    using DividedClockedBehaviour::on_restart;
    public:
        #define NUM_PATTERNS 16
        bool auto_advance_pattern = false;   // todo: make configurable!

        int last_note = -1, current_note = -1;
        //int last_transposed_note = -1, current_transposed_note = -1;

        uint16_t vid = 0x1c75, pid = 0x0206;
        virtual uint32_t get_packed_id () override { return (this->vid<<16 | this->pid); }

        virtual const char *get_label() override {
            return (char*)"BeatStep";
        }
        virtual bool has_input() { return true; }

        FLASHMEM 
        virtual void setup_callbacks() override {
            if (!DeviceBehaviourUSBBase::is_connected()) return;

            this->device->setHandleNoteOn(beatstep_handle_note_on);
            this->device->setHandleNoteOff(beatstep_handle_note_off);
        }

        virtual void receive_note_on(uint8_t channel, uint8_t note, uint8_t velocity) override {
            //Serial.printf("beatstep got note on %i\n", note); Serial.flush();

            this->current_note = note;
            ClockedBehaviour::receive_note_on(channel, note, 127);
        }

        virtual void receive_note_off(uint8_t channel, uint8_t note, uint8_t velocity) override {
            //Serial.printf("beatstep got note off %i\n", note); Serial.flush();

            // update current / remember last played note
            this->last_note = note;
            if (this->current_note==note) 
                current_note = -1;
            ClockedBehaviour::receive_note_off(channel, note, 127);
        }

        #ifdef ENABLE_BEATSTEP_SYSEX
            void set_auto_advance_pattern(bool auto_advance_pattern) {
                this->auto_advance_pattern = auto_advance_pattern;
            }
            bool is_auto_advance_pattern() {
                return this->auto_advance_pattern;
            }

            virtual void on_phrase(uint32_t phrase) override {
                if (this->device==nullptr) return;

                DividedClockedBehaviour::on_phrase(phrase);

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

        #ifdef ENABLE_SCREEN
            LinkedList<MenuItem*> *make_menu_items() override;
        #endif

};

extern DeviceBehaviour_Beatstep *behaviour_beatstep;

#endif

#endif