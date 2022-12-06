#ifndef BEHAVIOUR_BEATSTEP__INCLUDED
#define BEHAVIOUR_BEATSTEP__INCLUDED

#include <Arduino.h>

#include "Config.h"

#ifdef ENABLE_BEATSTEP

#include "behaviours/behaviour_base_usb.h"
#include "behaviours/behaviour_clocked.h"

#include "multi_usb_handlers.h"

extern MIDIOutputWrapper *beatstep_output;

//void beatstep_setOutputWrapper(MIDIOutputWrapper *);
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
            return "BeatStep";
        }
        virtual bool has_input() { return true; }

        FLASHMEM 
        virtual void setup_callbacks() override {
            if (!DeviceBehaviourUSBBase::is_connected()) return;

            this->device->setHandleNoteOn(beatstep_handle_note_on);
            this->device->setHandleNoteOff(beatstep_handle_note_off);
        }

        virtual void receive_note_on(uint8_t channel, uint8_t note, uint8_t velocity) override {
            //Serial.printf("beatstep got note on %i\n", note); Serial_flush();

            this->current_note = note;
            ClockedBehaviour::receive_note_on(channel, note, 127);
        }

        virtual void receive_note_off(uint8_t channel, uint8_t note, uint8_t velocity) override {
            //Serial.printf("beatstep got note off %i\n", note); Serial_flush();

            // update current / remember last played note
            this->last_note = note;
            if (this->current_note==note) 
                current_note = -1;
            ClockedBehaviour::receive_note_off(channel, note, 127);
        }

        #ifdef ENABLE_BEATSTEP_SYSEX
            // thank you to https://www.untergeek.de/2014/11/taming-arturias-beatstep-sysex-codes-for-programming-via-ipad/ for this info
            // stuff for advancing pattern
            void set_auto_advance_pattern(bool auto_advance_pattern) {
                this->auto_advance_pattern = auto_advance_pattern;
            }
            bool is_auto_advance_pattern() {
                return this->auto_advance_pattern;
            }
            virtual void on_end_phrase(uint32_t phrase) override {
                if (this->device==nullptr) return;

                DividedClockedBehaviour::on_end_phrase(phrase);

                if (this->auto_advance_pattern) {
                    int phrase_number = (phrase % NUM_PATTERNS);
                    this->send_preset_change(phrase_number);
                    //this->on_restart(); 
                    this->set_restart_on_bar(true);
                }
            }
            void send_preset_change(int phrase_number) {
                if (this->device==nullptr) return;

                Debug_printf(F("beatstep#send_preset_change(%i)\n"), phrase_number % NUM_PATTERNS);

                uint8_t data[] = {
                    0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x05, (uint8_t)/*1+*/(phrase_number % NUM_PATTERNS), 0xF7
                };
                this->device->sendSysEx(sizeof(data), data, true);
            }

            // pattern length settings
            byte pattern_length = 16;
            void setPatternLength(byte length) {
                // Pattern length: F0 00 20 6B 7F 42 02 00 50 06 nn F7 (1-16 steps, 1-0x10).
                length = constrain(length,1,16);
                //if (this->pattern_length!=length) {
                    uint8_t data[] = {
                        0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x02, 0x00, 0x50, 0x06, (byte)(length), 0xF7
                    };
                    this->device->sendSysEx(sizeof(data), data, true);
                    Debug_printf(F("%s#setPatternLength(%i)\n"), this->get_label(), length);
                //}
                this->pattern_length = length;
            }
            byte getPatternLength() {
                return pattern_length;
            }

            //playback direction settings
            byte direction = 0;
            void setDirection(byte direction) {
                direction = constrain(direction,0,3);
                uint8_t data[] = {
                    0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x02, 0x00, 0x50, 0x04, (byte)(direction), 0xF7
                };
                this->device->sendSysEx(sizeof(data), data, true);
                this->direction = direction;
            }
            byte getDirection() {
                return this->direction;
            }

            /*class BeatstepDirectionParameter : public DataParameter<DeviceBehaviour_Beatstep,byte> {
                public:
                BeatstepDirectionParameter (char *label, DeviceBehaviour_Beatstep *target) 
                    : DataParameter<DeviceBehaviour_Beatstep,byte>(label, target, &DeviceBehaviour_Beatstep::setDirection, &DeviceBehaviour_Beatstep::getDirection)
                    {}

                virtual const char* parseFormattedDataType(byte value) override {
                    switch (value) {
                        case 0:
                            return "Fwd";
                        case 1:
                            return "Rev";
                        case 2:
                            return "Alt";
                        case 3:
                            return "Ran";
                        default:
                            return "??";
                    }
                    return "??";
                };
            };*/

            FLASHMEM
            virtual LinkedList<DoubleParameter*> *initialise_parameters() override {
                static bool already_initialised = false;
                if (already_initialised)
                    return this->parameters;
                DeviceBehaviourUSBBase::initialise_parameters();
                //Serial.println(F("\tcalling ClockedBehaviour::initialise_parameters()"));
                ClockedBehaviour::initialise_parameters();

                // this actually isn't much use as a modulation target since the beatstep restarts pattern when this changes - made instead into a regular menuitem thing
                /*this->parameters->add((new DataParameter<DeviceBehaviour_Beatstep,byte>(
                        (const char*)"Pattern length", 
                        this, 
                        &DeviceBehaviour_Beatstep::setPatternLength, 
                        &DeviceBehaviour_Beatstep::getPatternLength
                ))->initialise_values(1,16)->set_modulatable(false));

                this->parameters->add(new BeatstepDirectionParameter("Direction", this));*/

                already_initialised = true;

                return parameters;
            }

            virtual void add_save_lines(LinkedList<String> *lines) override {   
                DeviceBehaviourUltimateBase::add_save_lines(lines);
                DividedClockedBehaviour::add_save_lines(lines);

                lines->add(String(F("pattern_length=")) + String(this->getPatternLength()));
                lines->add(String(F("pattern_direction=")) + String(this->getDirection()));
            }
            virtual bool load_parse_key_value(String key, String value) {
                if (key.equals(F("pattern_length"))) {
                    this->setPatternLength(value.toInt());
                    return true;
                } else if (key.equals(F("pattern_direction"))) {
                    this->setDirection(value.toInt());
                    return true;
                } else if (DividedClockedBehaviour::load_parse_key_value(key, value)) {
                    return true;
                } else if (DeviceBehaviourUltimateBase::load_parse_key_value(key, value)) {
                    return true;
                }
                return false;
            }

        #endif

        #ifdef ENABLE_SCREEN
            //FLASHMEM
            LinkedList<MenuItem*> *make_menu_items() override;
        #endif

};

extern DeviceBehaviour_Beatstep *behaviour_beatstep;

#endif

#endif