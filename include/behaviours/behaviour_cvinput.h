#pragma once

//#define DEBUG_VELOCITY    // enable velocity control so we can tell what's going on

#include "Config.h"

#include "bpm.h"

#include "midi_helpers.h"

#include "scales.h"

#include "behaviour_base.h"
#include "parameter_inputs/VoltageParameterInput.h"

#include "ParameterManager.h"
extern ParameterManager *parameter_manager;

#ifdef ENABLE_SCREEN
    //class DeviceBehaviour_CVInput;
    template<class TargetClass>
    class ParameterInputSelectorControl;
#endif

class DeviceBehaviour_CVInput;

#ifdef ENABLE_SCALES
    // todo: move this to Parameter library, or midihelpers scale library..?
    template<class TargetClass=DeviceBehaviour_CVInput, class DataType=CHORD::Type>
    class ChordTypeParameter : /*virtual*/ public DataParameter<TargetClass, DataType> {
        public:
            ChordTypeParameter(const char *label, TargetClass *target, void(TargetClass::*setter_func)(DataType), DataType(TargetClass::*getter_func)()) 
            : DataParameter<TargetClass, DataType>(label, target, setter_func, getter_func, 0, NUMBER_CHORDS-1) {}

            /*virtual DataType normalToData(float value) override {
                value = this->constrainNormal(value);
                //DataType data = this->minimumDataValue + (value * (float)(this->maximumDataValue - this->minimumDataValue));
                DataType data = value * (float)NUMBER_CHORDS;
                if (this->debug && value>=0.0f) Serial.printf(" => %i\n", data);
                return data;
            }
            virtual float dataToNormal(DataType value) override {
                //if (this->debug) Serial.printf(F("dataToNormal(%i) "), value);
                //float normal = (float)(value - minimumDataValue) / (float)(maximumDataValue - minimumDataValue);
                //if (this->debug) Serial.printf(F(" => %f\n"), normal);
                float normal = value / (float)NUMBER_CHORDS;
                return normal;
                // eg   min = 0, max = 100, actual = 50 ->          ( 50 - 0 ) / (100-0)            = 0.5
                //      min = 0, max = 100, actual = 75 ->          ( 75 - 0 ) / (100-0)            = 0.75
                //      min = -100, max = 100, actual = 0 ->        (0 - -100) / (100--100)         = 0.5
                //      min = -100, max = 100, actual = -100 - >    (-100 - -100) / (100 - -100)    = 0
            }*/

            #ifdef ENABLE_SCREEN
                virtual const char* parseFormattedDataType(DataType value) {
                    static char fmt[MENU_C_MAX] = "              ";
                    snprintf(fmt, MENU_C_MAX, chords[value].label);
                    return fmt;
                    //return "??";
                };
            #endif
    };
#endif

#include "chord_player.h"
#include "pitch_trigger_source.h"
#include "functional-vlpp.h"

#ifndef MAX_LABEL_LENGTH
    #define MAX_LABEL_LENGTH 40
#endif

class DeviceBehaviour_CVInput : /* virtual */ public DeviceBehaviourUltimateBase {  // making virtual increases code usage by about 500 bytes!
    public:
        #ifdef ENABLE_SCALES
            ChordPlayer chord_player = ChordPlayer(
                [=](int8_t channel, int8_t note, int8_t velocity) -> void { this->receive_note_on(channel, note, velocity); },
                [=](int8_t channel, int8_t note, int8_t velocity) -> void { this->receive_note_off(channel, note, velocity); }
            );

            PitchTriggerSource pitch_trigger = PitchTriggerSource(
                [=](int8_t note, int8_t velocity) -> void {
                    this->chord_player.trigger_on_for_pitch(
                        note,
                        velocity,
                        this->chord_player.get_selected_chord(),
                        this->chord_player.get_inversion()
                    );
                },
                [=](int8_t note, int8_t velocity) -> void {
                    this->chord_player.trigger_off_for_pitch_because_length(note, velocity);
                },
                [=](int8_t note, int8_t velocity) -> void {
                    this->chord_player.trigger_off_for_pitch_because_changed(note, velocity);
                }
            );
        #endif

        DeviceBehaviour_CVInput(const char *label = nullptr) : DeviceBehaviourUltimateBase () {
            if (label!=nullptr)
                strncpy(this->label, label, MAX_LABEL_LENGTH);
        }
        virtual ~DeviceBehaviour_CVInput() {
            if (this->pitch_input!=nullptr) delete this->pitch_input;
            if (this->velocity_input!=nullptr) delete this->velocity_input;
        }

        char label[MAX_LABEL_LENGTH] = "Pitch CV Input";
        virtual const char *get_label() override {
            return label;
        }

        virtual int getType() override {
            return BehaviourType::virt;
        }
        virtual bool receives_midi_notes() override {
            return true;
        }

        BaseParameterInput *pitch_input = nullptr;
        BaseParameterInput *velocity_input = nullptr;

        // todo: move this functionality to a generic PitchedMonophonicMIDIOutputBehaviour class...?
        // todo: make a Polyphonic equivalent...?
        // todo: split into mono/fake-poly/true-poly variations and put in dedicated class

        virtual void set_selected_parameter_input(BaseParameterInput *input); 
        virtual void set_selected_velocity_input(BaseParameterInput *input); 
        virtual BaseParameterInput * get_selected_parameter_input() {
            return this->pitch_input;
        }
        virtual BaseParameterInput * get_selected_velocity_input() {
            return this->velocity_input;
        }

        //void on_tick(unsigned long ticks) override {
        // if we send this during tick then the notes never get received, for some reason.  sending during on_pre_clock seems to work ok for now.
        // TODO: see if this is solved now and we can revert back to using on_tick, now that we have updated to newer version of USBHost_t36 library?
        void on_pre_clock(unsigned long ticks) override {
            int8_t new_note = NOTE_OFF;
            if (this->pitch_input!=nullptr && this->pitch_input->supports_pitch()) {
                VoltageParameterInput *voltage_source_input = (VoltageParameterInput*)this->pitch_input;
                new_note = voltage_source_input->get_voltage_pitch();
                if (this->debug) Serial.printf("setting pitch to %i (%2.2f)\n", new_note, this->pitch_input->get_normal_value_unipolar());
            }

            int velocity = MIDI_MAX_VELOCITY;
            if (this->velocity_input!=nullptr) {
                velocity = constrain(((float)MIDI_MAX_VELOCITY)*(float)this->velocity_input->get_normal_value_unipolar(), 0, MIDI_MAX_VELOCITY);
                if (this->debug) Serial.printf("setting velocity to %i (%2.2f)\n", velocity, this->velocity_input->get_normal_value_unipolar());
            }

            #ifdef ENABLE_SCALES
                this->pitch_trigger.on_pre_clock(ticks, new_note, velocity);
            #endif
        }

        virtual void setup_saveable_settings() override {
            DeviceBehaviourUltimateBase::setup_saveable_settings();

            register_setting(new LSaveableSetting<const char*>(
                "pitch_source", "CV",
                nullptr,
                [=](const char* name) {
                    BaseParameterInput *source = (name && name[0]) ? parameter_manager->getInputForName((char*)name) : nullptr;
                    if (source != nullptr)
                        this->set_selected_parameter_input(source);
                    else {
                        this->set_selected_parameter_input(nullptr);
                        if (name && name[0])
                            messages_log_add(String(F("WARNING: CVInput couldn't find pitch input '")) + name + "'");
                    }
                },
                [=]() -> const char* {
                    return (this->pitch_input != nullptr) ? this->pitch_input->name : nullptr;
                }
            ), false, SL_SCOPE_PROJECT);
            register_setting(new LSaveableSetting<const char*>(
                "velocity_source", "CV",
                nullptr,
                [=](const char* name) {
                    BaseParameterInput *source = (name && name[0]) ? parameter_manager->getInputForName((char*)name) : nullptr;
                    if (source != nullptr)
                        this->set_selected_velocity_input(source);
                    else {
                        this->set_selected_velocity_input(nullptr);
                        if (name && name[0])
                            messages_log_add(String(F("WARNING: CVInput couldn't find velocity input '")) + name + "'");
                    }
                },
                [=]() -> const char* {
                    return (this->velocity_input != nullptr) ? this->velocity_input->name : nullptr;
                }
            ), false, SL_SCOPE_PROJECT);

            #ifdef ENABLE_SCALES
                register_child(&this->pitch_trigger);
                register_child(&this->chord_player);
            #endif
        }


        #ifdef ENABLE_PARAMETERS
            bool already_initialised = false;
            //FLASHMEM 
            virtual LinkedList<FloatParameter*> *initialise_parameters() override {
                //Serial.printf(F("DeviceBehaviour_CraftSynth#initialise_parameters()..."));
                if (already_initialised && this->parameters!=nullptr)
                    return this->parameters;

                DeviceBehaviourUltimateBase::initialise_parameters();
                
                // these two don't work?  and probably don't make too much sense to allow to be modulated anyway...
                //parameters->add(new DataParameter<DeviceBehaviour_CVInput,int8_t>("Scale Root", this, &DeviceBehaviour_CVInput::set_scale_root, &DeviceBehaviour_CVInput::get_scale_root, 0, 12));
                //parameters->add(new DataParameter<DeviceBehaviour_CVInput,SCALE>("Scale", this, &DeviceBehaviour_CVInput::set_scale, &DeviceBehaviour_CVInput::get_scale, (SCALE)0, (SCALE)NUMBER_SCALES));

                // these probably work, but we don't have enough flash to add this right now!
                //parameters->add(new DataParameter<DeviceBehaviour_CVInput,bool>("Quantise", this, &DeviceBehaviour_CVInput::set_quantise, &DeviceBehaviour_CVInput::is_quantise));
                //parameters->add(new DataParameter<DeviceBehaviour_CVInput,bool>("Play Chords", this, &DeviceBehaviour_CVInput::set_play_chords, &DeviceBehaviour_CVInput::is_play_chords));

                parameters->add(new ChordTypeParameter<ChordPlayer>("Chord Type", &this->chord_player, &ChordPlayer::set_selected_chord, &ChordPlayer::get_selected_chord));
                parameters->add(
                    new LDataParameter<int8_t>(
                        "Inversion", 
                        [=](int8_t v) -> void { this->chord_player.set_inversion(v); }, 
                        [=](void) -> int8_t { return this->chord_player.get_inversion(); }, 
                        0, 
                        MAXIMUM_INVERSIONS
                    )
                );

                //Serial.printf(F("Finished initialise_parameters() in %s\n"), this->get_label());

                return parameters;
            }
        #endif

        #ifdef ENABLE_SCREEN
            ParameterInputSelectorControl<DeviceBehaviour_CVInput> *pitch_parameter_selector = nullptr;
            ParameterInputSelectorControl<DeviceBehaviour_CVInput> *velocity_parameter_selector = nullptr;
            //FLASHMEM
            virtual LinkedList<MenuItem *> *make_menu_items() override;
        #endif

};

extern DeviceBehaviour_CVInput *behaviour_cvinput_1;
extern DeviceBehaviour_CVInput *behaviour_cvinput_2;
extern DeviceBehaviour_CVInput *behaviour_cvinput_3;

