#ifndef BEHAVIOUR_BAMBLE__INCLUDED
#define BEHAVIOUR_BAMBLE__INCLUDED

#include <Arduino.h>

#include "Config.h"

#ifdef ENABLE_BAMBLE

#include "behaviours/behaviour_base.h"
#include "behaviours/behaviour_clocked.h"
#include "project.h"
#include "clock.h"

#include "usb/multi_usb_handlers.h"

//extern MIDITrack mpk49_loop_track;
//class MIDITrack;

#ifdef ENABLE_BAMBLE_INPUT
    void bamble_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
    void bamble_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
    void bamble_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
#endif

#define CC_EUCLIDIAN_MUTATE_DENSITY 7
#define CC_DEMO_MODE 19
#define CC_EUCLIDIAN_DENSITY 114
#define CC_EUCLIDIAN_SET_MINIMUM_PATTERN 25	
#define CC_EUCLIDIAN_SET_MAXIMUM_PATTERN 26	
#define CC_EUCLIDIAN_SEED_MODIFIER 22
#define CC_EUCLIDIAN_SEED_MODIFIER_2 23
#define CC_EUCLIDIAN_RESET_BEFORE_MUTATE 24
#define CC_EUCLIDIAN_SEED_USE_PHRASE 27
#define CC_EUCLIDIAN_FILLS 28

#include "midi/Drums.h"

#include "SaveableParameters.h"

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
            : MIDICCParameter(label, target, (byte)CC_EUCLIDIAN_FILLS, (byte)10) {
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

        virtual bool transmits_midi_notes() { return true; }
        //virtual bool receives_midi_notes() { return true; }

        uint16_t vid = 0x2886, pid = 0x800B;
        virtual uint32_t get_packed_id() override { return (this->vid<<16 | this->pid); }

        virtual const char *get_label() override {
            return (const char*)"Bambleweeny57";
        }

        /*** for tracking/setting Bambleweeny options */

        float density = 64;
        int8_t demo_mode = 0;
        bool fills_mode = 0;
        int8_t minimum_pattern = 0;
        int8_t maximum_pattern = 16;
        uint16_t euclidian_seed_modifier = 0;
        bool euclidian_reset_before_mutate = true;
        bool euclidian_seed_use_phrase = true;

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
            this->sendControlChange(CC_EUCLIDIAN_FILLS, mode, 10);
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

        void setEuclidianSeedModifier(uint16_t v) {
            this->euclidian_seed_modifier = v;
            this->sendControlChange(CC_EUCLIDIAN_SEED_MODIFIER, v >> 8, 10);
            this->sendControlChange(CC_EUCLIDIAN_SEED_MODIFIER_2, 0x00FF & v, 10);
        }
        uint16_t getEuclidianSeedModifier() {
            return this->euclidian_seed_modifier;
        }
        void setEuclidianResetBeforeMutate(bool v) {
            this->euclidian_reset_before_mutate = v;
            this->sendControlChange(CC_EUCLIDIAN_RESET_BEFORE_MUTATE, v?127:0, 10);
        }
        bool getEuclidianResetBeforeMutate() {
            return this->euclidian_reset_before_mutate;
        }
        void setEuclidianSeedUsePhrase(bool v) {
            this->euclidian_seed_use_phrase = v;
            this->sendControlChange(CC_EUCLIDIAN_SEED_USE_PHRASE, v?127:0, 10);
        }
        bool getEuclidianSeedUsePhrase() {
            return this->euclidian_seed_use_phrase;
        }
        void setRandomSeed() {
            this->setEuclidianSeedModifier(random(16384));
        }

        // Trigger/LFO settings: 0->19 = trigger #, 20 = off, 32->51 = trigger #+loop, 64->83 = trigger #+invert, 96->115 = trigger #+loop+invert
        struct envelope_trigger_on_t {
            const int cc;
            const int channel = 10;
            int pattern_number;
            bool loop = false;
            bool invert = false;
        };
        #define LOOP_MASK 32
        #define INVERT_MASK 64
        #define NUM_ENVELOPES 9
        envelope_trigger_on_t envelope_trigger_on[NUM_ENVELOPES] = {
            //cc, channel, pattern
            { 71,   10,   TRIGGER_CRASH_2       },
            { 79,   10,   TRIGGER_SPLASH        },
            { 87,   10,   TRIGGER_VIBRA         },
            { 95,   10,   TRIGGER_RIDE_BELL     },
            { 103,  10,   TRIGGER_RIDE_CYM      },
            { 71,   11,   TRIGGER_PITCH_1_CH1   },
            { 79,   11,   TRIGGER_PITCH_1_CH1   },
            { 87,   11,   TRIGGER_PITCH_2_CH2   },
            { 95,   11,   TRIGGER_PITCH_2_CH2   }
        };
        bool is_valid_envelope(int env) {
            return env>=0 && env < NUM_ENVELOPES;
        }
        // actually send update
        void send_envelope_trigger_on_value(int env) {
            if (!is_valid_envelope(env)) return;
            //int channel = env >= 5 ? 11 : 10;
            int v = envelope_trigger_on[env].pattern_number | (LOOP_MASK*envelope_trigger_on[env].loop) | (INVERT_MASK*envelope_trigger_on[env].invert);
            this->sendControlChange((byte)envelope_trigger_on[env].cc, v, envelope_trigger_on[env].channel);
        }
        void set_envelope_trigger_on(int env, int pattern_number) {
            if (!is_valid_envelope(env)) return;
            this->envelope_trigger_on[env].pattern_number = pattern_number;
            this->send_envelope_trigger_on_value(env);
        }
        int get_envelope_trigger_on(int env) {
            if (!is_valid_envelope(env)) return false;
            return this->envelope_trigger_on[env].pattern_number;
        }
        bool is_envelope_trigger_loop(int env) {
            if (!is_valid_envelope(env)) return false;
            return this->envelope_trigger_on[env].loop;
        }
        void set_envelope_trigger_loop(int env, bool v) {
            if (!is_valid_envelope(env)) return;
            this->envelope_trigger_on[env].loop = v;
            this->send_envelope_trigger_on_value(env);
        }
        bool is_envelope_trigger_invert(int env) {
            if (!is_valid_envelope(env)) return false;
            return this->envelope_trigger_on[env].invert;
        }
        void set_envelope_trigger_invert(int env, bool v) {
            if (!is_valid_envelope(env)) return;
            this->envelope_trigger_on[env].invert = v;
            this->send_envelope_trigger_on_value(env);
        }

        bamble_pattern patterns[20] = {
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
            { 48, "BassCh4" },
            { 49, "BitsCh3" },
            { 50, "Pitch1Ch1" },
            { 51, "Pitch2Ch2" }
        };
        #define NUM_EUCLIDIAN_PATTERNS (int)(sizeof(patterns) / sizeof(bamble_pattern))

        // update internal state and send message to bamble to match
        void setPatternEnabled(int number, bool state) {
            this->patterns[number].current_state = state;
            this->sendControlChange(patterns[number].cc_number, state ? 127 : 0, 10);
            Debug_printf(
                F("%s#setPatternEnabled(%i, %s) sending ControlChange for %s\n"), 
                this->get_label(), 
                number, 
                state?"On":"Off", 
                this->patterns[number].label
            );
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

        //FLASHMEM // error: virtual LinkedList<MenuItem*>* DeviceBehaviour_Bamble::make_menu_items() causes a section type conflict with virtual void DeviceBehaviour_Bamble::add_adhsr_parameters(const char*, int, int)
        //FLASHMEM
        void add_adhsr_parameters(const char *prefix, int start, int channel) {
            parameters->add(new MIDICCParameter<>((String(prefix) + String(F(" Attack"))).c_str(),  this, start++, channel));
            parameters->add(new MIDICCParameter<>((String(prefix) + String(F(" Hold"))).c_str(),    this, start++, channel));
            parameters->add(new MIDICCParameter<>((String(prefix) + String(F(" Decay"))).c_str(),   this, start++, channel));
            parameters->add(new MIDICCParameter<>((String(prefix) + String(F(" Sustain"))).c_str(), this, start++, channel));
            parameters->add(new MIDICCParameter<>((String(prefix) + String(F(" Release"))).c_str(), this, start++, channel));
            parameters->add(new MIDICCParameter<>((String(prefix) + String(F(" HD Vib"))).c_str(),  this, start++, channel));
            parameters->add(new MIDICCParameter<>((String(prefix) + String(F(" SR Vib"))).c_str(),  this, start++, channel));
        }

        /// these for mappable parameters
        //FLASHMEM 
        bool already_initialised = false;
        virtual LinkedList<FloatParameter*> *initialise_parameters() override {
            Debug_printf(F("DeviceBehaviour_Bamble#initialise_parameters()..."));
            //static bool already_initialised = false;
            if (already_initialised && this->parameters!=nullptr)
                return this->parameters;

            Debug_println(F("\tcalling DeviceBehaviourUSBBase::initialise_parameters()")); 
            DeviceBehaviourUSBBase::initialise_parameters();
            Debug_println(F("\tcalling ClockedBehaviour::initialise_parameters()"));
            DividedClockedBehaviour::initialise_parameters();

            this->add_adhsr_parameters("Env PT 1", 64, 11);
            this->add_adhsr_parameters("Env PT 2", 72, 11);
            this->add_adhsr_parameters("Env PT 3", 80, 11);
            this->add_adhsr_parameters("Env PT 4", 88, 11);

            //parameters->add(new BamblePlaybackModeParameter((char*)"Playback mode", this));
            //parameters->add(new BambleFillsParameter(       (char*)"Fills",         this));
            //parameters->add(new MIDICCParameter((char*)"Density", this, 114, 10));
            //parameters->add(new MIDICCParameter((char*)"Density", this, 114, 10));

            return parameters;
        }

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

            // disable playing from own BPM (ie, require clock to play)
            DeviceBehaviourUSBBase::sendControlChange(16, 0, 10);

            //this->setDemoMode(1);       // initialise in playing/non-mutating state
            this->setDemoMode(2);       // initialise in playing+mutating state
            this->setFillsMode(true);   // initialise in fills mode
            this->setDensity(0.5);      // initialise density to 50%
            this->setMinimumPattern(TRIGGER_KICK);
            this->setMaximumPattern(TRIGGER_PITCH_2_CH2);
            this->setEuclidianSeedModifier(0);          // 22, 23 CC_EUCLIDIAN_SEED_MODIFIER_2
            this->setEuclidianResetBeforeMutate(true);  // 24 CC_EUCLIDIAN_RESET_BEFORE_MUTATE
            this->setEuclidianSeedUsePhrase(true);      //  `27` | `CC_EUCLIDIAN_SEED_USE_PHRASE`

            for (unsigned int i = 0 ; i < NUM_EUCLIDIAN_PATTERNS ; i++) {
                this->setPatternEnabled(i, true);
            }

            DeviceBehaviourUSBBase::sendControlChange(CC_EUCLIDIAN_MUTATE_DENSITY, 0, 10); // CC_EUCLIDIAN_MUTATE_DENSITY	automatically mutate density on/off

            // send envelope trigger-on values to match initial setup
            for (unsigned int i = 0 ; i < NUM_ENVELOPES ; i++) {
                this->send_envelope_trigger_on_value(i);
            }

            // this should disable euclidian pulses on the pitch outputs ch1 + ch2
            /*DeviceBehaviourUSBBase::sendControlChange(78, 0, 10);
            DeviceBehaviourUSBBase::sendControlChange(79, 0, 10);
            DeviceBehaviourUSBBase::sendControlChange(50, 0, 10);
            DeviceBehaviourUSBBase::sendControlChange(51, 0, 10);*/

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

        /*#define NUM_SAVEABLE_PARAMETERS 8
        SaveableParameterBase *saveable_parameters[NUM_SAVEABLE_PARAMETERS] = {
            new BambleSaveableParameter<int8_t> ("euclidian_mode", this, &DeviceBehaviour_Bamble::setDemoMode,   &DeviceBehaviour_Bamble::getDemoMode),
            new BambleSaveableParameter<bool>   ("fills_mode",     this, &DeviceBehaviour_Bamble::setFillsMode,  &DeviceBehaviour_Bamble::getFillsMode),
            new BambleSaveableParameter<float>  ("density",        this, &DeviceBehaviour_Bamble::setDensity,    &DeviceBehaviour_Bamble::getDensity),
            new BambleSaveableParameter<int8_t> ("mutate_low",     this, &DeviceBehaviour_Bamble::setMinimumPattern,    &DeviceBehaviour_Bamble::getMinimumPattern),
            new BambleSaveableParameter<int8_t> ("mutate_high",    this, &DeviceBehaviour_Bamble::setMaximumPattern,    &DeviceBehaviour_Bamble::getMaximumPattern),
            new BambleSaveableParameter<uint16_t>("mutate_seed_modifier", this,    &DeviceBehaviour_Bamble::setEuclidianSeedModifier,    &DeviceBehaviour_Bamble::getEuclidianSeedModifier), // aka 'Seed Modifier'
            new BambleSaveableParameter<bool>   ("reset_before_mutate",  this,   &DeviceBehaviour_Bamble::setEuclidianResetBeforeMutate,    &DeviceBehaviour_Bamble::getEuclidianResetBeforeMutate), // aka 'auto-reset'
            new BambleSaveableParameter<bool>   ("seed_use_phrase", this,   &DeviceBehaviour_Bamble::setEuclidianSeedUsePhrase,    &DeviceBehaviour_Bamble::getEuclidianSeedUsePhrase), // aka 'Use Phrase'
        };*/

        class SaveableParameterPatternEnabled : public SaveableParameterBase {
            public:
                DeviceBehaviour_Bamble *target = nullptr;
                int index;
                const String prefix = String("pattern_enable_") + String(index);
                SaveableParameterPatternEnabled(DeviceBehaviour_Bamble *target, int index) 
                    : SaveableParameterBase(target->patterns[index].label, index < 16 ? "Patterns 1" : "Patterns 2"), 
                    target(target), 
                    index(index) {}

                virtual String get_line() override { 
                    return prefix + String('=') + String(target->patterns[index].current_state?"true":"false");
                }
                virtual bool parse_key_value(String key, String value) override {
                    if (key.startsWith(prefix)) {
                        target->setPatternEnabled(index, value.equals("true") || value.equals("enabled"));
                        return true;
                    }
                    return false;
                }
        };

        virtual void setup_saveable_parameters() {
            DeviceBehaviourUltimateBase::setup_saveable_parameters();
            DividedClockedBehaviour::setup_saveable_parameters();

            this->saveable_parameters->add(new SaveableParameter<DeviceBehaviour_Bamble, int8_t> ("euclidian_mode", "Euclidian", this, &this->demo_mode, nullptr, nullptr, &DeviceBehaviour_Bamble::setDemoMode,   &DeviceBehaviour_Bamble::getDemoMode));
            this->saveable_parameters->add(new SaveableParameter<DeviceBehaviour_Bamble, bool>   ("fills_mode",     "Euclidian", this, &this->fills_mode, nullptr, nullptr, &DeviceBehaviour_Bamble::setFillsMode,  &DeviceBehaviour_Bamble::getFillsMode));
            this->saveable_parameters->add(new SaveableParameter<DeviceBehaviour_Bamble, float>  ("density",        "Euclidian", this, &this->density, nullptr, nullptr, &DeviceBehaviour_Bamble::setDensity,    &DeviceBehaviour_Bamble::getDensity));
            this->saveable_parameters->add(new SaveableParameter<DeviceBehaviour_Bamble, int8_t> ("mutate_low",     "Mutate",    this, &this->minimum_pattern, nullptr, nullptr, &DeviceBehaviour_Bamble::setMinimumPattern,    &DeviceBehaviour_Bamble::getMinimumPattern));
            this->saveable_parameters->add(new SaveableParameter<DeviceBehaviour_Bamble, int8_t> ("mutate_high",    "Mutate",    this, &this->maximum_pattern, nullptr, nullptr, &DeviceBehaviour_Bamble::setMaximumPattern,    &DeviceBehaviour_Bamble::getMaximumPattern));
            this->saveable_parameters->add(new SaveableParameter<DeviceBehaviour_Bamble, uint16_t>("mutate_seed_modifier", "Mutate",    this, &this->euclidian_seed_modifier, nullptr, nullptr, &DeviceBehaviour_Bamble::setEuclidianSeedModifier,    &DeviceBehaviour_Bamble::getEuclidianSeedModifier)); // aka 'Seed Modifier' / 'Seed bank'
            this->saveable_parameters->add(new SaveableParameter<DeviceBehaviour_Bamble, bool>   ("reset_before_mutate", "Mutate",    this, &this->euclidian_reset_before_mutate, nullptr, nullptr, &DeviceBehaviour_Bamble::setEuclidianResetBeforeMutate,    &DeviceBehaviour_Bamble::getEuclidianResetBeforeMutate)); // aka 'auto-reset'
            this->saveable_parameters->add(new SaveableParameter<DeviceBehaviour_Bamble, bool>   ("seed_use_phrase","Mutate",    this, &this->euclidian_seed_use_phrase, nullptr, nullptr, &DeviceBehaviour_Bamble::setEuclidianSeedUsePhrase,    &DeviceBehaviour_Bamble::getEuclidianSeedUsePhrase)); // aka 'Use Phrase')

            const unsigned int size = NUM_EUCLIDIAN_PATTERNS;
            for (unsigned int i = 0 ; i < size ; i++) {
                this->saveable_parameters->add(new SaveableParameterPatternEnabled(this, i));
            }
        }

        /*virtual void save_sequence_add_lines(LinkedList<String> *lines) override {
            DeviceBehaviourUltimateBase::save_sequence_add_lines(lines);
            //DividedClockedBehaviour::save_sequence_add_lines(lines);

            // save out the 'saveable parameters'
            //this->save_sequence_add_lines_saveable_parameters(lines);

            const int size = NUM_EUCLIDIAN_PATTERNS;
            for (int i = 0 ; i < size ; i++) {
                lines->add(
                    String(F("pattern_enable_")) + String(i) + String('=') + 
                    String(this->patterns[i].current_state?F("enabled"):F("disabled"))
                );
            }
        }*/

        /*virtual bool load_parse_key_value(String key, String value) override {
            if (key.startsWith(F("pattern_enable_"))) {
                int number = key.replace(F("pattern_enable_"),"").toInt();
                this->setPatternEnabled(number, value.equals(F("enabled")));
                return true;
            } else if (key.startsWith(F("mutate_low"))) {
                this->setMinimumPattern(value.toInt());
                return true;
            } else if (key.startsWith(F("mutate_high"))) {
                this->setMaximumPattern(value.toInt());
                return true;
            } else if (key.startsWith(F("mutate_seed_modifier"))) {
                this->setEuclidianSeedModifier(value.toInt());
                return true;
            } else if (key.startsWith(F("reset_before_mutate"))) {
                this->setEuclidianResetBeforeMutate(value.equals("true"));
                return true;
            } else if (key.startsWith(F("seed_use_phrase"))) {
                this->setEuclidianSeedUsePhrase(value.equals("true"));
                return true;
            } else if (DividedClockedBehaviour::load_parse_key_value(key, value)) {
                return true;
            } else if (DeviceBehaviourUltimateBase::load_parse_key_value(key, value)) {
                return true;
            }
            return false;
        }*/

        #ifdef ENABLE_SCREEN
            // FLASHMEM // virtual LinkedList<MenuItem*>* DeviceBehaviour_Bamble::make_menu_items() causes a section type conflict with virtual void DeviceBehaviour_Bamble::setup_callbacks()
            FLASHMEM
            virtual LinkedList<MenuItem*> *make_menu_items() override;
        #endif
};

extern DeviceBehaviour_Bamble *behaviour_bamble;

#endif
#endif