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
        
        virtual void set_note_length(int32_t length_ticks) {
            this->note_length_ticks = length_ticks;
        }
        virtual int32_t get_note_length () {
            return this->note_length_ticks;
        }

        #ifdef ENABLE_SCREEN
            ParameterInputSelectorControl<DeviceBehaviour_CVInput> *parameter_input_selector;
            LinkedList<MenuItem *> *make_menu_items() override;
        #endif

        // using these (instead of receive_note_on/off directly from the processing function below) has problems with note offs not being received, weirdly
        // so just do it raw for now
        /*virtual void trigger_on_for_pitch(int8_t pitch, byte velocity = 127) {
            if (this->is_quantise())
                pitch = quantise_pitch(pitch, this->scale_root, this->scale);
            this->current_note = pitch;
            this->is_playing = true;

            this->receive_note_on(channel, pitch, velocity);
        }
        virtual void trigger_off_for_pitch(int8_t pitch, byte velocity = 0) {
            if (this->is_quantise()) {
                int8_t new_pitch = quantise_pitch(pitch, this->scale_root, this->scale);
                if (this->debug) if (new_pitch!=pitch)
                    Serial.printf("\tquantised pitch %i to to %i (current_note is %i)\n", pitch, new_pitch, current_note);
                pitch = new_pitch;
            }
            if (this->current_note==pitch) {
                this->last_note = pitch;
                this->is_playing = false;
            }
            this->receive_note_off(channel, pitch, velocity);
            this->current_note = -1; //255;
        }*/

        //void on_tick(unsigned long ticks) override {
        // if we send this during tick then the notes never get received, for some reason.  sending during on_pre_clock seems to work ok for now.
        void on_pre_clock(unsigned long ticks) override {
            // check if playing note duration has passed regardless of whether source_input is set, so that notes will still finish even if disconncted
            if (is_playing && this->get_note_length()>0 && abs((long)this->note_started_at_tick-(long)ticks) >= this->get_note_length()) {
                if (this->debug) Serial.printf(F("CVInput: Stopping note\t%i because playing and elapsed is (%u-%u=%u)\n"), current_note, note_started_at_tick, ticks, abs((long)this->note_started_at_tick-(long)ticks));
                this->last_note = current_note;
                is_playing = false;
                this->receive_note_off(channel, this->current_note, 0);
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
                    this->receive_note_off(channel, this->current_note, 0);
                    this->is_playing = false;
                    this->current_note = 255;
                    this->last_note = this->current_note;
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
                        this->receive_note_off(channel, this->current_note, 0);
                        this->last_note = current_note;
                        this->current_note = 255;
                        this->is_playing = false;
                    }
                    if (this->get_note_length()>0) {
                        if (this->debug) Serial.printf(F("CVInput: Starting note %i\tat\t%u\n"), new_note, ticks);
                        this->current_note = new_note;
                        this->note_started_at_tick = ticks;
                        this->receive_note_on(channel, this->current_note, 127);
                        this->is_playing = true;
                    }
                }
            }
        }

        // todo: do we like, want a 'save_global_add_lines' sorta thing for global configs?  or should this be project config?
        virtual void save_project_add_lines(LinkedList<String> *lines) override {
            if (this->source_input!=nullptr)
                lines->add(String(F("parameter_source=")) + String(this->source_input->name));
        }

        /*virtual void save_sequence_add_lines(LinkedList<String> *lines) override {
            DeviceBehaviourUltimateBase::save_sequence_add_lines(lines);
            lines->add(String(F("note_length_ticks=")) + String(this->get_note_length()));
        }*/

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
            } else if (DeviceBehaviourUltimateBase::load_parse_key_value(key, value)) {
                return true;
            }

            return false;
        }

};

extern DeviceBehaviour_CVInput *behaviour_cvinput;

#endif