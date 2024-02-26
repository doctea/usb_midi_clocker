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

// todo: move this to Parameter library, or midihelpers scale library..?
template<class TargetClass=DeviceBehaviour_CVInput, class DataType=CHORD::Type>
class ChordTypeParameter : public DataParameter<TargetClass, DataType> {
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

        virtual const char* parseFormattedDataType(DataType value) {
            static char fmt[MENU_C_MAX] = "              ";
            snprintf(fmt, MENU_C_MAX, chords[value].label);
            return fmt;
            //return "??";
        };
};


class DeviceBehaviour_CVInput : /* virtual */ public DeviceBehaviourUltimateBase {  // making virtual increases code usage by about 500 bytes!
    public:

        DeviceBehaviour_CVInput(const char *label = nullptr) : DeviceBehaviourUltimateBase () {
            if (label!=nullptr)
                strncpy(this->label, label, MAX_LABEL_LENGTH);
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
        bool is_playing = false;
        bool is_playing_chord = false;
        int last_note = -1, current_note = -1, current_raw_note = -1;
        CHORD::Type last_chord = CHORD::NONE, current_chord = CHORD::NONE, selected_chord_number = CHORD::NONE;
        unsigned long note_started_at_tick = 0;
        int32_t note_length_ticks = PPQN;
        int32_t trigger_on_ticks = 0;   // 0 = on change
        int32_t trigger_delay_ticks = 0;
        int8_t inversion = 0;

        chord_instance_t current_chord_data;
        chord_instance_t last_chord_data;

        #ifdef DEBUG_VELOCITY
            int8_t velocity = MIDI_MAX_VELOCITY;
        #endif

 
        uint8_t channel = 0;
        #ifdef CVINPUT_CONFIGURABLE_CHANNEL
            uint8_t get_channel() { return channel; }
            void set_channel(uint8_t channel) { this->channel = channel; }
        #endif

        bool quantise = false, play_chords = false;
        SCALE scale = SCALE::MAJOR;
        int8_t scale_root = SCALE_ROOT_C;

        void set_scale(SCALE scale) {
            //trigger_off_for_pitch_because_changed(this->current_note);
            this->scale = scale;
        }
        SCALE get_scale() {
            return this->scale;
        }
        void set_scale_root(int8_t scale_root) {
            //trigger_off_for_pitch_because_changed(this->current_note);
            this->scale_root = scale_root;
        }
        int8_t get_scale_root() {
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
            trigger_off_for_pitch_because_changed(this->current_note);
            this->play_chords = play_chords;
        }
        bool is_play_chords() {
            return this->play_chords;
        }

        int8_t get_inversion() {
            return inversion;
        }
        void set_inversion(int8_t inversion) {
            //trigger_off_for_pitch_because_changed(this->current_note);
            /*if (this->inversion!=inversion)
                Serial.printf("%s#set_inversion(%i) (was previously %i)\n", this->get_label(), inversion, this->inversion);*/
            this->inversion = inversion;
        }
        void set_selected_chord(CHORD::Type chord) {
            /*if (this->selected_chord_number!=chord)
                Serial.printf("%s#set_selected_chord(%i) aka %s\n", this->get_label(), chord, chords[chord].label);*/
            this->selected_chord_number = chord;
        }
        CHORD::Type get_selected_chord() {
            return this->selected_chord_number;
        }

        virtual void set_selected_parameter_input(BaseParameterInput *input); 
        virtual void set_selected_velocity_input(BaseParameterInput *input); 
        virtual BaseParameterInput * get_selected_parameter_input() {
            return this->pitch_input;
        }
        virtual BaseParameterInput * get_selected_velocity_input() {
            return this->velocity_input;
        }
        
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
        virtual void set_trigger_delay_ticks(int32_t delay_ticks) {
            this->trigger_delay_ticks = delay_ticks;
        }
        virtual int32_t get_trigger_delay_ticks () {
            return this->trigger_delay_ticks;
        }

        #ifdef ENABLE_SCREEN
            ParameterInputSelectorControl<DeviceBehaviour_CVInput> *pitch_parameter_selector = nullptr;
            ParameterInputSelectorControl<DeviceBehaviour_CVInput> *velocity_parameter_selector = nullptr;
            virtual LinkedList<MenuItem *> *make_menu_items() override;
        #endif

        void stop_chord(chord_instance_t chord) {
            this->stop_chord(chord.chord_root, chord.chord_type, chord.inversion, chord.velocity);
        }

        void stop_chord(int8_t pitch, CHORD::Type chord_number = CHORD::TRIAD, int8_t inversion = 0, uint8_t velocity = 0) {
            if (debug) Serial.printf("\t---\nstop_chord: Stopping chord for %i (%s) - chord type %s, inversion %i\n", pitch, get_note_name_c(pitch), chords[chord_number].label, inversion);

            //int8_t n = -1;
            //for (size_t i = 0 ; (n = quantise_pitch_chord_note(pitch, chord_number, i, this->scale_root, this->scale, this->current_chord_data->inversion, this->debug)) >= 0 ; i++) {
            for (size_t i = 0 ; i < PITCHES_PER_CHORD /*&& ((n = this->current_chord_data.pitches[i]) >= 0)*/ ; i++) {
                int8_t n = this->current_chord_data.pitches[i];
                if (debug) Serial.printf("\t\tStopping note\t[%i/%i]: %i\t(%s)\n", i, PITCHES_PER_CHORD, n, get_note_name_c(n));
                if (is_valid_note(n))
                    receive_note_off(channel, n, velocity);
            }
            
            last_chord = this->current_chord;
            this->last_chord_data = current_chord_data;
            this->is_playing = false;
            this->is_playing_chord = false;
            this->current_chord_data.clear();
            if (debug) Serial.println("---");
        }
        void play_chord(int8_t pitch, CHORD::Type chord_number = CHORD::TRIAD, int8_t inversion = 0, uint8_t velocity = MIDI_MAX_VELOCITY) {
            if (debug) Serial.printf("\t--- play_chord: playing chord for %i (%s) - chord type %s, inversion %i\n", pitch, get_note_name_c(pitch), chords[chord_number].label, inversion);
            if (is_playing_chord)
                this->stop_chord(this->current_chord_data);
            int8_t n = -1;
            this->current_chord_data.clear();
            this->current_chord_data.set(chord_number, pitch, inversion, velocity);
            //this->last_chord = current_chord;
            current_chord = chord_number;
            is_playing_chord = true;

            int8_t previously_played_note = -1; // avoid duplicating notes, like what happens sometimes when playing inverted +octaved chords..!
            for (size_t i = 0 ; i < PITCHES_PER_CHORD && ((n = quantise_pitch_chord_note(pitch, chord_number, i, this->get_scale_root(), this->get_scale(), inversion, this->debug)) >= 0) ; i++) {
                this->current_chord_data.set_pitch(i, n);
                if (debug) Serial.printf("\t\tPlaying note\t[%i/%i]: %i\t(%s)\n", i, PITCHES_PER_CHORD, n, get_note_name_c(n));
                if (n!=previously_played_note) {
                    receive_note_on(channel, n, velocity);
                } else {
                    if (debug) Serial.printf("\t\tSkipping note\t[%i/%i]: %i\t(%s)\n", i, PITCHES_PER_CHORD, n, get_note_name_c(n));
                }
                previously_played_note = n;
            }
            if (debug) Serial.println("---");
        }

        virtual void trigger_off_for_pitch_because_length(int8_t pitch, uint8_t velocity = MIDI_MIN_VELOCITY) {
            // don't reset current_note so that we don't retrigger the same note again immediately
            if (is_playing_chord) //is_quantise()) 
                this->stop_chord(this->current_chord_data);
            else
                this->receive_note_off(channel, this->current_note, 0);

            is_playing = false;
            this->last_note = pitch;
        }
        virtual void trigger_off_for_pitch_because_changed(int8_t pitch, uint8_t velocity = MIDI_MIN_VELOCITY) {
            if (is_playing_chord) //is_quantise())
                this->stop_chord(this->current_chord_data);
            else
                this->receive_note_off(channel, this->current_note, 0);

            this->is_playing = false;
            this->last_note = this->current_note;
            this->current_note = 255;
        }
        virtual void trigger_on_for_pitch(int8_t pitch, uint8_t velocity = MIDI_MAX_VELOCITY, CHORD::Type chord_number = CHORD::TRIAD, int8_t inversion = 0) {
            if (this->is_playing)
                this->stop_chord(this->current_chord_data);

            this->current_note = pitch;
            this->note_started_at_tick = ticks;
            #ifdef DEBUG_VELOCITY
                this->velocity = velocity;
            #endif
            if (!is_quantise() || !is_play_chords() || this->selected_chord_number==CHORD::NONE)
                this->receive_note_on(channel, this->current_note, velocity);
            else
                this->play_chord(pitch, chord_number, inversion, velocity);
            this->is_playing = true;
        }

        //void on_tick(unsigned long ticks) override {
        // if we send this during tick then the notes never get received, for some reason.  sending during on_pre_clock seems to work ok for now.
        // TODO: see if this is solved now and we can revert back to using on_tick, now that we have updated to newer version of USBHost_t36 library?
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
                
                if (!(get_trigger_on_ticks()==0 || (ticks-trigger_delay_ticks) % get_trigger_on_ticks()==0))
                    return;

                VoltageParameterInput *voltage_source_input = (VoltageParameterInput*)this->pitch_input;
                int new_note = voltage_source_input->get_voltage_pitch();
                this->current_raw_note = new_note;
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
                        int velocity = MIDI_MAX_VELOCITY;
                        if (this->velocity_input!=nullptr) {
                            velocity = constrain(((float)MIDI_MAX_VELOCITY)*(float)this->velocity_input->get_normal_value_unipolar(), 0, MIDI_MAX_VELOCITY);
                            if (this->debug) Serial.printf("setting velocity to %i (%2.2f)\n", velocity, this->velocity_input->get_normal_value_unipolar());
                        }

                        if (this->debug) Serial.printf(F("CVInput: Starting note %i\tat\t%u\n"), new_note, ticks);
                        trigger_on_for_pitch(new_note, velocity, selected_chord_number, this->inversion);
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

            // key centre + scale
            this->saveable_parameters->add(new LSaveableParameter<int8_t>    
                ("scale_root", "CV", &this->scale_root, [=](int8_t v) -> void { this->set_scale_root(v); }, [=]() -> int8_t { return this->get_scale_root(); } ));
            // scale number (key) is handled in load_parse_key_value/save_sequence_add_lines because SCALE type breaks SaveableParameter
            //this->saveable_parameters->add(new LSaveableParameter<SCALE>("scale_number", "CV", &this->scale, [=](SCALE v) -> void { this->set_scale(v); }, [=]() -> SCALE { return this->get_scale(); }));

            // note duration and triggers
            this->saveable_parameters->add(new LSaveableParameter<int32_t>
                ("note_length_ticks", "CV", &this->note_length_ticks, [=](int32_t v) -> void{ this->set_note_length(v); }, [=]() -> int32_t { return this->get_note_length(); } ));
            this->saveable_parameters->add(new LSaveableParameter<int32_t>
                ("trigger_each", "CV", &this->trigger_on_ticks, [=](int32_t v) -> void{ this->set_trigger_on_ticks(v); }, [=]() -> int32_t { return this->get_trigger_on_ticks(); } ));
            this->saveable_parameters->add(new LSaveableParameter<int32_t>
                ("trigger_delay_ticks", "CV", &this->trigger_delay_ticks, [=](int32_t v) -> void { this->set_trigger_delay_ticks(v); }, [=]() -> int32_t { return this->get_trigger_delay_ticks(); } ));

            // quantisation settings
            this->saveable_parameters->add(new LSaveableParameter<bool>   
                ("quantise_enable", "CV", &this->quantise, [=](bool v) -> void { this->set_quantise(v); }, [=]() -> bool { return this->is_quantise(); } ));
            this->saveable_parameters->add(new LSaveableParameter<bool>   
                ("play_chords", "CV", &this->play_chords, [=](bool v) -> void{ this->set_play_chords(v); }, [=]() -> bool { return this->is_play_chords(); } ));
            this->saveable_parameters->add(new LSaveableParameter<CHORD::Type>
                ("Chord", "CV", &this->selected_chord_number, [=](CHORD::Type v) -> void { this->set_selected_chord(v); }, [=]() -> CHORD::Type { return get_selected_chord(); } ));
            this->saveable_parameters->add(new LSaveableParameter<int8_t>
                ("inversion", "CV", &this->inversion, [=](int8_t v) -> void { this->set_inversion(v); }, [=]() -> int8_t { return this->get_inversion(); } ));

        }

        // ask behaviour to process the key/value pair
        virtual bool load_parse_key_value(String key, String value) override {
            static const String warning_message = String("WARNING: Behaviour_CVInput couldn't find an input for the name '");
            if (key.equals(F("pitch_source"))) {
                //this->pitch_input = parameter_manager->getInputForName((char*)value.c_str()); //.charAt(0));
                BaseParameterInput *source = parameter_manager->getInputForName((char*)value.c_str());
                if (source!=nullptr)
                    this->set_selected_parameter_input(source);
                else {
                    this->set_selected_parameter_input(nullptr);
                    messages_log_add(warning_message + value + "'");
                }
                return true;
            } else if (key.equals(F("velocity_source"))) {
                //this->pitch_input = parameter_manager->getInputForName((char*)value.c_str()); //.charAt(0));
                BaseParameterInput *source = parameter_manager->getInputForName((char*)value.c_str());
                if (source!=nullptr)
                    this->set_selected_velocity_input(source);
                else {
                    this->set_selected_velocity_input(nullptr);
                    messages_log_add(warning_message + value + "'");
                }
                return true;
            } else if (key.equals(F("scale"))) {           // do this here because SCALE won't cast to Int implicitly TODO: solve this
                this->set_scale((SCALE)value.toInt());
                return true;
            } else if (DeviceBehaviourUltimateBase::load_parse_key_value(key, value)) {
                return true;
            }

            return false;
        }

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

            parameters->add(new ChordTypeParameter<>("Chord Type", this, &DeviceBehaviour_CVInput::set_selected_chord, &DeviceBehaviour_CVInput::get_selected_chord));
            parameters->add(new DataParameter<DeviceBehaviour_CVInput,int8_t>("Inversion", this, &DeviceBehaviour_CVInput::set_inversion, &DeviceBehaviour_CVInput::get_inversion, 0, MAXIMUM_INVERSIONS));

            //Serial.printf(F("Finished initialise_parameters() in %s\n"), this->get_label());

            return parameters;
        }

};

extern DeviceBehaviour_CVInput *behaviour_cvinput_1;
extern DeviceBehaviour_CVInput *behaviour_cvinput_2;
extern DeviceBehaviour_CVInput *behaviour_cvinput_3;

