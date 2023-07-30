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

class DeviceBehaviour_CVInput;

template<class TargetClass=DeviceBehaviour_CVInput, class DataType=CHORD::Type>
class ChordTypeParameter : public DataParameter<TargetClass, DataType> {
    public:
        ChordTypeParameter(const char *label, TargetClass *target, void(TargetClass::*setter_func)(DataType), DataType(TargetClass::*getter_func)()) : DataParameter<TargetClass, DataType>(label, target, setter_func, getter_func) {}

        virtual DataType normalToData(float value) override {
            /*if (this->debug && value>=0.0) {
                Serial.printf(F("%s#"), this->label);
                Serial.printf(F("normalToData(%f) "), value);
                Serial.printf(F(", range is %i to %i "), this->minimumDataValue, this->maximumDataValue);
            }*/
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
        }

        virtual const char* parseFormattedDataType(DataType value) {
            static char fmt[MENU_C_MAX] = "              ";
            snprintf(fmt, MENU_C_MAX, chords[value].label);
            return fmt;
            //return "??";
        };
};


class DeviceBehaviour_CVInput : public DeviceBehaviourUltimateBase {
    public:
        virtual const char *get_label() override {
            return "Pitch CV Input";
        }

        virtual int getType() {
            return BehaviourType::virt;
        }

        BaseParameterInput *pitch_input = nullptr;
        BaseParameterInput *velocity_input = nullptr;

        bool is_playing = false;
        int last_note = -1, current_note = -1;
        CHORD::Type last_chord = CHORD::NONE, current_chord = CHORD::NONE, selected_chord_number = CHORD::NONE;
        unsigned long note_started_at_tick = 0;
        int32_t note_length_ticks = PPQN;
        int32_t trigger_on_ticks = 0;   // 0 = on change
 
        byte channel = 0;
        #ifdef CVINPUT_CONFIGURABLE_CHANNEL
            byte get_channel() { return channel; }
            void set_channel(byte channel) { this->channel = channel; }
        #endif

        bool quantise = false, play_chords = false;
        SCALE scale = SCALE::MAJOR;
        int scale_root = SCALE_ROOT_C;

        void set_scale(SCALE scale) {
            trigger_off_for_pitch_because_changed(this->current_note);
            this->scale = scale;
        }
        SCALE get_scale() {
            return this->scale;
        }
        void set_scale_root(int scale_root) {
            trigger_off_for_pitch_because_changed(this->current_note);
            this->scale_root = scale_root;
        }
        int get_scale_root() {
            return this->scale_root;
        }
        void set_quantise(bool quantise) {
            trigger_off_for_pitch_because_changed(this->current_note);
            this->quantise = quantise;
        }
        bool is_quantise() {
            return this->quantise;
        }
        void set_play_chords(bool play_chords) {
            //trigger_off_for_pitch_because_changed(this->current_note);
            this->play_chords = play_chords;
        }
        bool is_play_chords() {
            return this->play_chords;
        }

        void set_selected_chord(CHORD::Type chord) {
            this->selected_chord_number = chord;
        }
        CHORD::Type get_selected_chord() {
            return this->selected_chord_number;
        }

        virtual void set_selected_parameter_input(BaseParameterInput *input); 
        virtual void set_selected_velocity_input(BaseParameterInput *input); 
        
        virtual void set_note_length(int32_t length_ticks) {
            this->note_length_ticks = length_ticks;
        }
        virtual int32_t get_note_length () {
            return this->note_length_ticks;
        }
        virtual void set_trigger_on_ticks(int32_t length_ticks) {
            this->trigger_on_ticks = length_ticks;
        }
        virtual int32_t get_trigger_on_ticks () {
            return this->trigger_on_ticks;
        }

        //int8_t chord_held_notes[127];

        #ifdef ENABLE_SCREEN
            ParameterInputSelectorControl<DeviceBehaviour_CVInput> *pitch_parameter_selector = nullptr;
            ParameterInputSelectorControl<DeviceBehaviour_CVInput> *velocity_parameter_selector = nullptr;
            LinkedList<MenuItem *> *make_menu_items() override;
        #endif

        void stop_chord(int8_t pitch, CHORD::Type chord_number = CHORD::TRIAD, byte velocity = 0) {
            if (debug) Serial.printf("\t--- Stopping chord for %i (%s)\n", pitch, get_note_name_c(pitch));
            int8_t n = -1;
            last_chord = this->current_chord;
            for (int i = 0 ; (n = quantise_pitch_chord_note(pitch, chord_number, i, this->scale_root, this->scale)) >= 0 ; i++) {
                if (debug) Serial.printf("Stopping note %i: %i\t(%s)\n", i, n, get_note_name_c(n));
                receive_note_off(channel, n, velocity);
            }
        }
        void play_chord(int8_t pitch, CHORD::Type chord_number = CHORD::TRIAD, byte velocity = 127) {
            if (debug) Serial.printf("\t--- playing chord for %i (%s)\n", pitch, get_note_name_c(pitch));
            int8_t n = -1;
            this->last_chord = chord_number;
            for (int i = 0 ; (n = quantise_pitch_chord_note(pitch, chord_number, i, this->scale_root, this->scale)) >= 0 ; i++) {
                if (debug) Serial.printf("Playing note %i: %i\t(%s)\n", i, n, get_note_name_c(n));
                receive_note_on(channel, n, velocity);
            }
            /*int i = 0;
            int8_t n = quantise_pitch_chord_note(pitch, CHORD::TRIAD, i++, this->scale_root, this->scale);
            while(n>=0) {
                receive_note_on(channel, n, velocity);
                n = quantise_pitch_chord_note(pitch, CHORD::TRIAD, i++, this->scale_root, this->scale);
            }*/

            Serial.println("---");
        }

        virtual void trigger_off_for_pitch_because_length(int8_t pitch, byte velocity = 0) {
            // don't reset current_note so that we don't retrigger the same note again immediately
            if (!is_quantise()) 
                this->receive_note_off(channel, this->current_note, 0);
            else 
                this->stop_chord(this->current_note, this->last_chord);

            is_playing = false;
            this->last_note = pitch;
        }
        virtual void trigger_off_for_pitch_because_changed(int8_t pitch, byte velocity = 0) {
            if (!is_quantise())
                this->receive_note_off(channel, pitch, 0);
            else 
                this->stop_chord(pitch, this->last_chord);
            this->is_playing = false;
            this->last_note = this->current_note;
            this->current_note = 255;
        }
        virtual void trigger_on_for_pitch(int8_t pitch, byte velocity = 127, CHORD::Type chord_number = CHORD::TRIAD) {
            this->current_note = pitch;
            this->note_started_at_tick = ticks;
            if (!is_quantise() || chord_number==CHORD::NONE)
                this->receive_note_on(channel, this->current_note, velocity);
            else
                this->play_chord(pitch, chord_number);
            this->is_playing = true;
        }

        //void on_tick(unsigned long ticks) override {
        // if we send this during tick then the notes never get received, for some reason.  sending during on_pre_clock seems to work ok for now.
        void on_pre_clock(unsigned long ticks) override {
            // check if playing note duration has passed regardless of whether pitch_input is set, so that notes will still finish even if disconncted
            if (is_playing && this->get_note_length()>0 && abs((long)this->note_started_at_tick-(long)ticks) >= this->get_note_length()) {
                if (this->debug) Serial.printf(F("CVInput: Stopping note\t%i because playing and elapsed is (%u-%u=%u)\n"), current_note, note_started_at_tick, ticks, abs((long)this->note_started_at_tick-(long)ticks));
                trigger_off_for_pitch_because_length(current_note);
                //this->current_note = -1; // dont clear current_note, so that we don't retrigger it again
            }

            // if source input is connected, we wanna check for values
            if (this->pitch_input!=nullptr) {
                // TODO: make this tolerant of other types of ParameterInput!
                if (!this->pitch_input->supports_pitch()) return;
                
                if (!(get_trigger_on_ticks()==0 || ticks % get_trigger_on_ticks()==0))
                    return;

                VoltageParameterInput *voltage_source_input = (VoltageParameterInput*)this->pitch_input;
                int new_note = voltage_source_input->get_voltage_pitch();
                if (this->is_quantise())
                    new_note = quantise_pitch(new_note, this->scale_root, this->scale);

                // has pitch become invalid?  is so and if note playing, stop note
                if (is_playing && !is_valid_note(new_note) && is_valid_note(this->current_note)) {
                    if (this->debug) Serial.printf(F("CVInput: Stopping note\t%i because playing and new_note isn't valid\n"), new_note);
                    trigger_off_for_pitch_because_changed(this->current_note);
                } else if (is_valid_note(new_note) && (new_note!=this->current_note || this->get_trigger_on_ticks()>0)) {
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
                        trigger_on_for_pitch(new_note, velocity, selected_chord_number);
                    }
                }
            }
        }

        // todo: do we like, want a 'save_global_add_lines' sorta thing for global configs?  or should this be project config?
        virtual void save_project_add_lines(LinkedList<String> *lines) override {
            if (this->pitch_input!=nullptr)
                lines->add(String(F("pitch_source=")) + String(this->pitch_input->name));
            if (this->velocity_input!=nullptr)
                lines->add(String(F("velocity_source=")) + String(this->velocity_input->name));
        }

        virtual void save_sequence_add_lines(LinkedList<String> *lines) override {
            DeviceBehaviourUltimateBase::save_sequence_add_lines(lines);
            lines->add(String(F("scale=")) + String(this->get_scale()));    // add this here because SCALE won't cast to Int implicitly TODO: solve this
        }

        virtual void setup_saveable_parameters() override {
            if (this->saveable_parameters==nullptr)
                DeviceBehaviourUltimateBase::setup_saveable_parameters();
            this->saveable_parameters->add(new SaveableParameter<DeviceBehaviour_CVInput,int32_t>("note_length_ticks", "CV", this, &this->note_length_ticks, nullptr, nullptr, &DeviceBehaviour_CVInput::set_note_length, &DeviceBehaviour_CVInput::get_note_length));
            this->saveable_parameters->add(new SaveableParameter<DeviceBehaviour_CVInput,int>("scale_root", "CV", this, &this->scale_root, nullptr, nullptr, &DeviceBehaviour_CVInput::set_scale_root, &DeviceBehaviour_CVInput::get_scale_root));
            this->saveable_parameters->add(new SaveableParameter<DeviceBehaviour_CVInput,int32_t>("trigger_on", "CV", this, &this->trigger_on_ticks, nullptr, nullptr, &DeviceBehaviour_CVInput::set_trigger_on_ticks, &DeviceBehaviour_CVInput::get_trigger_on_ticks));
            //this->saveable_parameters->add(new SaveableParameter<DeviceBehaviour_CVInput,SCALE>("scale_number", "CV", this, &this->scale, nullptr, nullptr, &DeviceBehaviour_CVInput::set_scale, &DeviceBehaviour_CVInput::get_scale));
        }

        // ask behaviour to process the key/value pair
        virtual bool load_parse_key_value(String key, String value) override {
            if (key.equals(F("pitch_source"))) {
                //this->pitch_input = parameter_manager->getInputForName((char*)value.c_str()); //.charAt(0));
                BaseParameterInput *source = parameter_manager->getInputForName((char*)value.c_str());
                if (source!=nullptr)
                    this->set_selected_parameter_input(source);
                else
                    messages_log_add(String("WARNING: Behaviour_CVInput couldn't find an input for the name '" + value + "'"));
                return true;
            } else if (key.equals(F("velocity_source"))) {
                //this->pitch_input = parameter_manager->getInputForName((char*)value.c_str()); //.charAt(0));
                BaseParameterInput *source = parameter_manager->getInputForName((char*)value.c_str());
                if (source!=nullptr)
                    this->set_selected_velocity_input(source);
                else
                    messages_log_add(String("WARNING: Behaviour_CVInput couldn't find an input for the name '" + value + "'"));
                return true;
            } else if (key.equals(F("scale"))) {           // do this here because SCALE won't cast to Int implicitly TODO: solve this
                this->set_scale((SCALE)value.toInt());
                return true;
            } else if (DeviceBehaviourUltimateBase::load_parse_key_value(key, value)) {
                return true;
            }

            return false;
        }


        //FLASHMEM 
        virtual LinkedList<FloatParameter*> *initialise_parameters() override {
            //Serial.printf(F("DeviceBehaviour_CraftSynth#initialise_parameters()..."));
            static bool already_initialised = false;
            if (already_initialised)
                return this->parameters;

            DeviceBehaviourUltimateBase::initialise_parameters();

            parameters->add(new ChordTypeParameter<>("Chord Type", this, &DeviceBehaviour_CVInput::set_selected_chord, &DeviceBehaviour_CVInput::get_selected_chord));
            
            //Serial.printf(F("Finished initialise_parameters() in %s\n"), this->get_label());

            return parameters;
        }

};

extern DeviceBehaviour_CVInput *behaviour_cvinput;

#endif