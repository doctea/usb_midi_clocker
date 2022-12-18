#ifndef BEHAVIOUR_BEATSTEP__INCLUDED
#define BEHAVIOUR_BEATSTEP__INCLUDED

#include <Arduino.h>

#include "Config.h"

#ifdef ENABLE_BEATSTEP

#include "behaviours/behaviour_base_usb.h"
#include "behaviours/behaviour_clocked.h"

#include "multi_usb_handlers.h"

#include "queue.h"

extern MIDIOutputWrapper *beatstep_output;

#define BEATSTEP_PATTERN_LENGTH_MINIMUM 1
#define BEATSTEP_PATTERN_LENGTH_MAXIMUM 16

#define BEATSTEP_GLOBAL         0x50

#define BEATSTEP_DIRECTION      0x04
#define BEATSTEP_PATTERN_LENGTH 0x06

#define SYSEX_TIMEOUT   50   // timeout if a request for a sysex parameter doesn't get a response, so that the queue doesn't get stuck

//void beatstep_setOutputWrapper(MIDIOutputWrapper *);
//void beatstep_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void beatstep_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void beatstep_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void beatstep_handle_sysex(const uint8_t *data, uint16_t length, bool complete);

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
            this->device->setHandleSysEx(beatstep_handle_sysex);
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

        FLASHMEM
        virtual LinkedList<DoubleParameter*> *initialise_parameters() override {
            static bool already_initialised = false;
            if (already_initialised)
                return this->parameters;
            DeviceBehaviourUSBBase::initialise_parameters();
            //Serial.println(F("\tcalling ClockedBehaviour::initialise_parameters()"));
            ClockedBehaviour::initialise_parameters();

            // not very useful as a modulation target since it forces a restart of the sequence when changed
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

        // proof of concept of fetching parameter values from beatstep over sysex
        struct BeatstepSysexRequest {
            byte pp;
            byte cc;
        };
        Queue<BeatstepSysexRequest,50> *sysex_request_queue = new Queue<BeatstepSysexRequest,50>();
        bool sysex_request_in_flight = false;

        // called every main loop - process any queued requests
        void loop(uint32_t ticks) override {
            if (sysex_request_queue->isReady() && this->device) {
                BeatstepSysexRequest *req = sysex_request_queue->pop();
                //Serial.printf("loop() dequeued %02x,%02x\n", req->cc, req->pp);
                this->process_sysex_request(*req);
            }
            return DividedClockedBehaviour::loop(ticks);
        }

        // snoop on every realtime MIDI message sent to the beatstep, and request the latest values if its a Start message, in case we've changed pattern
        virtual void sendRealTime(uint8_t message) override {
            DividedClockedBehaviour::sendRealTime(message);
            if (message==(uint8_t)(midi::Start))
                this->testRequest(10);
        }

        // for testing
        void testRequest() {
            testRequest(3);
        }
        void testRequest(int delay) {
            Serial.println("testRequest!!");
            this->request_sysex_parameter(BEATSTEP_GLOBAL, BEATSTEP_PATTERN_LENGTH, delay);
            this->request_sysex_parameter(BEATSTEP_GLOBAL, BEATSTEP_DIRECTION, 0);            
        }

        // actually send a dequeued request, and re-pause the queue
        void process_sysex_request(BeatstepSysexRequest req) {
            const uint8_t data[] = { 0xF0,0x00,0x20,0x6B,0x7F,0x42,0x01,0x00,req.pp,req.cc,0xF7 };
            Serial.print("Sending Sysex to BeatStep\t[ ");
            for(uint32_t i = 0 ; i < sizeof(data) ; i++) 
                Serial.printf("%02x ", data[i]);
            Serial.println("] (request)");
            if (this->device) {
                //this->device->sendSysEx(sizeof(data), data, true);
                this->device->sendSysEx(sizeof(data), data, true);
            }
            sysex_request_queue->setPaused(true);
        }
        // put a new request into the sysex queue
        void request_sysex_parameter(byte pp, byte cc, int delay = 0) {
            Serial.printf("queueing request for %02x, %02x with %ims delay\n", pp, cc, delay);
            this->sysex_request_queue->push(BeatstepSysexRequest {pp, cc}, SYSEX_TIMEOUT, delay);
        }
        // set a beatstep sysex parameter
        void set_sysex_parameter(byte pp, byte cc, byte vv) {
            const uint8_t data[] = { 0xF0,0x00,0x20,0x6B,0x7F,0x42,0x02,0x00,pp,cc,vv,0xF7 };
                                   //0xF0,0x00,0x20,0x6B,0x7F,0x42,0x02,0x00,0x50,0x04,(byte)(direction), 0xF7
            Serial.print("Sending Sysex to BeatStep\t[ ");
            for(uint32_t i = 0 ; i < sizeof(data) ; i++) 
                Serial.printf("%02x ", data[i]);
            Serial.printf("] (setting %02x,%02x to %02x)\n", pp, cc, vv);
            if (this->device)
                this->device->sendSysEx(sizeof(data), data, true);
        }

        // handles incoming sysex from beatstep to update internal state - unpause queue if we've received something
        void handle_sysex(const uint8_t *data, uint16_t length, bool complete) {
            Serial.print("BeatStep replied with Sysex:\t[ ");
            for (uint32_t i = 0 ; i < length ; i++) {
                Serial.printf("%02x ", data[i]);
            }
            Serial.print("] ");
            Serial.print(complete? "complete" : "incomplete");
            Serial.println();
            #define BROAD_POS 8
            #define SPEC_POS 9
            #define VALUE_POS 10
            if (length < BROAD_POS-1 || !complete) {
                Serial.printf("handle_sysex received incomplete message with length %i - ignoring!\n", length);
                return;
            }
            if (data[BROAD_POS]==BEATSTEP_GLOBAL) {
                if (data[SPEC_POS]==BEATSTEP_DIRECTION) {
                    this->direction = data[VALUE_POS];  // dont use setDirection 'cos that will re-send the change
                } else if (data[SPEC_POS]==BEATSTEP_PATTERN_LENGTH) {
                    this->pattern_length = data[VALUE_POS];
                }
            }
            //Serial.println("clearing sysex_request_in_flight flag");
            sysex_request_queue->setPaused(false);
            //this->sysex_request_in_flight = false;
        }

        void on_bar(int bar) override {
            DividedClockedBehaviour::on_bar(bar);
            this->testRequest();
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

                this->testRequest(50);
            }

            // pattern length settings
            int8_t pattern_length = 16;
            void setPatternLength(int8_t length) {
                length = constrain(length,BEATSTEP_PATTERN_LENGTH_MINIMUM,BEATSTEP_PATTERN_LENGTH_MAXIMUM);
                this->set_sysex_parameter(BEATSTEP_GLOBAL, BEATSTEP_PATTERN_LENGTH, length);
                this->pattern_length = length;
            }
            int8_t getPatternLength() {
                return pattern_length;
            }

            //playback direction settings
            int8_t direction = 0;
            void setDirection(int8_t direction) {
                direction = constrain(direction,0,3);
                this->set_sysex_parameter(BEATSTEP_GLOBAL, BEATSTEP_DIRECTION, direction);
                this->direction = direction;
            }
            int8_t getDirection() {
                return this->direction;
            }

            virtual void save_sequence_add_lines(LinkedList<String> *lines) override {   
                DeviceBehaviourUltimateBase::save_sequence_add_lines(lines);
                DividedClockedBehaviour::save_sequence_add_lines(lines);

                lines->add(String(F("pattern_length=")) + String(this->getPatternLength()));
                lines->add(String(F("pattern_direction=")) + String(this->getDirection()));
            }
            virtual bool load_parse_key_value(String key, String value) override {
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