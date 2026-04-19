#pragma once

//#define DEBUG_VELOCITY    // enable velocity control so we can tell what's going on

#include "Config.h"

#include "bpm.h"

#include "midi_helpers.h"

#include "scales.h"

#include "behaviour_base.h"
#include "parameter_inputs/VoltageParameterInput.h"
#include "cv_pitch_trigger_core.h"
#include "cv_pitch_input_wiring.h"
#include "chord_modulation_parameters.h"

#include "ParameterManager.h"
extern ParameterManager *parameter_manager;

#ifdef ENABLE_SCREEN
    //class DeviceBehaviour_CVInput;
    template<class TargetClass>
    class ParameterInputSelectorControl;
#endif

class DeviceBehaviour_CVInput;

#include "chord_player.h"
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

            CVPitchTriggerCore trigger_core = CVPitchTriggerCore(
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
            #ifdef ENABLE_SCALES
                this->trigger_core.set_debug(this->debug);
                this->trigger_core.on_pre_clock(ticks);
            #endif
        }

        virtual void setup_saveable_settings() override {
            DeviceBehaviourUltimateBase::setup_saveable_settings();

            register_setting(
                CVPitchInputWiring::make_group_and_name_parameter_input_setting<DeviceBehaviour_CVInput>(
                    "pitch_source",
                    "CV",
                    this,
                    &DeviceBehaviour_CVInput::set_selected_parameter_input,
                    &DeviceBehaviour_CVInput::get_selected_parameter_input,
                    "",
                    [=](const char *group_and_name) -> void {
                        messages_log_add(String(F("WARNING: CVInput couldn't find pitch input '")) + group_and_name + "'");
                    }
                ),
                false,
                SL_SCOPE_PROJECT
            );

            register_setting(
                CVPitchInputWiring::make_group_and_name_parameter_input_setting<DeviceBehaviour_CVInput>(
                    "velocity_source",
                    "CV",
                    this,
                    &DeviceBehaviour_CVInput::set_selected_velocity_input,
                    &DeviceBehaviour_CVInput::get_selected_velocity_input,
                    "",
                    [=](const char *group_and_name) -> void {
                        messages_log_add(String(F("WARNING: CVInput couldn't find velocity input '")) + group_and_name + "'");
                    }
                ),
                false,
                SL_SCOPE_PROJECT
            );

            #ifdef ENABLE_SCALES
                register_child(this->trigger_core.get_pitch_trigger());
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

                add_chord_modulation_parameters(parameters, &this->chord_player);

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

