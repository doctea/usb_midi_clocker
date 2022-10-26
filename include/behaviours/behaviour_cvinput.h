#include "Config.h"

#include "bpm.h"

#include "midi/midi_helpers.h"

#include "behaviour_base.h"
#include "parameter_inputs/VoltageParameterInput.h"

#include "ParameterManager.h"
extern ParameterManager *parameter_manager;

class DeviceBehaviour_CVInput : public DeviceBehaviourUltimateBase {
    public:
        virtual const char *get_label() override {
            return (char*)"CV Input";
        }

        VoltageParameterInput *source_input = nullptr;

        bool is_playing = false;
        int last_note = -1, current_note = -1;
        unsigned long note_started_at_tick = 0;
        int32_t note_length_ticks = PPQN;

        #ifdef ENABLE_SCREEN
            LinkedList<MenuItem *> *make_menu_items() override;
        #endif

        virtual void set_selected_parameter_input(BaseParameterInput *input) {
            Serial.printf(F("set_selected_parameter_input(%c)\n"), input->name);
            // TODO: make this tolerant of other ParameterInput types
            this->source_input = (VoltageParameterInput*)input;
            if (input==nullptr)
                Serial.printf(F("nullptr passed to set_selected_parameter_input(BaseParameterInput *input)\n"));
            //else
            //Serial.printf("WARNING in %s: set_selected_parameter_input() not passed a VoltageParameterInput in '%c'!\n", this->get_label(), input->name);               
        }
        virtual void set_note_length(int32_t length_ticks) {
            this->note_length_ticks = length_ticks;
        }
        virtual int32_t get_note_length () {
            return this->note_length_ticks;
        }

        void on_tick(unsigned long ticks) override {

            // check if playing note duration has passed regardless of whether source_input is set, so that notes will still finish even if disconncted
            if (is_playing && abs((long)this->note_started_at_tick-(long)ticks) >= this->get_note_length()) {
                if (this->debug) Serial.printf(F("Stopping note\t%i because playing and elapsed is (%u-%u=%u)\n"), current_note, note_started_at_tick, ticks, abs((long)this->note_started_at_tick-(long)ticks));
                this->last_note = current_note;
                is_playing = false;
                this->receive_note_off(1, this->current_note, 0);
                //this->current_note = -1; // dont clear current_note, so that we don't retrigger it again
            }

            // if source input is connected, we wanna check for values
            if (this->source_input!=nullptr) {
                int new_note = this->source_input->get_voltage_pitch();

                // has pitch become invalid?  is so and if note playing, stop note
                if (is_playing && !is_valid_note(new_note) && is_valid_note(this->current_note)) {
                    if (this->debug) Serial.printf(F("Stopping note\t%i because playing and new_note isn't valid\n"), new_note);
                    this->receive_note_off(1, this->current_note, 0);
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
                        if (this->debug) Serial.printf("Stopping note\t%i because of new_note\t%i\n", this->current_note, new_note);
                        this->receive_note_off(1, this->current_note, 0);
                        this->last_note = current_note;
                        this->current_note = 255;
                        this->is_playing = false;
                    }
                    if (this->get_note_length()>0) {
                        if (this->debug) Serial.printf("Starting note %i\tat\t%u\n", new_note, ticks);
                        this->current_note = new_note;
                        this->note_started_at_tick = ticks;
                        this->receive_note_on(1, this->current_note, 127);
                        this->is_playing = true;
                    }
                }
            }
        }

        // todo: do we like, want a 'save_global_add_lines' sorta thing for global configs?  or should this be project config?
        virtual void save_project_add_lines(LinkedList<String> *lines) override {               
            if (this->source_input!=nullptr)
                lines->add(String("parameter_source=") + String(this->source_input->name));
        }

        virtual void save_sequence_add_lines(LinkedList<String> *lines) override {
            DeviceBehaviourUltimateBase::save_project_add_lines(lines);
            lines->add(String("note_length_ticks=") + String(this->get_note_length()));
        }

        // ask behaviour to process the key/value pair
        virtual bool load_parse_key_value(String key, String value) override {
            if (key.equals("note_length_ticks")) {
                this->set_note_length((int) value.toInt());
                return true;
            } else if (key.equals("parameter_source")) {
                this->source_input = (VoltageParameterInput*)parameter_manager->getInputForName(value.charAt(0));
            } else if (DeviceBehaviourUltimateBase::load_parse_key_value(key, value)) {
                return true;
            }

            return false;
        }

};

extern DeviceBehaviour_CVInput *behaviour_cvinput;