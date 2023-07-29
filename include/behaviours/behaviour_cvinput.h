#ifndef BEHAVIOUR_CVINPUT__INCLUDED
#define BEHAVIOUR_CVINPUT__INCLUDED

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

class DeviceBehaviour_CVInput : public DeviceBehaviourUltimateBase {
    public:
        virtual const char *get_label() override {
            return "Pitch CV Input";
        }

        virtual int getType() {
            return BehaviourType::virt;
        }

        BaseParameterInput *source_input = nullptr;
        BaseParameterInput *velocity_input = nullptr;

        bool is_playing = false;
        int last_note = -1, current_note = -1;
        unsigned long note_started_at_tick = 0;
        int32_t note_length_ticks = PPQN;
 
        byte channel = 0;
        #ifdef CVINPUT_CONFIGURABLE_CHANNEL
            byte get_channel() { return channel; }
            void set_channel(byte channel) { this->channel = channel; }
        #endif

        bool quantise = false;
        SCALE scale = SCALE::MAJOR;
        int scale_root = SCALE_ROOT_C;

        void set_scale(SCALE scale) {
            this->scale = scale;
        }
        SCALE get_scale() {
            return this->scale;
        }
        void set_scale_root(int scale_root) {
            this->scale_root = scale_root;
        }
        int get_scale_root() {
            return this->scale_root;
        }
        void set_quantise(bool quantise) {
            this->quantise = quantise;
        }
        bool is_quantise() {
            return this->quantise;
        }

        virtual void set_selected_parameter_input(BaseParameterInput *input); 
        virtual void set_selected_velocity_input(BaseParameterInput *input); 
        
        virtual void set_note_length(int32_t length_ticks) {
            this->note_length_ticks = length_ticks;
        }
        virtual int32_t get_note_length () {
            return this->note_length_ticks;
        }

        #ifdef ENABLE_SCREEN
            ParameterInputSelectorControl<DeviceBehaviour_CVInput> *pitch_parameter_selector = nullptr;
            ParameterInputSelectorControl<DeviceBehaviour_CVInput> *velocity_parameter_selector = nullptr;
            LinkedList<MenuItem *> *make_menu_items() override;
        #endif

        virtual void trigger_off_for_pitch_because_length(int8_t pitch, byte velocity = 0) {
            // don't reset current_note so that we don't retrigger the same note again immediately
            this->receive_note_off(channel, this->current_note, 0);
            is_playing = false;
            this->last_note = pitch;
        }
        virtual void trigger_off_for_pitch_because_changed(int8_t pitch, byte velocity = 0) {
            this->receive_note_off(channel, pitch, 0);
            this->is_playing = false;
            this->last_note = this->current_note;
            this->current_note = 255;
        }
        virtual void trigger_on_for_pitch(int8_t pitch, byte velocity = 127) {
            this->current_note = pitch;
            this->note_started_at_tick = ticks;
            this->receive_note_on(channel, this->current_note, velocity);
            this->is_playing = true;
        }

        //void on_tick(unsigned long ticks) override {
        // if we send this during tick then the notes never get received, for some reason.  sending during on_pre_clock seems to work ok for now.
        void on_pre_clock(unsigned long ticks) override {
            // check if playing note duration has passed regardless of whether source_input is set, so that notes will still finish even if disconncted
            if (is_playing && this->get_note_length()>0 && abs((long)this->note_started_at_tick-(long)ticks) >= this->get_note_length()) {
                if (this->debug) Serial.printf(F("CVInput: Stopping note\t%i because playing and elapsed is (%u-%u=%u)\n"), current_note, note_started_at_tick, ticks, abs((long)this->note_started_at_tick-(long)ticks));
                trigger_off_for_pitch_because_length(current_note);
                //this->current_note = -1; // dont clear current_note, so that we don't retrigger it again
            }

            // if source input is connected, we wanna check for values
            if (this->source_input!=nullptr) {
                // TODO: make this tolerant of other types of ParameterInput!
                if (!this->source_input->supports_pitch()) return;
                
                VoltageParameterInput *voltage_source_input = (VoltageParameterInput*)this->source_input;
                int new_note = voltage_source_input->get_voltage_pitch();
                if (this->is_quantise())
                    new_note = quantise_pitch(new_note, this->scale_root, this->scale);

                // has pitch become invalid?  is so and if note playing, stop note
                if (is_playing && !is_valid_note(new_note) && is_valid_note(this->current_note)) {
                    if (this->debug) Serial.printf(F("CVInput: Stopping note\t%i because playing and new_note isn't valid\n"), new_note);
                    trigger_off_for_pitch_because_changed(this->current_note);

                /*} else if (is_playing && is_valid_note(new_note) && new_note==this->current_note) {
                    if (this->debug) Serial.printf("Stopping note\t%i because playing and new_note=current_note=%i\n", current_note, new_note);
                    this->receive_note_off(1, this->current_note, 0);
                    this->last_note = current_note;
                    is_playing = false;
                    // dont clear current_note, so that we don't retrigger it again
                */
                } else if (is_valid_note(new_note) && new_note!=this->current_note) {
                    // note has changed from valid to a different valid
                    if (is_playing) {
                        if (this->debug) Serial.printf(F("CVInput: Stopping note\t%i because of new_note\t%i\n"), this->current_note, new_note);
                        trigger_off_for_pitch_because_changed(this->current_note);
                    }
                    if (this->get_note_length()>0) {
                        int velocity = 127;
                        if (this->velocity_input!=nullptr) {
                            velocity = constrain(127.0*(float)this->velocity_input->get_normal_value(), 0, 127);
                            if (this->debug) Serial.printf("setting velocity to %i (%2.2f)\n", velocity, this->velocity_input->get_normal_value());
                        }

                        if (this->debug) Serial.printf(F("CVInput: Starting note %i\tat\t%u\n"), new_note, ticks);
                        trigger_on_for_pitch(new_note, velocity);
                    }
                }
            }
        }

        // todo: do we like, want a 'save_global_add_lines' sorta thing for global configs?  or should this be project config?
        virtual void save_project_add_lines(LinkedList<String> *lines) override {
            if (this->source_input!=nullptr)
                lines->add(String(F("parameter_source=")) + String(this->source_input->name));
        }

        virtual void save_sequence_add_lines(LinkedList<String> *lines) override {
            DeviceBehaviourUltimateBase::save_sequence_add_lines(lines);
            lines->add(String(F("scale=")) + String(this->get_scale()));
        }

        virtual void setup_saveable_parameters() override {
            if (this->saveable_parameters==nullptr)
                DeviceBehaviourUltimateBase::setup_saveable_parameters();
            this->saveable_parameters->add(new SaveableParameter<DeviceBehaviour_CVInput,int32_t>("note_length_ticks", "CV", this, &this->note_length_ticks, nullptr, nullptr, &DeviceBehaviour_CVInput::set_note_length, &DeviceBehaviour_CVInput::get_note_length));
            this->saveable_parameters->add(new SaveableParameter<DeviceBehaviour_CVInput,int>("scale_root", "CV", this, &this->scale_root, nullptr, nullptr, &DeviceBehaviour_CVInput::set_scale_root, &DeviceBehaviour_CVInput::get_scale_root));
            //this->saveable_parameters->add(new SaveableParameter<DeviceBehaviour_CVInput,SCALE>("scale_number", "CV", this, &this->scale, nullptr, nullptr, &DeviceBehaviour_CVInput::set_scale, &DeviceBehaviour_CVInput::get_scale));
        }

        // ask behaviour to process the key/value pair
        virtual bool load_parse_key_value(String key, String value) override {
            if (key.equals(F("parameter_source"))) {
                //this->source_input = parameter_manager->getInputForName((char*)value.c_str()); //.charAt(0));
                BaseParameterInput *source = parameter_manager->getInputForName((char*)value.c_str());
                if (source!=nullptr)
                    this->set_selected_parameter_input(source);
                else
                    messages_log_add(String("WARNING: Behaviour_CVInput couldn't find an input for the name '" + value + "'"));
                return true;
            } else if (key.equals(F("scale"))) {
                this->set_scale((SCALE)value.toInt());
                return true;
            } else if (DeviceBehaviourUltimateBase::load_parse_key_value(key, value)) {
                return true;
            }

            return false;
        }

};

extern DeviceBehaviour_CVInput *behaviour_cvinput;

#endif