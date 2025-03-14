#pragma once

//#define ENABLE_CV_OUTPUT 0x4C
#include "Config.h"

#if defined(CONFIG_CV_OUTPUT_1) || defined(CONFIG_CV_OUTPUT_2) || defined(CONFIG_CV_OUTPUT_3)
    
#include "behaviour_base.h"
#include "parameter_inputs/VoltageParameterInput.h"
#include "parameters/CVOutputParameter.h"
#include "ParameterManager.h"

#include "behaviours/behaviour_polyphonic.h"

#include "Wire.h"

#include "interfaces/interfaces.h"

#include "submenuitem_bar.h"

#include "cv_output.h"

extern ParameterManager *parameter_manager;

/*#ifdef ENABLE_SCREEN
    template<class TargetClass>
    class ParameterInputSelectorControl;
#endif*/

// todo: make CVOutputParameter more agnostic about underlying library so it can support other DACs
// todo: including a 'dummy' DAC for testing purposes
// todo: and a generic base class for CVOutputParameters of different types to inherit from so that 
//       cvoutput_config_t (in cv_output.h) doesn't have to know the DAC type

template<class DACClass>
class DeviceBehaviour_CVOutput : virtual public DeviceBehaviourUltimateBase, virtual public PolyphonicBehaviour {
    public:
        DACClass *dac_output = nullptr;

        static const int8_t channel_count = 4;
        CVOutputParameter<DACClass> *outputs[channel_count] = { nullptr, nullptr, nullptr, nullptr };

        const char *parameter_label_prefix = "CVO-";

        GateManager *gate_manager = nullptr;
        int8_t gate_bank = -1, gate_offset = 0;

        DeviceBehaviour_CVOutput(const char *label, uint8_t address, uint8_t dac_extended_address, const char *parameter_label_prefix = "CVO-", TwoWire *wire = &Wire)
            : DeviceBehaviourUltimateBase() {
            if (label != nullptr)
                strncpy(this->label, label, MAX_LABEL_LENGTH);
            this->dac_output = new DACClass(address, wire);
            this->dac_output->setExtendedAddress(dac_extended_address);

            this->parameter_label_prefix = parameter_label_prefix;
            //this->debug = true;
            this->init();
        }
            
        DeviceBehaviour_CVOutput(const char *label, DACClass *dac_output) : DeviceBehaviourUltimateBase() {
            if (label != nullptr)
                strncpy(this->label, label, MAX_LABEL_LENGTH);
            this->dac_output = dac_output;
            //this->debug = true;
            this->init();
        }

        virtual void set_gate_outputter(GateManager *gate_manager, int8_t gate_bank = 0, int8_t gate_offset = 0) {
            this->gate_manager = gate_manager;
            this->gate_bank = gate_bank;
            this->gate_offset = gate_offset;
            for (int i=0; i<channel_count; i++) {
                if (outputs[i] != nullptr) outputs[i]->set_gate_outputter(gate_manager, gate_bank, gate_offset + i);
            }
        }

        virtual void set_calibration_parameter_input(int channel, VoltageParameterInput *input) {
            if (channel < channel_count) {
                if (outputs[channel] != nullptr) outputs[channel]->set_calibration_parameter_input(input);
            }
        }

        virtual void set_calibration_parameter_input(int channel, const char *input_name) {
            Serial.printf("DeviceBehaviour_CVOutput#set_calibration_parameter_input(%i, %s)\n", channel, input_name);
            if (channel < channel_count) {
                if (outputs[channel] != nullptr) outputs[channel]->set_calibration_parameter_input(input_name);
            }
        }

        virtual void init() override {
            DeviceBehaviourUltimateBase::init();

            if (debug && Serial) 
                Serial.println("DeviceBehaviour_CVOutput#init()..");

            if (dac_output!=nullptr) {
                if (debug && Serial) Serial.println("DeviceBehaviour_CVOutput telling dac_output to start and setting up the CVOutputParameters..");
                Wire.begin();
                dac_output->begin();

                outputs[0] = new CVOutputParameter<DAC8574,float>((String(parameter_label_prefix)+String("A")).c_str(), dac_output, 0, VALUE_TYPE::UNIPOLAR, true);
                outputs[1] = new CVOutputParameter<DAC8574,float>((String(parameter_label_prefix)+String("B")).c_str(), dac_output, 1, VALUE_TYPE::UNIPOLAR, true);
                outputs[2] = new CVOutputParameter<DAC8574,float>((String(parameter_label_prefix)+String("C")).c_str(), dac_output, 2, VALUE_TYPE::UNIPOLAR, true);
                outputs[3] = new CVOutputParameter<DAC8574,float>((String(parameter_label_prefix)+String("D")).c_str(), dac_output, 3, VALUE_TYPE::UNIPOLAR, true);

                if (this->debug) this->outputs[0]->debug = true;

                // hardwire the LFO sync to the first slot of first output, for testing...
                // TODO: remove this from here (and bake it into configuration/sequence/project saves instead..)
                // hmmm, so, currently init() is called before the parameterinputs are created, so this doesn't work
                // BUT we need the outputs set up before set_calibration_parameter_input() is called 
                //outputs[0]->set_slot_input(0, "LFO sync");
                //outputs[0]->set_slot_0_amount(1.0);

                this->parameters->add(outputs[0]);
                this->parameters->add(outputs[1]);
                this->parameters->add(outputs[2]);
                this->parameters->add(outputs[3]);
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

        virtual void actualSendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
            if (debug) Serial_printf("DeviceBehaviour_CVOutput#actualSendNoteOn (%i aka %s, %i, %i)\n", note, get_note_name_c(note), velocity, channel);
            if (channel > channel_count) {
                // this shouldn't happen?
                Serial_printf("WARNING: DeviceBehaviour_CVOutput#actualSendNoteOn (%i, %i, %i) got invalid channel!\n", note, velocity, channel);
            } else {
                if (outputs[channel-1] != nullptr) outputs[channel-1]->sendNoteOn(note, velocity, channel);
            }
        }

        virtual void actualSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
            if (debug) Serial_printf("DeviceBehaviour_CVOutput#actualSendNoteOff(%i aka %s, %i, %i)\n", note, get_note_name_c(note), velocity, channel);
            if (channel > channel_count) {
                // this shouldn't happen?
                Serial_printf("WARNING: DeviceBehaviour_CVOutput#actualSendNoteOff(%i, %i, %i) got invalid channel!\n", note, velocity, channel);
            } else {
                if (outputs[channel-1] != nullptr) outputs[channel-1]->sendNoteOff(note, velocity, channel);
            }
        }

        using PolyphonicBehaviour::sendNoteOn;
        using PolyphonicBehaviour::sendNoteOff;

        /*virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
            // TODO: logic to round-robin the outputs and track notes that are playing etc
            //          unison mode...?
            //          ensure that the note is only sent to the output if it's not already playing
            if (outputs[channel-1] != nullptr) outputs[channel-1]->sendNoteOn(note, velocity, channel);
            //if (outputs[1] != nullptr) outputs[1]->sendNoteOn(note, velocity, channel);
            //if (output_c != nullptr) output_c->sendNoteOn(note, velocity, channel);
            //if (output_d != nullptr) output_d->sendNoteOn(note, velocity, channel);
        }

        virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
            // TODO: logic to round-robin the outputs and track notes that are playing etc
        }*/

        virtual void setup_saveable_parameters() override {
            if (this->saveable_parameters == nullptr)
                DeviceBehaviourUltimateBase::setup_saveable_parameters();

            PolyphonicBehaviour::setup_saveable_parameters();

            // Add saveable parameters specific to CVOutputBehaviour
            for (int i = 0 ; i < channel_count; i++) {
                if (outputs[i] != nullptr) {
                    this->saveable_parameters->add(new LSaveableParameter<bool>(
                        (String("Slew ") + String(/*'A'+*/i)).c_str(),
                        "Slews",
                        &outputs[i]->slew_enabled
                    ));
                }
            }
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
            if (PolyphonicBehaviour::load_parse_key_value(key, value)) {
                return true;
            }

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

                PolyphonicBehaviour::initialise_parameters();

                // Add parameters specific to CVOutputBehaviour
                //this->init();

                return parameters;
            }
        #endif

        #ifdef ENABLE_SCREEN
            //ParameterInputSelectorControl<DeviceBehaviour_CVOutput> *pitch_parameter_selector = nullptr;
            //ParameterInputSelectorControl<DeviceBehaviour_CVOutput> *velocity_parameter_selector = nullptr;
            //virtual LinkedList<MenuItem *> *make_menu_items() override;
            virtual LinkedList<MenuItem*> *make_menu_items() override {
                LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();

                PolyphonicBehaviour::make_menu_items();

                // make slew controls for the outputs
                SubMenuItemBar *slew_menu = new SubMenuItemBar("Slew", true, true);
                for (int i=0; i<channel_count; i++) {
                    if (outputs[i] != nullptr) {
                        slew_menu->add(new LambdaToggleControl(
                            (String("Output ") + String(i+1)).c_str(),
                            [=](bool v) -> void { outputs[i]->slew_enabled = v; },
                            [=]() -> bool { return outputs[i]->slew_enabled; }
                        ));
                    }
                }
                menuitems->add(slew_menu);

                //MIDIBassBehaviour::make_menu_items();

                return menuitems;
            }
        #endif
};

#endif