#pragma once

//#define ENABLE_CV_OUTPUT 0x4C
#include "Config.h"

#ifdef ENABLE_CV_OUTPUT

#include "behaviour_base.h"
#include "parameter_inputs/VoltageParameterInput.h"
#include "parameters/CVOutputParameter.h"
#include "ParameterManager.h"

#include "Wire.h"

extern ParameterManager *parameter_manager;

/*#ifdef ENABLE_SCREEN
    template<class TargetClass>
    class ParameterInputSelectorControl;
#endif*/

template<class DACClass>
class DeviceBehaviour_CVOutput : public DeviceBehaviourUltimateBase {
    public:
        DACClass *dac_output = nullptr;

        CVOutputParameter<DACClass> *output_a = nullptr;
        CVOutputParameter<DACClass> *output_b = nullptr;
        CVOutputParameter<DACClass> *output_c = nullptr;
        CVOutputParameter<DACClass> *output_d = nullptr;

        DeviceBehaviour_CVOutput(const char *label = nullptr, uint8_t address = ENABLE_CV_OUTPUT, TwoWire *wire = &Wire) : DeviceBehaviourUltimateBase() {
            if (label != nullptr)
                strncpy(this->label, label, MAX_LABEL_LENGTH);
            this->dac_output = new DACClass(address, wire);
            //this->debug = true;
            //this->init();
        }
            
        DeviceBehaviour_CVOutput(const char *label = nullptr, DACClass *dac_output = nullptr) : DeviceBehaviourUltimateBase() {
            if (label != nullptr)
                strncpy(this->label, label, MAX_LABEL_LENGTH);
            this->dac_output = dac_output;
            //this->debug = true;
            //this->init();
        }

        virtual void init() override {
            DeviceBehaviourUltimateBase::init();

            if (debug && Serial) 
                Serial.println("DeviceBehaviour_CVOutput#init()..");

            if (dac_output!=nullptr) {
                if (debug && Serial) Serial.println("DeviceBehaviour_CVOutput telling dac_output to start and setting up the CVOutputParameters..");
                Wire.begin();
                dac_output->begin();

                output_a = new CVOutputParameter<DAC8574,float>("CVO-A", dac_output, 0, VALUE_TYPE::UNIPOLAR, true);
                output_b = new CVOutputParameter<DAC8574,float>("CVO-B", dac_output, 1, VALUE_TYPE::UNIPOLAR, true);
                output_c = new CVOutputParameter<DAC8574,float>("CVO-C", dac_output, 2, VALUE_TYPE::UNIPOLAR, true);
                output_d = new CVOutputParameter<DAC8574,float>("CVO-D", dac_output, 3, VALUE_TYPE::UNIPOLAR, true);

                if (this->debug) this->output_a->debug = true;

                output_a->set_parameter_input_for_calibration((VoltageParameterInput*)parameter_manager->getInputForName("A"));
                output_b->set_parameter_input_for_calibration((VoltageParameterInput*)parameter_manager->getInputForName("B"));
                output_c->set_parameter_input_for_calibration((VoltageParameterInput*)parameter_manager->getInputForName("C"));

                // hardwire the LFO sync to the first slot of first output, for testing...
                // TODO: remove this from here (and bake it into configuration instead..)
                output_a->set_slot_input(0, "LFO sync");
                output_a->set_slot_0_amount(1.0);

                this->parameters->add(output_a);
                this->parameters->add(output_b);
                this->parameters->add(output_c);
                this->parameters->add(output_d);
                if (debug && Serial) Serial.println("DeviceBehaviour_CVOutput#init() finished setting up the parameters.");
            } else {
                if (debug && Serial) Serial.printf("WARNING: DeviceBehaviour_CVOutput '%s' has null dac_output!\n", this->label);
                messages_log_add("WARNING: CVOutputBehaviour couldn't initialise DAC output due to nullness!");
            }
        };

        char label[MAX_LABEL_LENGTH] = "CV Output";
        virtual const char *get_label() override {
            return label;
        }

        virtual int getType() override {
            return BehaviourType::virt;
        }

        virtual bool transmits_midi_notes() override {
            return true;
        }

        virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
            // TODO: logic to round-robin the outputs and track notes that are playing etc
            //          unison mode...?
            //          ensure that the note is only sent to the output if it's not already playing
            if (output_a != nullptr) output_a->sendNoteOn(note, velocity, channel);
            if (output_b != nullptr) output_b->sendNoteOn(note, velocity, channel);
            if (output_c != nullptr) output_c->sendNoteOn(note, velocity, channel);
            if (output_d != nullptr) output_d->sendNoteOn(note, velocity, channel);
        }

        virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
            // TODO: logic to round-robin the outputs and track notes that are playing etc
            /*if (output_a != nullptr) output_a->setValue(0);
            if (output_b != nullptr) output_b->setValue(0);
            if (output_c != nullptr) output_c->setValue(0);
            if (output_d != nullptr) output_d->setValue(0);*/
        }

        //BaseParameterInput *pitch_output = nullptr;
        //BaseParameterInput *velocity_output = nullptr;

        /*
        virtual void set_selected_pitch_output(BaseParameterInput *output);
        virtual void set_selected_velocity_output(BaseParameterInput *output);
        
        virtual BaseParameterInput *get_selected_pitch_output() {
            return this->pitch_output;
        }
        virtual BaseParameterInput *get_selected_velocity_output() {
            return this->velocity_output;
        }
        */

        /*void on_pre_clock(unsigned long ticks) override {
            int8_t new_note = NOTE_OFF;
            if (this->pitch_output != nullptr && this->pitch_output->supports_pitch()) {
                VoltageParameterInput *voltage_source_output = (VoltageParameterInput*)this->pitch_output;
                new_note = voltage_source_output->get_voltage_pitch();
                if (this->debug) Serial.printf("setting pitch to %i (%2.2f)\n", new_note, this->pitch_output->get_normal_value_unipolar());
            }

            int velocity = MIDI_MAX_VELOCITY;
            if (this->velocity_output != nullptr) {
                velocity = constrain(((float)MIDI_MAX_VELOCITY)*(float)this->velocity_output->get_normal_value_unipolar(), 0, MIDI_MAX_VELOCITY);
                if (this->debug) Serial.printf("setting velocity to %i (%2.2f)\n", velocity, this->velocity_output->get_normal_value_unipolar());
            }

            // Implement the logic to handle the pitch and velocity output
        }*/

        /*virtual void save_project_add_lines(LinkedList<String> *lines) override {
            if (this->pitch_output != nullptr)
                lines->add(String(F("pitch_output=")) + String(this->pitch_output->name));
            if (this->velocity_output != nullptr)
                lines->add(String(F("velocity_output=")) + String(this->velocity_output->name));
        }*/

        virtual void setup_saveable_parameters() override {
            if (this->saveable_parameters == nullptr)
                DeviceBehaviourUltimateBase::setup_saveable_parameters();

            // Add saveable parameters specific to CVOutputBehaviour
        }

        virtual bool load_parse_key_value(String key, String value) override {
            static const String warning_message = String("WARNING: CVOutputBehaviour couldn't find an output for the name '");
            /*if (key.equals(F("pitch_output"))) {
                BaseParameterInput *source = parameter_manager->getInputForName((char*)value.c_str());
                if (source != nullptr)
                    this->set_selected_pitch_output(source);
                else {
                    this->set_selected_pitch_output(nullptr);
                    messages_log_add(warning_message + value + "'");
                }
                return true;
            } else if (key.equals(F("velocity_output"))) {
                BaseParameterInput *source = parameter_manager->getInputForName((char*)value.c_str());
                if (source != nullptr)
                    this->set_selected_velocity_output(source);
                else {
                    this->set_selected_velocity_output(nullptr);
                    messages_log_add(warning_message + value + "'");
                }
                return true;
            } else
            */
            if (DeviceBehaviourUltimateBase::load_parse_key_value(key, value)) {
                return true;
            }

            return false;
        }

        #ifdef ENABLE_PARAMETERS
            bool already_initialised = false;
            virtual LinkedList<FloatParameter*> *initialise_parameters() override {
                if (already_initialised && this->parameters != nullptr)
                    return this->parameters;

                DeviceBehaviourUltimateBase::initialise_parameters();

                // Add parameters specific to CVOutputBehaviour
                this->init();

                return parameters;
            }
        #endif

        #ifdef ENABLE_SCREEN
            //ParameterInputSelectorControl<DeviceBehaviour_CVOutput> *pitch_parameter_selector = nullptr;
            //ParameterInputSelectorControl<DeviceBehaviour_CVOutput> *velocity_parameter_selector = nullptr;
            //virtual LinkedList<MenuItem *> *make_menu_items() override;
        #endif
};

extern DeviceBehaviour_CVOutput<DAC8574> *behaviour_cvoutput_1;
//extern DeviceBehaviour_CVOutput *behaviour_cvoutput_2;
//extern DeviceBehaviour_CVOutput *behaviour_cvoutput_3;

#endif