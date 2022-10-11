#include "Config.h"

#include "bpm.h"

#include "behaviour_base.h"
#include "parameter_inputs/VoltageParameterInput.h"

#include "ParameterManager.h"
extern ParameterManager parameter_manager;

class DeviceBehaviour_CVInput : public DeviceBehaviourUltimateBase {
    public:
        virtual const char *get_label() override {
            return (char*)"CV Input";
        }

        VoltageParameterInput *source_input = nullptr;

        bool is_playing = false;
        int last_note = -1, current_note = -1;
        unsigned long note_started_at_tick = 0;
        uint32_t note_length_ticks = PPQN;

        #ifdef ENABLE_SCREEN
            FLASHMEM LinkedList<MenuItem *> *make_menu_items() override;
        #endif

        virtual void set_selected_parameter_input(VoltageParameterInput *input) {
            Serial.printf("set_selected_parameter_input(%c)\n", input->name);
            this->source_input = input;
            if (input==nullptr)
                Serial.printf("nullptr passed to set_selected_parameter_input(BaseParameterInput *input)\n");
            //else
            //Serial.printf("WARNING in %s: set_selected_parameter_input() not passed a VoltageParameterInput in '%c'!\n", this->get_label(), input->name);               
        }
        virtual void set_note_length(uint32_t length_ticks) {
            this->note_length_ticks = length_ticks;
        }
        /*virtual void set_selected_parameter_input(BaseParameterInput *input) {
            //this->source_input = input;
            if (input!=nullptr)
                Serial.printf("WARNING in %s: set_selected_parameter_input() not passed a VoltageParameterInput in '%c'!\n", this->get_label(), input->name);
            else
                Serial.printf("nullptr passed to set_selected_parameter_input(BaseParameterInput *input)\n");
        }*/
        virtual uint32_t get_note_length () {
            return this->note_length_ticks;
        }

        void on_tick(unsigned long ticks) override {
            // TODO: check this->source_input for latest news on pitch changes
            if (this->source_input!=nullptr) {
                //if (0==ticks%PPQN) {
                    //Serial.printf("%s#on_tick(%u)\n", this->get_label(), ticks); Serial.flush();
                    int new_note = this->source_input->get_voltage_pitch();

                    // has pitch become invalid?  is so and if note playing, stop note
                    if (is_playing && new_note==255 && this->current_note!=255) {
                        if (this->debug) Serial.printf("Stopping note\t%i because playing and new_note=255\n", new_note);
                        this->receive_note_off(1, this->current_note, 0);
                        this->is_playing = false;
                        this->current_note = 255;
                        this->last_note = this->current_note;
                    } else if (is_playing && new_note<=127 && new_note==this->current_note && abs((long)this->note_started_at_tick-(long)ticks) >= this->get_note_length()) {
                        if (this->debug) Serial.printf("Stopping note\t%i because playing and new_note=current_note=%i elapsed is (%u-%u=%u)\n", current_note, new_note, note_started_at_tick, ticks, abs((long)this->note_started_at_tick-(long)ticks));
                        receive_note_off(1, this->current_note, 0);
                        this->last_note = current_note;
                        is_playing = false;
                        // dont clear current_note, so that we don't retrigger it again
                    } else if (new_note<=127 && new_note!=this->current_note) {
                        if (is_playing) {
                            if (this->debug) Serial.printf("Stopping note\t%i because of new_note\t%i\n", this->current_note, new_note);
                            receive_note_off(1, this->current_note, 0);
                            this->last_note = current_note;
                            this->current_note = 255;
                        }
                        if (this->get_note_length()>0) {
                            if (this->debug) Serial.printf("Starting note %i\tat\t%u\n", new_note, ticks);
                            this->current_note = new_note;
                            this->note_started_at_tick = ticks;
                            receive_note_on(1, this->current_note, 127);
                            this->is_playing = true;
                        }
                    }
                    /*

                    bool note_off = this->current_note!=255 && (abs(this->note_started_at_tick-ticks)>PPQN || 
                                    (this->current_note!=255 && new_note!=this->current_note && new_note!=last_note));
                    bool note_on = new_note!=255 && this->current_note!=new_note;

                    //Serial.printf("%s: got CV pitch %i (%s)\n", this->get_label(), new_note, get_note_name(new_note).c_str()); Serial.flush();
                    //Serial.printf("%s: got CV pitch %i (%s)\n", this->get_label(), new_note, get_note_name(new_note).c_str()); Serial.flush();

                    if (this->current_note==255 || abs(this->note_started_at_tick-ticks)>PPQN) {
                        // stop old note if playing
                        this->current_note = 255;
                        this->last_note = this->current_note;
                    }                      
                    if (note_on) { //this->current_note!=new_note) {
                        this->current_note = new_note;

                        if (this->current_note>=0 && this->current_note<=127) {
                            this->note_started_at_tick = ticks;
                            Serial.printf("%s: new note for CV pitch %i (%s)\n", this->get_label(), new_note, get_note_name(new_note).c_str()); Serial.flush();
                            // trigger new note
                            this->receive_note_on(1, this->current_note, 127);
                        }
                    } */
                /*} else {
                    Serial.printf(".");
                }*/
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
                this->source_input = (VoltageParameterInput*)parameter_manager.getInputForName(value.charAt(0));
            } else if (DeviceBehaviourUltimateBase::load_parse_key_value(key, value)) {
                return true;
            }

            return false;
        }

};

extern DeviceBehaviour_CVInput *behaviour_cvinput;