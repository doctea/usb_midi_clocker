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

//void beatstep_setOutputWrapper(MIDIOutputWrapper *);
//void beatstep_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void beatstep_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void beatstep_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void beatstep_handle_sysex(const uint8_t *data, uint16_t length, bool complete);

#define BEATSTEP_PATTERN_LENGTH_MINIMUM 1
#define BEATSTEP_PATTERN_LENGTH_MAXIMUM 16

#define BEATSTEP_GLOBAL         0x50
#define BEATSTEP_STEP_PITCH     0x52
#define BEATSTEP_STEP_STATE     0x53

// todo: note that the first requested sysex parameter is often missed!  so in this case 'TRANSPOSE' is kinda sacrificial
#define BEATSTEP_TRANSPOSE      0x02
#define BEATSTEP_DIRECTION      0x04
#define BEATSTEP_PATTERN_LENGTH 0x06
#define BEATSTEP_SWING          0x07
#define BEATSTEP_GATE           0x08
#define BEATSTEP_LEGATO         0x09

#define SYSEX_TIMEOUT   50   // timeout if a request for a sysex parameter doesn't get a response, so that the queue doesn't get stuck

class DeviceBehaviour_Beatstep : public DeviceBehaviourUSBBase, public DividedClockedBehaviour {
    using DividedClockedBehaviour::on_restart;
    
    public:
        #define NUM_PATTERNS 16
        bool auto_advance_pattern = false; 

        bool query_pattern = false;

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

        bool already_initialised = false;
        FLASHMEM
        virtual LinkedList<DoubleParameter*> *initialise_parameters() override {
            //static bool already_initialised = false;
            if (already_initialised && this->parameters!=nullptr)
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

        #ifdef ENABLE_BEATSTEP_SYSEX
            // thank you to https://www.untergeek.de/2014/11/taming-arturias-beatstep-sysex-codes-for-programming-via-ipad/ for this info

            // pattern stuff
            struct StepData {
                bool state = false;
                int8_t pitch = -1;
            };

            class BeatstepPattern {
                #define NUM_BEATSTEP_STEPS 16
                StepData *step = nullptr;
                int rotation = 0;
                public:
                    DeviceBehaviour_Beatstep *target = nullptr;
                    BeatstepPattern(DeviceBehaviour_Beatstep *target) {
                        this->target = target;
                        step = (StepData*)calloc(NUM_BEATSTEP_STEPS, sizeof(StepData));
                        for(int i = 0 ; i < NUM_BEATSTEP_STEPS ; i++) {
                            step[i] = { .state = false, .pitch = -1 };
                        }
                    }
                    int get_rotated_index(int i) {
                        return (i + rotation) % NUM_BEATSTEP_STEPS;
                    }
                    void send_to_beatstep() {
                        Serial.printf("----send_to_beatstep with rotation %i----\n", rotation);
                        for (int i = 0 ; i < NUM_BEATSTEP_STEPS ; i++) {
                            Serial.printf(
                                "send_to_beatstep: %i\t(rotated to %i)\tis %s\t(pitch %s)\n", 
                                i, 
                                get_rotated_index(i), 
                                get_state_at_step(i)?"ON":"OFF", 
                                get_note_name_c(get_pitch_at_step(i))
                            );
                            target->set_sysex_parameter(BEATSTEP_STEP_PITCH, i, get_pitch_at_step(i));
                            //delay(10);
                            //target->set_sysex_parameter(BEATSTEP_STEP_STATE, i, get_state_at_step(i) ? 0x7F : 0);
                        }
                        for (int i = 0 ; i < NUM_BEATSTEP_STEPS ; i++) {
                            //Serial.printf("send_to_beatstep: %i\t(rotated to %i)\tis %s\t(pitch %s)\n", i, get_rotated_index(i), get_state_at_step(i)?"ON":"OFF", get_note_name_c(get_pitch_at_step(i)));
                            //target->set_sysex_parameter(BEATSTEP_STEP_PITCH, i, get_pitch_at_step(i));
                            //delay(10);
                            target->set_sysex_parameter(BEATSTEP_STEP_STATE, i, get_state_at_step(i) ? 1 : 0);
                        }   
                    }

                    void request_from_beatstep () {
                        for (int i = 0 ; i < NUM_BEATSTEP_STEPS ; i++) {
                            int delay = NUM_BEATSTEP_STEPS;
                            target->request_sysex_parameter(BEATSTEP_STEP_PITCH, i, delay);
                            target->request_sysex_parameter(BEATSTEP_STEP_STATE, i);
                        }
                    }
                    void process_received_sysex(int8_t parameter_number, int8_t step_number, int8_t value) {
                        step_number = get_rotated_index(step_number);
                        if (parameter_number==BEATSTEP_STEP_PITCH) {
                            //MIDI note of step
                            //this->set_pitch_at_step(step_number, value);
                            this->step[step_number].pitch = value;
                        } else if (parameter_number==BEATSTEP_STEP_STATE) {
                            //this->set_state_at_step(step_number, value);
                            this->step[step_number].state = value;
                        }
                    }
                    void rotate(int rotate_value) {
                        this->rotation = rotate_value;
                    }
                    int get_rotation() {
                        return this->rotation;
                    }
                    StepData get_step_actual(int i) {
                        return (this->step[i]);
                    }
                    int8_t get_pitch_at_step(int i) {
                        return this->get_step_actual(get_rotated_index(i)).pitch;
                    }
                    bool get_state_at_step(int i) {
                        return this->get_step_actual(get_rotated_index(i)).state;
                    }

                    void set_pitch_at_step(int i, int8_t pitch) {
                        //this->step[get_rotated_index(i) % NUM_BEATSTEP_STEPS].value = value;
                        this->step[get_rotated_index(i)].pitch = pitch;
                    }
                    void set_state_at_step(int i, bool state) {
                        this->step[get_rotated_index(i)].state = state;
                    }
            };

            BeatstepPattern *sequence = new BeatstepPattern(this);

            void clear_beatstep() {
                for (int i = 0 ; i < NUM_BEATSTEP_STEPS ; i++) {
                    //int delay = NUM_BEATSTEP_STEPS;
                    set_sysex_parameter(BEATSTEP_STEP_PITCH, i, 0);
                    delay(10);
                    set_sysex_parameter(BEATSTEP_STEP_STATE, i, 0);
                    delay(10);
                }
            }
            void request_sequence_from_beatstep() {
                this->sequence->request_from_beatstep();
            }
            void push_sequence_to_beatstep() {
                this->sequence->send_to_beatstep();
            }
            void setSequenceRotation(int r) {
                this->sequence->rotate(r);
            }
            int getSequenceRotation() {
                return this->sequence->get_rotation();
            }
            
            // stuff for advancing pattern
            void set_auto_advance_pattern(bool auto_advance_pattern) {
                this->auto_advance_pattern = auto_advance_pattern;
            }
            bool is_auto_advance_pattern() {
                return this->auto_advance_pattern;
            }
            
            virtual void on_end_phrase_pre_clock(uint32_t phrase) override {
                if (this->device==nullptr) return;

                if (this->auto_advance_pattern) {
                    int phrase_number = (phrase % NUM_PATTERNS);
                    this->send_preset_change(phrase_number);
                    //this->on_restart(); 
                    this->set_restart_on_bar(true);
                }

                DividedClockedBehaviour::on_end_phrase_pre_clock(phrase);
            }
            void send_preset_change(int phrase_number) {
                if (this->device==nullptr) return;

                Serial.printf(F("beatstep#send_preset_change(%i)\n"), phrase_number % NUM_PATTERNS);

                uint8_t data[] = {
                    0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x05, (uint8_t)/*1+*/(phrase_number % NUM_PATTERNS), 0xF7
                };
                this->device->sendSysEx(sizeof(data), data, true);

                //this->request_all_sysex_parameters(50);
            }

            void on_bar(int bar) override {
                DividedClockedBehaviour::on_bar(bar);
                this->request_all_sysex_parameters();
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

            // note swing setting
            // todo: make this modulatable parameters, since they dont force a pattern restart when changed
            int8_t swing = 0;
            void setSwing(int8_t swing) {
                swing = constrain(swing, 0x32, 0x4b);
                this->set_sysex_parameter(BEATSTEP_GLOBAL, BEATSTEP_SWING, swing);
                this->swing = swing;
            }
            int8_t getSwing() {
                return swing;
            }

            // note gate length setting
            // todo: make this modulatable parameters, since they dont force a pattern restart when changed
            int8_t gate = 0;
            void setGate(int8_t gate) {
                gate = constrain(gate, 0, 0x63);
                this->set_sysex_parameter(BEATSTEP_GLOBAL, BEATSTEP_GATE, gate);
                this->gate = gate;
            }
            int8_t getGate() {
                return gate;
            }

            // note legato settings
            // todo: make this modulatable parameters, since they dont force a pattern restart when changed
            int8_t legato = 0;
            void setLegato(int8_t legato) {
                legato = constrain(legato, 0, 2);
                this->set_sysex_parameter(BEATSTEP_GLOBAL, BEATSTEP_LEGATO, legato);
                this->legato = legato;
            }
            int8_t getLegato() {
                return legato;
            }

            struct sysex_parameter_t {
                const int8_t cc;
                const int8_t pp;
                int8_t *target_variable = nullptr;
                const char *label = nullptr;
                void(DeviceBehaviour_Beatstep::*setter_func)(int8_t) = nullptr;
                bool enable_recall = true;
            };
            #define NUM_SYSEX_PARAMETERS 6

            // proof of concept of fetching parameter values from beatstep over sysex

            bool debug_sysex = true;

            sysex_parameter_t sysex_parameters[NUM_SYSEX_PARAMETERS] {
                { BEATSTEP_GLOBAL, BEATSTEP_TRANSPOSE, nullptr, "Transpose"},    // transpose (unused, sacrificial to the gods of missing beatstep data)
                { BEATSTEP_GLOBAL, BEATSTEP_DIRECTION, &this->direction, "Direction", &DeviceBehaviour_Beatstep::setDirection },
                { BEATSTEP_GLOBAL, BEATSTEP_PATTERN_LENGTH, &this->pattern_length, "Steps", &DeviceBehaviour_Beatstep::setPatternLength },
                { BEATSTEP_GLOBAL, BEATSTEP_SWING, &this->swing, "Swing", &DeviceBehaviour_Beatstep::setSwing },        // swing, 0x32 to 0x4b (ie 50-100%)
                { BEATSTEP_GLOBAL, BEATSTEP_GATE, &this->gate, "Gate", &DeviceBehaviour_Beatstep::setGate },            // gate length, 0x32 to 0x63
                { BEATSTEP_GLOBAL, BEATSTEP_LEGATO, &this->legato, "Legato", &DeviceBehaviour_Beatstep::setLegato }     // legato 0=off, 1=on, 2=reset
            };

            struct BeatstepSysexRequest {
                byte pp;
                byte cc;
            };
            struct BeatstepSysexSetRequest {
                byte pp;
                byte cc;
                byte vv;
            };
            Queue<BeatstepSysexRequest,50> *sysex_request_queue = new Queue<BeatstepSysexRequest,50>();
            Queue<BeatstepSysexSetRequest,50> *sysex_send_queue = new Queue<BeatstepSysexSetRequest,50>();

            // called every main loop - process any queued requests
            void loop(uint32_t ticks) override {
                if (sysex_request_queue->isReady() && this->device) {
                    BeatstepSysexRequest *req = sysex_request_queue->pop();
                    //Serial.printf("loop() dequeued %02x,%02x\n", req->cc, req->pp);
                    this->process_sysex_request(*req);
                }
                if (sysex_send_queue->isReady() && this->device) {
                    BeatstepSysexSetRequest *req = sysex_send_queue->pop();
                    Serial.println("sysex_send_queue dequeued an item");
                    this->process_set_sysex_parameter(*req);
                }
                return DividedClockedBehaviour::loop(ticks);
            }

            // snoop on every realtime MIDI message sent to the beatstep, and request the latest values if its a Start message, in case we've changed pattern
            virtual void sendRealTime(uint8_t message) override {
                DividedClockedBehaviour::sendRealTime(message);
                if (message==(uint8_t)(midi::Start))
                    this->request_all_sysex_parameters(10);
            }

            // for testing
            void request_all_sysex_parameters() {
                request_all_sysex_parameters(3);
            }
            void request_all_sysex_parameters(int delay) {
                //if (debug_sysex) Serial.printf("request_all_sysex_parameters with delay %i!!\n", delay);
                /*this->request_sysex_parameter(BEATSTEP_GLOBAL, BEATSTEP_PATTERN_LENGTH, delay);
                this->request_sysex_parameter(BEATSTEP_GLOBAL, BEATSTEP_DIRECTION, 0);            */
                for (unsigned int i = 0 ; i < NUM_SYSEX_PARAMETERS ; i++) {
                    this->request_sysex_parameter(sysex_parameters[i].cc, sysex_parameters[i].pp);
                }
                if (this->query_pattern)
                    this->sequence->request_from_beatstep();
            }

            // put a new request into the sysex queue
            void request_sysex_parameter(byte pp, byte cc, int delay = 0) {
                //if (debug_sysex) Serial.printf("Queueing get request for %02x, %02x with %ims delay\n", pp, cc, delay);
                this->sysex_request_queue->push(BeatstepSysexRequest {pp, cc}, SYSEX_TIMEOUT, delay);
            }
            // queues set request
            void set_sysex_parameter(byte pp, byte cc, byte vv, int delay = 2
             ) {
                if (debug_sysex) Serial.printf("Queueing set request for %02x, %02x, %02x, with %ims delay\n", pp, cc, vv, delay);
                sysex_send_queue->push(BeatstepSysexSetRequest { pp, cc, vv }, 0, delay);
            }

            // actually send a dequeued request, and re-pause the queue
            void process_sysex_request(BeatstepSysexRequest req) {
                const uint8_t data[] = { 0xF0,0x00,0x20,0x6B,0x7F,0x42,0x01,0x00,req.pp,req.cc,0xF7 };
                if (debug_sysex) {
                    Serial.print("Sending get Sysex to BeatStep\t[ ");
                    for(uint32_t i = 0 ; i < sizeof(data) ; i++) 
                        Serial.printf("%02x ", data[i]);
                    Serial.println("] (request)");
                }
                if (this->device) {
                    //this->device->sendSysEx(sizeof(data), data, true);
                    this->device->sendSysEx(sizeof(data), data, true);
                }
                sysex_request_queue->setPaused(true);
            }
            // actually set a beatstep sysex parameter
            void process_set_sysex_parameter(BeatstepSysexSetRequest req) {
                this->process_set_sysex_parameter(req.pp, req.cc, req.vv);
            }
            // actually set a beatstep sysex parameter
            void process_set_sysex_parameter(byte pp, byte cc, byte vv) {
                const uint8_t data[] = { 0xF0,0x00,0x20,0x6B,0x7F,0x42,0x02,0x00,pp,cc,vv,0xF7 };
                if (debug_sysex) {
                    Serial.print("Sending set Sysex to BeatStep\t[ ");
                    for(uint32_t i = 0 ; i < sizeof(data) ; i++) 
                        Serial.printf("%02x ", data[i]);
                    Serial.printf("] (setting %02x,%02x to %02x)\n", pp, cc, vv);
                }
                if (this->device)
                    this->device->sendSysEx(sizeof(data), data, true);
            }

            // handles incoming sysex from beatstep to update internal state - unpauses the queue if we've received something
            void handle_sysex(const uint8_t *data, uint16_t length, bool complete) {
                if (debug_sysex) {
                    Serial.print("BeatStep replied with Sysex:\t[ ");
                    for (uint32_t i = 0 ; i < length ; i++) {
                        Serial.printf("%02x ", data[i]);
                    }
                    Serial.print("] ");
                    Serial.print(complete? "complete" : "incomplete");
                    Serial.println();
                }
                #define BROAD_POS 8
                #define SPEC_POS 9
                #define VALUE_POS 10
                if (length < BROAD_POS-1 || !complete) {
                    if (debug_sysex) Serial.printf("handle_sysex received incomplete message with length %i - ignoring!\n", length);
                    return;
                }
                if (data[BROAD_POS]==0x52 || data[BROAD_POS]==0x53) {
                    this->sequence->process_received_sysex(data[BROAD_POS], data[SPEC_POS], data[VALUE_POS]);
                } else {
                    for (unsigned int i = 0 ; i < NUM_SYSEX_PARAMETERS ; i++) {
                        if (sysex_parameters[i].cc==data[BROAD_POS] && sysex_parameters[i].pp==data[SPEC_POS]) {
                            if (sysex_parameters[i].target_variable!=nullptr)
                                *sysex_parameters[i].target_variable = data[VALUE_POS];
                        }
                    }
                }

                sysex_request_queue->setPaused(false);
            }


            virtual void save_sequence_add_lines(LinkedList<String> *lines) override {   
                DeviceBehaviourUltimateBase::save_sequence_add_lines(lines);
                DividedClockedBehaviour::save_sequence_add_lines(lines);

                for (unsigned int i = 0 ; i < NUM_SYSEX_PARAMETERS ; i++) {
                    if (!sysex_parameters[i].enable_recall) continue;
                    if (sysex_parameters[i].target_variable==nullptr) continue;
                    //Serial.printf("Beatstep#save_sequence_add_lines processing: %i '%s'\n", i, sysex_parameters[i].label);

                    String line =   String(sysex_parameters[i].label) + 
                                    String("=") + 
                                    String(*sysex_parameters[i].target_variable);
                    //Serial.printf("Beatstep#save_sequence_add_lines got line: %s\n", line.c_str());
                    lines->add(line);
                }
            }
            virtual bool load_parse_key_value(String key, String value) override {
                for (unsigned int i = 0 ; i < NUM_SYSEX_PARAMETERS ; i++) {
                    if (!sysex_parameters[i].enable_recall) 
                        continue;
                    if (key.equals(sysex_parameters[i].label)) {
                        if (sysex_parameters[i].setter_func==nullptr) 
                            break;
                        (this->*sysex_parameters[i].setter_func)(value.toInt()); 
                        return true;
                    }
                }
                if (DividedClockedBehaviour::load_parse_key_value(key, value)) {
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

#include "menuitems_object_multitoggle.h"

class BeatstepSysexOptionToggle : public MultiToggleItemBase {
    DeviceBehaviour_Beatstep::sysex_parameter_t *target_sysex_parameter = nullptr;
    DeviceBehaviour_Beatstep *target_object = nullptr;

    public:
        BeatstepSysexOptionToggle(DeviceBehaviour_Beatstep *target_object, DeviceBehaviour_Beatstep::sysex_parameter_t *pattern) 
            : MultiToggleItemBase(pattern->label) {
            this->target_object = target_object;
            this->target_sysex_parameter = pattern;
            this->label = pattern->label;
        }
        virtual bool do_getter() override {
            return this->target_sysex_parameter->enable_recall;
        }
        virtual void do_setter(bool state) override {
            target_sysex_parameter->enable_recall = state;
        }
};

#endif



#endif