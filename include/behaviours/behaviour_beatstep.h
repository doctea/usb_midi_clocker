#ifndef BEHAVIOUR_BEATSTEP__INCLUDED
#define BEHAVIOUR_BEATSTEP__INCLUDED

#include <Arduino.h>

#include "Config.h"

#ifdef ENABLE_BEATSTEP

#include "behaviours/behaviour_base_usb.h"
#include "behaviours/behaviour_clocked.h"

#include "usb/multi_usb_handlers.h"

#include "queue.h"

#include "SaveableParameters.h"

extern MIDIOutputWrapper *beatstep_output;

//void beatstep_setOutputWrapper(MIDIOutputWrapper *);
//void beatstep_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void beatstep_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void beatstep_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void beatstep_handle_sysex(const uint8_t *data, uint16_t length, bool complete);

#ifdef ENABLE_BEATSTEP_2
    void beatstep_2_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
    void beatstep_2_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
    void beatstep_2_handle_sysex(const uint8_t *data, uint16_t length, bool complete);
#endif

#define BEATSTEP_PATTERN_LENGTH_MINIMUM 1
#define BEATSTEP_PATTERN_LENGTH_MAXIMUM 16

// Step size: (0=1/4, 1=1/8, 2=1/16, 3=1/32)
#define BEATSTEP_PATTERN_STEP_SIZE_MINIMUM 0
#define BEATSTEP_PATTERN_STEP_SIZE_MAXIMUM 3

#define BEATSTEP_GLOBAL         0x50

// todo: note that the first requested sysex parameter is often missed!  so in this case 'TRANSPOSE' is kinda sacrificial
#define BEATSTEP_TRANSPOSE      0x02
#define BEATSTEP_DIRECTION      0x04
#define BEATSTEP_STEP_SIZE      0x05
#define BEATSTEP_PATTERN_LENGTH 0x06
#define BEATSTEP_SWING          0x07
#define BEATSTEP_GATE           0x08
#define BEATSTEP_LEGATO         0x09

#define SYSEX_TIMEOUT   50   // timeout if a request for a sysex parameter doesn't get a response, so that the queue doesn't get stuck

#define BEATSTEP_SYSEX_BASE_ENCODER 0x20
#define BEATSTEP_SYSEX_BASE_PADS    0x70
#define NUM_ENCODERS                16
#define NUM_PADS                    16
#define CHANNEL_GLOBAL              0x41
#define SYSEX_CHANNEL               0x02
#define SYSEX_CCNOTE                0x03
#define SYSEX_BEHAVIOUR             0x06
#define SYSEX_PAD_MODE_OFF          0x00
#define SYSEX_PAD_MODE_SILENT_CC_SWITCH 0x01
#define SYSEX_PAD_MODE_MMC          0x07
#define SYSEX_PAD_MODE_CC_SWITCH    0x08
#define SYSEX_PAD_MODE_NOTE         0x09

/*
For future configuration of Beatstep pads, to implement a 'second sequencer' mode:-

from https://www.untergeek.de/2014/11/taming-arturias-beatstep-sysex-codes-for-programming-via-ipad/

PADS
----
The pads 1-16 have controller numbers 0x70 to 0x7F.
    F0 00 20 6B 7F 42 02 00 01 cc vv F7
        cc is the controller you like to program, 
        vv sets the mode: 0=off, 1=Silent CC Switch, 7=MMC, 8=CC Switch, 9=Note, 0x0B=Program Change. Hm. Makes you wonder which modes the missing values might activate.
            SILENT CC SWITCH MODE (VV=1) Works just like CC, but doesn’t light up the (red) pad LED while it is pressed, unlike the CC (vv=8) mode.

    Setting the parameters: Send F0 00 20 6B 7F 42 02 00 and then…
        …02 cc vv F7 to set MIDI channel (vv: channel-1, 0-15)
        …03 cc vv F7 to set the CC (vv from 0-127)

    The MIDI channel for all other parameters may be set to follow the global channel by using vv=0x41.

ENCODERS
----
The encoders 1-16 have controller numbers 0x20 to 0x2F, the large volume dial has controller number 0x30.
    F0 00 20 6B 7F 42 02 00 01 cc vv F7
        cc is the controller you like to program, 
        vv sets the mode: 0=off, 1=Midi CC, 4=RPN/NRPN.

    Setting the parameters: Send F0 00 20 6B 7F 42 02 00 and then…
        …02 cc vv F7 to set MIDI channel (vv: channel-1, 0-15)
        …03 cc vv F7 to set the CC number that is used.
        …06 cc vv F7 to set the behaviour: 0=Absolute, 1-3=Relative mode 1-3.
            Relative modes 1-3 send a delta. The center values are 64, 0 and 16, respectively.
            I.e. for RELATIVE1, turning a knob (slowly) clockwise yields 65, turning it anticlockwise yields 63. For RELATIVE2 it would be 1 and 127 respectively.

Global MIDI channel – F0 00 20 6B 7F 42 02 00 50 0B nn F7 (MIDI channel-1, 0-15)

STORE COMMAND:
    F0 00 20 6B 7F 42 06 mm F7
RECALL COMMAND:
    F0 00 20 6B 7F 42 05 mm F7

*/

class DeviceBehaviour_Beatstep : virtual public DeviceBehaviourUSBBase, virtual public DividedClockedBehaviour {
    using DividedClockedBehaviour::on_restart;
    
    public:
        #define BEATSTEP_NUM_PATTERNS 16
        bool auto_advance_pattern = false;   // todo: make configurable!
        bool wait_before_changing = true;    // for quantising pattern parameters that otherwise force untimely pattern restarts

        int last_note = -1, current_note = -1;

        uint16_t vid = 0x1c75, pid = 0x0206;
        virtual uint32_t get_packed_id () override { return (this->vid<<16 | this->pid); }

        virtual const char *get_label() override {
            return "BeatStep";
        }
        virtual bool receives_midi_notes() { return true; }

        //FLASHMEM 
        virtual void setup_callbacks() override {
            if (!DeviceBehaviourUSBBase::is_connected()) return;

            this->device->setHandleNoteOn(beatstep_handle_note_on);
            this->device->setHandleNoteOff(beatstep_handle_note_off);
            this->device->setHandleSysEx(beatstep_handle_sysex);
        }

        virtual void receive_note_on(uint8_t channel, uint8_t note, uint8_t velocity) override {
            //Serial_printf("beatstep %s got note on %i on channel %i\n", get_label(), note, channel); Serial_flush();

            this->current_note = note;
            ClockedBehaviour::receive_note_on(channel, note, 127);
        }

        virtual void receive_note_off(uint8_t channel, uint8_t note, uint8_t velocity) override {
            //Serial_printf("beatstep %s got note off %i on channel %i\n", get_label(), note, channel); Serial_flush();

            // update current / remember last played note
            this->last_note = note;
            if (this->current_note==note) 
                current_note = -1;
            ClockedBehaviour::receive_note_off(channel, note, 127);
        }

        bool already_initialised = false;
        //FLASHMEM
        virtual LinkedList<FloatParameter*> *initialise_parameters() override {
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
            // stuff for advancing pattern
            void set_auto_advance_pattern(bool auto_advance_pattern) {
                this->auto_advance_pattern = auto_advance_pattern;
            }
            bool is_auto_advance_pattern() {
                return this->auto_advance_pattern;
            }

            bool auto_fetch = false;
            bool shouldAutoFetch() {
                return this->getAutoFetch();
            }
            bool getAutoFetch() {
                return this->auto_fetch;
            }
            void setAutoFetch(bool state = true) {
                this->auto_fetch = true;
            }
            
            void on_bar(int bar) override {
                DividedClockedBehaviour::on_bar(bar);
                if (this->shouldAutoFetch())
                    this->request_all_sysex_parameters();
            }

            virtual void on_end_phrase_pre_clock(uint32_t phrase) override {
                if (this->device==nullptr) return;

                if (this->auto_advance_pattern) {
                    int phrase_number = (phrase % BEATSTEP_NUM_PATTERNS);
                    this->send_preset_change(phrase_number);
                    //this->on_restart(); 
                    this->set_restart_on_bar(true);
                }

                if (this->wait_before_changing) {
                    // do queued changes
                    if (this->pattern_length_queued) {
                        this->set_sysex_parameter(BEATSTEP_GLOBAL, BEATSTEP_PATTERN_LENGTH, pattern_length);
                        this->pattern_length_queued = false;
                        this->sent_pattern_length = this->pattern_length;
                    }
                    if (this->step_size_queued) {
                        this->set_sysex_parameter(BEATSTEP_GLOBAL, BEATSTEP_STEP_SIZE, pattern_step_size);
                        this->step_size_queued = false;
                        this->sent_step_size = this->pattern_step_size;
                    }
                    if (this->direction_queued) {
                        this->set_sysex_parameter(BEATSTEP_GLOBAL, BEATSTEP_DIRECTION, direction);
                        this->direction_queued = false;
                        this->sent_direction = this->direction;
                    }
                }

                DividedClockedBehaviour::on_end_phrase_pre_clock(phrase);
            }
            void send_preset_change(int phrase_number) {
                if (this->device==nullptr) return;

                Serial.printf(F("beatstep#send_preset_change(%i)\n"), phrase_number % BEATSTEP_NUM_PATTERNS);

                uint8_t data[] = {
                    0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x05, (uint8_t)/*1+*/(phrase_number % BEATSTEP_NUM_PATTERNS), 0xF7
                };
                this->device->sendSysEx(sizeof(data), data, true);

                //this->request_all_sysex_parameters(50);
            }
            void send_preset_save(int preset_number) {
                if (this->device==nullptr) return;

                Serial.printf(F("beatstep#send_preset_save(%i)\n"), preset_number % BEATSTEP_NUM_PATTERNS);

                uint8_t data[] = {
                    0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x06, (uint8_t)/*1+*/(preset_number % BEATSTEP_NUM_PATTERNS), 0xF7
                };
                this->device->sendSysEx(sizeof(data), data, true);

                //this->request_all_sysex_parameters(50);
            }

            // pattern length settings
            int8_t pattern_length = 16;
            int8_t sent_pattern_length = pattern_length;
            bool pattern_length_queued = false;
            void setPatternLength(int8_t length) {
                length = constrain(length,BEATSTEP_PATTERN_LENGTH_MINIMUM,BEATSTEP_PATTERN_LENGTH_MAXIMUM);
                if (!wait_before_changing) {
                    this->set_sysex_parameter(BEATSTEP_GLOBAL, BEATSTEP_PATTERN_LENGTH, length);
                    this->sent_pattern_length = length;
                } else {
                    this->pattern_length_queued = true;
                }
                this->pattern_length = length;
            }
            int8_t getPatternLength() {
                return pattern_length;
            }

            //step size settings
            int8_t pattern_step_size = 0;
            int8_t sent_step_size = pattern_step_size;
            bool step_size_queued = false;
            void setStepSize(int8_t step_size) {
                step_size = constrain(step_size,BEATSTEP_PATTERN_STEP_SIZE_MINIMUM,BEATSTEP_PATTERN_STEP_SIZE_MAXIMUM);
                if (!wait_before_changing) {
                    this->set_sysex_parameter(BEATSTEP_GLOBAL, BEATSTEP_STEP_SIZE, step_size);
                    this->sent_step_size = step_size;
                } else {
                    this->step_size_queued = true;
                }
                this->pattern_step_size = step_size;
            }
            int8_t getStepSize() {
                return pattern_step_size;
            }

            //playback direction settings
            int8_t direction = 0;
            int8_t sent_direction = direction;
            bool direction_queued = false;
            void setDirection(int8_t direction) {
                direction = constrain(direction,0,3);
                if (!wait_before_changing) {
                    this->set_sysex_parameter(BEATSTEP_GLOBAL, BEATSTEP_DIRECTION, direction);
                    sent_direction = direction;
                } else {
                    this->direction_queued = true;
                }
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
                //bool enable_recall = true;
            };
            #define NUM_SYSEX_PARAMETERS 7

            // proof of concept of fetching parameter values from beatstep over sysex

            bool debug_sysex = false;

            sysex_parameter_t sysex_parameters[NUM_SYSEX_PARAMETERS] {
                { BEATSTEP_GLOBAL, BEATSTEP_TRANSPOSE, nullptr, "Transpose"},    // transpose (unused, sacrificial to the gods of missing beatstep data)
                { BEATSTEP_GLOBAL, BEATSTEP_DIRECTION, &this->direction, "Direction", &DeviceBehaviour_Beatstep::setDirection },
                { BEATSTEP_GLOBAL, BEATSTEP_DIRECTION, &this->pattern_step_size, "Step Size", &DeviceBehaviour_Beatstep::setStepSize },
                { BEATSTEP_GLOBAL, BEATSTEP_PATTERN_LENGTH, &this->pattern_length, "Steps", &DeviceBehaviour_Beatstep::setPatternLength },
                { BEATSTEP_GLOBAL, BEATSTEP_SWING, &this->swing, "Swing", &DeviceBehaviour_Beatstep::setSwing },        // swing, 0x32 to 0x4b (ie 50-100%)
                { BEATSTEP_GLOBAL, BEATSTEP_GATE, &this->gate, "Gate", &DeviceBehaviour_Beatstep::setGate },            // gate length, 0x32 to 0x63
                { BEATSTEP_GLOBAL, BEATSTEP_LEGATO, &this->legato, "Legato", &DeviceBehaviour_Beatstep::setLegato }     // legato 0=off, 1=on, 2=reset
            };

            struct BeatstepSysexRequest {
                byte pp;
                byte cc;
            };
            Queue<BeatstepSysexRequest,50> *sysex_request_queue = new Queue<BeatstepSysexRequest,50>();

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
                if (this->shouldAutoFetch() && message==(uint8_t)(midi::Start))
                    this->request_all_sysex_parameters(10);
            }

            // for testing
            void request_all_sysex_parameters() {
                this->request_all_sysex_parameters(3);
            }
            void request_all_sysex_parameters(int delay) {
                if (debug_sysex) Serial.printf("request_all_sysex_parameters with delay %i!!\n", delay);
                /*this->request_sysex_parameter(BEATSTEP_GLOBAL, BEATSTEP_PATTERN_LENGTH, delay);
                this->request_sysex_parameter(BEATSTEP_GLOBAL, BEATSTEP_DIRECTION, 0);            */
                for (unsigned int i = 0 ; i < NUM_SYSEX_PARAMETERS ; i++) {
                    this->request_sysex_parameter(sysex_parameters[i].cc, sysex_parameters[i].pp);
                }
            }

            // actually send a dequeued request, and re-pause the queue
            void process_sysex_request(BeatstepSysexRequest req) {
                const uint8_t data[] = { 0xF0,0x00,0x20,0x6B,0x7F,0x42,0x01,0x00,req.pp,req.cc,0xF7 };
                if (debug_sysex) {
                    Serial.print("Sending Sysex to BeatStep\t[ ");
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
            // put a new request into the sysex queue
            void request_sysex_parameter(byte pp, byte cc, int delay = 0) {
                if (debug_sysex) Serial.printf("Queueing request for %02x, %02x with %ims delay\n", pp, cc, delay);
                this->sysex_request_queue->push(BeatstepSysexRequest {pp, cc}, SYSEX_TIMEOUT, delay);
            }
            // set a beatstep sysex parameter
            void set_sysex_parameter(byte pp, byte cc, byte vv) {
                const uint8_t data[] = { 0xF0,0x00,0x20,0x6B,0x7F,0x42,0x02,0x00,pp,cc,vv,0xF7 };
                if (debug_sysex) {
                    Serial.print("Sending Sysex to BeatStep\t[ ");
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
                    Serial.print(complete ? "complete" : "incomplete");
                    Serial.println();
                }
                #define BROAD_POS 8
                #define SPEC_POS 9
                #define VALUE_POS 10
                if (length < BROAD_POS-1 || !complete) {
                    if (debug_sysex) Serial.printf("handle_sysex received incomplete message with length %i - ignoring!\n", length);
                    return;
                }
                for (unsigned int i = 0 ; i < NUM_SYSEX_PARAMETERS ; i++) {
                    if (sysex_parameters[i].cc==data[BROAD_POS] && sysex_parameters[i].pp==data[SPEC_POS]) {
                        if (sysex_parameters[i].target_variable!=nullptr)
                            *sysex_parameters[i].target_variable = data[VALUE_POS];
                    }
                }

                sysex_request_queue->setPaused(false);
            }

            virtual void setup_saveable_parameters() override {
                if (this->saveable_parameters==nullptr)
                    DeviceBehaviourUltimateBase::setup_saveable_parameters();
                DividedClockedBehaviour::setup_saveable_parameters();

                for (unsigned int i = 0 ; i < NUM_SYSEX_PARAMETERS ; i++) {
                    saveable_parameters->add(new LSaveableParameter<int8_t>(
                        sysex_parameters[i].label,
                        "Sysex",
                        sysex_parameters[i].target_variable,
                        [=](int8_t v) -> void { 
                            void(DeviceBehaviour_Beatstep::*setter_func)(int8_t) = sysex_parameters[i].setter_func;
                            if (setter_func!=nullptr)
                                (this->*setter_func)(v); 
                            else if (sysex_parameters[i].target_variable!=nullptr)
                                *sysex_parameters[i].target_variable = v;
                        }
                    ));
                }                    
            }
        #endif

        #ifdef ENABLE_SCREEN
            FLASHMEM
            virtual LinkedList<MenuItem*> *make_menu_items() override;
        #endif

        // UNTESTED!!! 
        void configure_cntrl_for_mock_sequencer() {
            Serial.println("starting configure_cntrl_for_mock_sequencer...");
            for (int preset = 0 ; preset < 16 ; preset++) {
                this->send_preset_change(preset);
                delay(10);
                for (int pad = 0 ; pad < NUM_ENCODERS ; pad++) {
                    Serial.printf("Processing pad/encoder %i/%i..\n", pad, NUM_ENCODERS+1);
                    this->set_sysex_parameter(SYSEX_CHANNEL,    BEATSTEP_SYSEX_BASE_ENCODER + pad, 0x41);
                    this->set_sysex_parameter(SYSEX_CHANNEL,    BEATSTEP_SYSEX_BASE_PADS    + pad, 0x41);
                    this->set_sysex_parameter(SYSEX_BEHAVIOUR,  BEATSTEP_SYSEX_BASE_PADS    + pad, SYSEX_PAD_MODE_SILENT_CC_SWITCH);
                    delay(10);
                }
                this->send_preset_save(preset);
                delay(10);
            }
            Serial.println("finished configure_cntrl_for_mock_sequencer");
        }

};
extern DeviceBehaviour_Beatstep *behaviour_beatstep;

#ifdef ENABLE_BEATSTEP_2
    class DeviceBehaviour_Beatstep_2 : virtual public DeviceBehaviour_Beatstep {
        public:

        virtual const char *get_label() override {
            return "BeatStep#2";
        }

        //FLASHMEM 
        virtual void setup_callbacks() override {
            if (!DeviceBehaviourUSBBase::is_connected()) return;

            this->device->setHandleNoteOn(beatstep_2_handle_note_on);
            this->device->setHandleNoteOff(beatstep_2_handle_note_off);
            this->device->setHandleSysEx(beatstep_2_handle_sysex);
        }
    };
    extern DeviceBehaviour_Beatstep_2 *behaviour_beatstep_2;
#endif


//#include "menuitems_object_multitoggle.h"

/*class BeatstepSysexOptionToggle : public MultiToggleItemBase {
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
};*/

#endif



#endif