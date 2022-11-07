#ifndef BEHAVIOUR_BAMBLE__INCLUDED
#define BEHAVIOUR_BAMBLE__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviours/behaviour_base.h"
#include "behaviours/behaviour_clocked.h"
#include "project.h"
#include "clock.h"

#include "multi_usb_handlers.h"

//extern MIDITrack mpk49_loop_track;
//class MIDITrack;

#ifdef ENABLE_BAMBLE_INPUT
    void bamble_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
    void bamble_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
    void bamble_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
#endif

#define CC_DEMO_MODE 19
#define CC_FILLS_MODE 28
#define CC_EUCLIDIAN_DENSITY 114
#define CC_EUCLIDIAN_SET_MINIMUM_PATTERN 25	
#define CC_EUCLIDIAN_SET_MAXIMUM_PATTERN 26	

#include "midi/Drums.h"

/// these for mappable parameters
#include "parameters/MIDICCParameter.h"
/*
class BamblePlaybackModeParameter : public MIDICCParameter {
    public:
        BamblePlaybackModeParameter (char *label, DeviceBehaviourUltimateBase *target) 
            : MIDICCParameter(label, target, (byte)CC_DEMO_MODE, (byte)10) {
        }

        virtual const char* parseFormattedDataType(byte value) {
            switch (value) {
                case 0: return "None";
                case 1: return "Eucl";
                case 2: return "Muta";
                case 3: return "Demo";
                case 4: return "Rand";
                default:
                    return "????";
            }
        };
};

class BambleFillsParameter : public MIDICCParameter {
    public:
        BambleFillsParameter(char *label, DeviceBehaviourUltimateBase *target) 
            : MIDICCParameter(label, target, (byte)CC_FILLS_MODE, (byte)10) {
        }

        virtual const char* parseFormattedDataType(byte value) {
            switch (value) {
                case 0: return "Off";
                case 1 ... 127: return "On";
                default:
                    return "???";
            }
        };
};*/

#include "midi/Drums.h"

struct bamble_pattern {
    uint8_t cc_number;
    const char *label;
    bool    current_state = true;
};

class DeviceBehaviour_Bamble : virtual public DeviceBehaviourUSBBase, public DividedClockedBehaviour {
    public:
        using DeviceBehaviourUltimateBase::receive_control_change;
        using DeviceBehaviourUltimateBase::receive_note_on;
        using DeviceBehaviourUltimateBase::receive_note_off;
        using DeviceBehaviourUltimateBase::debug;

        virtual bool has_output() { return true; }
        //virtual bool has_input() { return true; }

        uint16_t vid = 0x2886, pid = 0x800B;
        virtual uint32_t get_packed_id() override { return (this->vid<<16 | this->pid); }

        virtual const char *get_label() override {
            return (const char*)"Bambleweeny57";
        }

        /*** for tracking/setting Bambleweeny options */

        int8_t demo_mode = 0;
        bool fills_mode = 0;
        float density = 64;
        byte minimum_pattern = 0;
        byte maximum_pattern = 16;

        int8_t getDemoMode() {
            return demo_mode;
        }
        void setDemoMode(int8_t mode) {
            this->demo_mode = mode;
            this->sendControlChange(CC_DEMO_MODE, mode, 10);
        }
        bool getFillsMode() {
            return fills_mode;
        }
        void setFillsMode(bool mode) {
            this->fills_mode = mode;
            this->sendControlChange(CC_FILLS_MODE, mode, 10);
        }
        void setDensity(float density) {
            //Serial.printf("setDensity %3.3f\n", density);
            this->density = density;
            this->sendControlChange(CC_EUCLIDIAN_DENSITY, map(density, 0.0, 1.0, 0, 127), 10);
        }
        float getDensity() {
            return this->density;
        }
        void setMinimumPattern(int8_t v) {
            this->minimum_pattern = v;
            this->sendControlChange(CC_EUCLIDIAN_SET_MINIMUM_PATTERN, v, 10);
        }
        int8_t getMinimumPattern() {
            return this->minimum_pattern;
        }
        void setMaximumPattern(int8_t v) {
            this->maximum_pattern = v;
            this->sendControlChange(CC_EUCLIDIAN_SET_MAXIMUM_PATTERN, v, 10);
        }
        int8_t getMaximumPattern() {
            return this->maximum_pattern;
        }

        bamble_pattern patterns[16] = {
            //CC, label
            { 32, "Kick" },
            { 33, "Sidestick" },
            { 34, "Clap" },
            { 35, "Snare" },
            { 36, "Crash 1" },
            { 37, "Tamb" },
            { 38, "HiTom" },
            { 39, "LoTom" },
            { 40, "Pedal" },
            { 41, "OHH" },
            { 42, "CHH" },
            { 43, "Crash 2" },
            { 44, "Splash" },
            { 45, "Vibra" },
            { 46, "Ride Bell" },
            { 47, "Ride Cym" },
            /*{ 48, "BassCh4" },
            { 49, "BitsCh3" },
            { 50, "Pitch1Ch1" },
            { 51, "Pitch2Ch2" }*/
        };

        void setPatternEnabled(int number, bool state) {
            this->patterns[number].current_state = state;
            this->sendControlChange(patterns[number].cc_number, state ? 127:0, 10);
        }

        //FLASHMEM 
        virtual void setup_callbacks() override {
            //behaviour_apcmini = this;
            if (this->device==nullptr) return;
            #ifdef ENABLE_BAMBLE_INPUT
                this->device->setHandleControlChange(bamble_control_change);
                this->device->setHandleNoteOn(bamble_note_on);
                this->device->setHandleNoteOff(bamble_note_off);
            #endif
        };

        source_id_t source_ids[5] = { -1, -1, -1, -1, -1 };
        //FLASHMEM
        virtual void self_register_midi_matrix_targets(MIDIMatrixManager *midi_matrix_manager) {
            // register multiple inputs / outputs

            midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"USB : Bamble : ch 1", this, 1));
            midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"USB : Bamble : ch 2", this, 2));
            midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"USB : Bamble : ch 3", this, 3));
            midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"USB : Bamble : ch 4", this, 4));
            midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"USB : Bamble : drums",this, 10));
        }

        /// these for mappable parameters
        /*FLASHMEM virtual LinkedList<DoubleParameter*> *initialise_parameters() override {
            Serial.printf(F("DeviceBehaviour_Bamble#initialise_parameters()..."));
            static bool already_initialised = false;
            if (already_initialised)
                return this->parameters;

            Serial.println(F("\tcalling DeviceBehaviourUSBBase::initialise_parameters()")); 
            DeviceBehaviourUSBBase::initialise_parameters();
            Serial.println(F("\tcalling ClockedBehaviour::initialise_parameters()"));
            DividedClockedBehaviour::initialise_parameters();

            //parameters->add(new BamblePlaybackModeParameter((char*)"Playback mode", this));
            //parameters->add(new BambleFillsParameter(       (char*)"Fills",         this));
            //parameters->add(new MIDICCParameter((char*)"Density", this, 114, 10));
            //parameters->add(new MIDICCParameter((char*)"Density", this, 114, 10));

            return parameters;
        }*/

        #ifdef ENABLE_BAMBLE_INPUT
            virtual void self_register_midi_matrix_sources(MIDIMatrixManager *midi_matrix_manager) {
                this->source_ids[0] = midi_matrix_manager->register_source(this, "bamble_input_ch1");
                this->source_ids[1] = midi_matrix_manager->register_source(this, "bamble_input_ch2");
                this->source_ids[2] = midi_matrix_manager->register_source(this, "bamble_input_ch3");
                this->source_ids[3] = midi_matrix_manager->register_source(this, "bamble_input_ch4");
                this->source_ids[4] = midi_matrix_manager->register_source(this, "bamble_input_ch16");
            }

            // special version that uses source_ids array based on incoming channel to route
            void receive_note_on(uint8_t channel, uint8_t note, uint8_t velocity) override {
                if (channel==16) channel = 5;    // remap midimuso drums to the last channel in the range
                midi_matrix_manager->processNoteOn(this->source_ids[channel-1], note, velocity); //, channel);
            }

            // called when a note_off message is received from the device
            void receive_note_off(uint8_t channel, uint8_t note, uint8_t velocity) override {
                if (channel==16) channel = 5;    // remap midimuso drums to the last channel in the range
                midi_matrix_manager->processNoteOff(this->source_ids[channel-1], note, velocity); //, channel);
            }
        #endif

        //FLASHMEM
        virtual void init() override {
            if (!DeviceBehaviourUSBBase::is_connected()) return;
            this->started = false;

            #ifdef ENABLE_BAMBLE_INPUT
                // enable midi echo - crashes it ?
                DeviceBehaviourUSBBase::sendControlChange(21, 127, 10);
            #endif

            // disable playing from own BPM (ie require clock to play)
            DeviceBehaviourUSBBase::sendControlChange(16, 0, 10);
            //this->setDemoMode(1);       // initialise in playing/non-mutating state
            this->setDemoMode(2);       // initialise in playing+mutating state
            this->setFillsMode(true);   // initialise in fills mode
            this->setDensity(0.5);      // initialise density to 50%
            this->setMinimumPattern(TRIGGER_KICK);
            this->setMaximumPattern(TRIGGER_RIDE_CYM);

            DeviceBehaviourUSBBase::sendControlChange(7, 0, 10); // CC_EUCLIDIAN_MUTATE_DENSITY	automatically mutate density on/off

            // this should disable euclidian pulses on the pitch outputs ch1 + ch2
            DeviceBehaviourUSBBase::sendControlChange(78, 0, 10);
            DeviceBehaviourUSBBase::sendControlChange(79, 0, 10);
            DeviceBehaviourUSBBase::sendControlChange(50, 0, 10);
            DeviceBehaviourUSBBase::sendControlChange(51, 0, 10);

            // sustain to max for the envelope outputs
            DeviceBehaviourUSBBase::sendControlChange(67, 127, 10);
            DeviceBehaviourUSBBase::sendControlChange(67, 127, 11);
            DeviceBehaviourUSBBase::sendControlChange(75, 127, 10);
            DeviceBehaviourUSBBase::sendControlChange(75, 127, 11);
            DeviceBehaviourUSBBase::sendControlChange(83, 127, 10);
            DeviceBehaviourUSBBase::sendControlChange(83, 127, 11);
            DeviceBehaviourUSBBase::sendControlChange(91, 127, 10);
            DeviceBehaviourUSBBase::sendControlChange(91, 127, 11);
            DeviceBehaviourUSBBase::sendControlChange(99, 127, 10);
            DeviceBehaviourUSBBase::sendControlChange(99, 127, 11);
        }

        virtual void save_sequence_add_lines(LinkedList<String> *lines) override {
            DividedClockedBehaviour::save_project_add_lines(lines);
            lines->add(String(F("euclidian_mode=")) + String(this->getDemoMode()));
            lines->add(String(F("fills_mode="))     + String(this->getFillsMode()));
            lines->add(String(F("density="))        + String(this->getDensity()));
            const uint8_t size = sizeof(this->patterns)/sizeof(bamble_pattern);
            for (uint8_t i = 0 ; i < size ; i++) {
                lines->add(
                    String(F("pattern_enable_")) + String(i) + String('=') + 
                    String(this->patterns[i].current_state?F("enabled"):F("disabled"))
                );
            }
        }

        virtual bool load_parse_key_value(String key, String value) override {
            if (key.equals(F("euclidian_mode"))) {
                this->setDemoMode((byte)    value.toInt());
                return true;
            } else if (key.equals(F("fills_mode"))) {
                this->setFillsMode((bool)   value.toInt());
                return true;
            } else if (key.equals(F("density"))) {
                this->setDensity((float)    value.toFloat());
                return true;
            } else if (key.startsWith(F("pattern_enable_"))) {
                int number = key.replace(F("pattern_enable_"),"").toInt();
                this->setPatternEnabled(number, value.equals(F("enabled")));
                return true;
            } else if (DividedClockedBehaviour::load_parse_key_value(key, value)) {
                return true;
            }
            return false;
        }

        #ifdef ENABLE_SCREEN
            // FLASHMEM // virtual LinkedList<MenuItem*>* DeviceBehaviour_Bamble::make_menu_items() causes a section type conflict with virtual void DeviceBehaviour_Bamble::setup_callbacks()
            FLASHMEM
            LinkedList<MenuItem*> *make_menu_items();
        #endif
};

extern DeviceBehaviour_Bamble *behaviour_bamble;

#endif