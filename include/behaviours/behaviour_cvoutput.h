#pragma once

//#define ENABLE_CV_OUTPUT 0x4C
#include "Config.h"

#ifdef ENABLE_CV_OUTPUT

#include "behaviour_base.h"
#include "parameter_inputs/VoltageParameterInput.h"
#include "parameters/CVOutputParameter.h"
#include "ParameterManager.h"


#include "behaviours/behaviour_polyphonic.h"

#include "Wire.h"

extern ParameterManager *parameter_manager;

/*#ifdef ENABLE_SCREEN
    template<class TargetClass>
    class ParameterInputSelectorControl;
#endif*/

template<class DACClass>
class DeviceBehaviour_CVOutput;

template<class DACClass>
class DeviceBehaviour_CVOutput : virtual public DeviceBehaviourUltimateBase, virtual public PolyphonicBehaviour {
    public:
        DACClass *dac_output = nullptr;

        static const int8_t channel_count = 4;
        CVOutputParameter<DACClass> *outputs[channel_count] = { nullptr, nullptr, nullptr, nullptr };

        DeviceBehaviour_CVOutput(const char *label = nullptr, uint8_t address = ENABLE_CV_OUTPUT, uint8_t bank = ENABLE_CV_OUTPUT_BANK, TwoWire *wire = &Wire) 
            : DeviceBehaviourUltimateBase() {
            if (label != nullptr)
                strncpy(this->label, label, MAX_LABEL_LENGTH);
            this->dac_output = new DACClass(address, wire);
            this->dac_output->setExtendedAddress(bank);
            //this->debug = true;
            //this->init();
        }
            
        DeviceBehaviour_CVOutput(const char *label, DACClass *dac_output) : DeviceBehaviourUltimateBase() {
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

                outputs[0] = new CVOutputParameter<DAC8574,float>("CVO-A", dac_output, 0, VALUE_TYPE::UNIPOLAR, true);
                outputs[1] = new CVOutputParameter<DAC8574,float>("CVO-B", dac_output, 1, VALUE_TYPE::UNIPOLAR, true);
                outputs[2] = new CVOutputParameter<DAC8574,float>("CVO-C", dac_output, 2, VALUE_TYPE::UNIPOLAR, true);
                outputs[3] = new CVOutputParameter<DAC8574,float>("CVO-D", dac_output, 3, VALUE_TYPE::UNIPOLAR, true);

                if (this->debug) this->outputs[0]->debug = true;

                outputs[0]->set_parameter_input_for_calibration((VoltageParameterInput*)parameter_manager->getInputForName("A"));
                outputs[1]->set_parameter_input_for_calibration((VoltageParameterInput*)parameter_manager->getInputForName("B"));
                outputs[2]->set_parameter_input_for_calibration((VoltageParameterInput*)parameter_manager->getInputForName("C"));

                // hardwire the LFO sync to the first slot of first output, for testing...
                // TODO: remove this from here (and bake it into configuration instead..)
                outputs[0]->set_slot_input(0, "LFO sync");
                outputs[0]->set_slot_0_amount(1.0);

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
            if (debug) Serial_printf("DeviceBehaviour_CVOutput#actual_sendNoteOn(%i, %i, %i)\n", note, velocity, channel);
            if (channel > channel_count) {
                // this shouldn't happen?
                Serial_printf("WARNING: DeviceBehaviour_CVOutput#actual_sendNoteOn(%i, %i, %i) got invalid channel!\n", note, velocity, channel);
            } else {
                if (outputs[channel-1] != nullptr) outputs[channel-1]->sendNoteOn(note, velocity, channel);
            }
        }

        virtual void actualSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
            if (debug) Serial_printf("DeviceBehaviour_CVOutput#actual_sendNoteOff(%i, %i, %i)\n", note, velocity, channel);
            if (channel > channel_count) {
                // this shouldn't happen?
                Serial_printf("WARNING: DeviceBehaviour_CVOutput#actual_sendNoteOff(%i, %i, %i) got invalid channel!\n", note, velocity, channel);
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
                this->init();

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

                //MIDIBassBehaviour::make_menu_items();

                return menuitems;
            }
        #endif
};

extern DeviceBehaviour_CVOutput<DAC8574> *behaviour_cvoutput_1;
#ifdef ENABLE_CV_OUTPUT_2
    extern DeviceBehaviour_CVOutput<DAC8574> *behaviour_cvoutput_2;
#endif
#ifdef ENABLE_CV_OUTPUT_3
    extern DeviceBehaviour_CVOutput<DAC8574> *behaviour_cvoutput_3;
#endif

#endif