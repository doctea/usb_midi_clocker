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

        #if defined(ENABLE_PARAMETERS)
            LDataParameter<float> *pitch_bend_parameters[channel_count] = { nullptr, nullptr, nullptr, nullptr };
        #endif

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

                /*this->parameters->add(outputs[0]);
                this->parameters->add(outputs[1]);
                this->parameters->add(outputs[2]);
                this->parameters->add(outputs[3]);
                */
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

        virtual PitchBendSupport get_pitch_bend_support() const override {
            #if defined(ENABLE_ADVANCED_PITCHBEND) && defined(ENABLE_PARAMETERS)
                return PitchBendSupport::MODULATED;
            #else
                return PitchBendSupport::PASSTHRU;
            #endif
        }

        virtual void apply_omni_pitch_bend_to_allowed_outputs(float semitones) {
            // Omni bend intentionally targets only outputs in the auto-voice pool.
            // Manually pinned voices keep dedicated/channel bend behavior only.
            for (uint8_t i = 0; i < channel_count; i++) {
                if (outputs[i] == nullptr)
                    continue;

                // Omni pitch bend should only affect outputs that are in the auto-voice pool.
                outputs[i]->set_omni_pitch_bend_semitones(this->allow_voice_for_auto[i] ? semitones : 0.0f);
                #if CV_PITCH_BEND_TRACE
                    Serial_printf("CVBEND omni idx=%u auto=%u semi=%0.3f\n", i, this->allow_voice_for_auto[i] ? 1 : 0, semitones);
                #endif
            }
        }

        virtual void emit_effective_pitch_bend_from_semitones(float semitones, uint8_t channel) override {
            (void)channel;
            apply_omni_pitch_bend_to_allowed_outputs(semitones);
        }

        #if defined(ENABLE_ADVANCED_PITCHBEND) && defined(ENABLE_PARAMETERS)
            virtual bool handle_modulated_pitch_bend(uint8_t inChannel, int bend) override {
                if (!this->supports_advanced_pitch_bend() || !this->transmits_midi_notes())
                    return false;

                this->last_received_pitch_bend = (int16_t)bend;
                this->last_received_pitch_bend_semitones = this->pitch_bend_to_semitones((int16_t)bend);

                if (inChannel == 0) {
                    // Keep the behaviour-level pitch bend parameter as the omni source.
                    this->ensure_advanced_pitch_bend_parameter();
                    this->current_channel = 0;
                    #if CV_PITCH_BEND_TRACE
                        Serial_printf("CVBEND advanced omni in bend=%d semi=%0.3f\n", bend, this->last_received_pitch_bend_semitones);
                    #endif

                    if (this->pitch_bend_parameter != nullptr) {
                        this->pitch_bend_parameter->updateValueFromData(this->last_received_pitch_bend_semitones);
                    } else {
                        this->apply_omni_pitch_bend_to_allowed_outputs(this->last_received_pitch_bend_semitones);
                    }
                    return true;
                }

                if (inChannel > channel_count)
                    return false;

                #if CV_PITCH_BEND_TRACE
                    Serial_printf("CVBEND advanced ch=%u bend=%d semi=%0.3f\n", inChannel, bend, this->last_received_pitch_bend_semitones);
                #endif

                this->ensure_pitch_bend_parameter(inChannel - 1);
                if (pitch_bend_parameters[inChannel - 1] != nullptr) {
                    pitch_bend_parameters[inChannel - 1]->updateValueFromData(this->last_received_pitch_bend_semitones);
                } else {
                    this->apply_pitch_bend_to_output(inChannel - 1, (int16_t)bend);
                }
                return true;
            }
        #endif

        #if defined(ENABLE_PARAMETERS)
            virtual void ensure_pitch_bend_parameter(uint8_t output_index) {
                if (output_index >= channel_count)
                    return;
                if (pitch_bend_parameters[output_index] != nullptr)
                    return;

                char parameter_label[MENU_C_MAX] = "";
                snprintf(parameter_label, MENU_C_MAX, "Pitch Bend %u", (unsigned int)(output_index + 1));
                const int8_t range = this->get_pitch_bend_range_semitones();

                pitch_bend_parameters[output_index] = new LDataParameter<float>(
                    parameter_label,
                    [=](float semitones) -> void {
                        if (outputs[output_index] != nullptr)
                            outputs[output_index]->set_channel_pitch_bend_semitones(semitones);
                    },
                    [=]() -> float {
                        if (outputs[output_index] != nullptr)
                            return outputs[output_index]->get_channel_pitch_bend_semitones();
                        return 0.0f;
                    },
                    (float)(-range),
                    (float)(range)
                );
                for (uint8_t i = 0; i < MAX_SLOT_CONNECTIONS; i++) {
                    pitch_bend_parameters[output_index]->connections[i].polar_mode = MOD_SLOT_UNI_CENTERED;
                }

                this->parameters->add(pitch_bend_parameters[output_index]);
            }
        #endif

        virtual void apply_pitch_bend_to_output(uint8_t output_index, int16_t bend) {
            if (output_index >= channel_count)
                return;

            float bend_semitones = this->pitch_bend_to_semitones(bend);

            #if defined(ENABLE_PARAMETERS)
                if (pitch_bend_parameters[output_index] != nullptr) {
                    pitch_bend_parameters[output_index]->updateValueFromData(bend_semitones);
                    return;
                }
            #endif

            if (outputs[output_index] != nullptr)
                outputs[output_index]->set_channel_pitch_bend_semitones(bend_semitones);
        }

        virtual void actualSendPitchBend(int16_t bend, uint8_t channel) override {
            if (channel == 0) {
                apply_omni_pitch_bend_to_allowed_outputs(this->pitch_bend_to_semitones(bend));
                return;
            }

            if (channel > channel_count) {
                if (debug) {
                    Serial_printf("WARNING: DeviceBehaviour_CVOutput#actualSendPitchBend(%i, %i) got invalid channel!\n", bend, channel);
                }
                return;
            }

            apply_pitch_bend_to_output(channel - 1, bend);
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
        using PolyphonicBehaviour::killCurrentNote;

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

        virtual void setup_saveable_settings() override {
            DeviceBehaviourUltimateBase::setup_saveable_settings();
            PolyphonicBehaviour::setup_saveable_settings();

            // todo: save velocity output and pitch output? actually, seems like that already didn't exist in main branch
            // todo: so, just investigate whether we actually need it and document how it connects to what we have here

            for (int i = 0 ; i < channel_count; i++) {
                if (outputs[i] != nullptr)
                    register_setting(new VarSetting<bool>(
                        (String("Slew ") + String(i)).c_str(), "Slews",
                        &outputs[i]->slew_enabled)
                    , SL_SCOPE_SCENE);
            }
            for (int i = 0 ; i < channel_count; i++) {
                if (outputs[i] != nullptr)
                    register_setting(new LSaveableSetting<float>(
                        (String("Slew Rate ") + String(i)).c_str(), "Slews", nullptr,
                        [=](float v) { outputs[i]->set_slew_rate_normal(v); },
                        [=]() -> float { return outputs[i]->get_slew_rate_normal(); }
                    ), SL_SCOPE_SCENE);
            }
            for (int i = 0 ; i < channel_count; i++) {
                if (outputs[i] != nullptr)
                    register_setting(new LSaveableSetting<bool>(
                        (String("Gate Output ") + String(i)).c_str(), "Gates", nullptr,
                        [=](bool v) { outputs[i]->set_gate_output_enabled(v); },
                        [=]() -> bool { return outputs[i]->get_gate_output_enabled(); }
                    ), SL_SCOPE_SCENE);
            }
        }

        #ifdef ENABLE_PARAMETERS
            bool already_initialised = false;
            virtual LinkedList<FloatParameter*> *initialise_parameters() {
                //if (already_initialised && this->parameters != nullptr)
                //    return this->parameters;
                //already_initialised = true;
                /*while (1) {
                    Serial.println("DeviceBehaviour_CVOutput#initialise_parameters()..");
                }*/

                DeviceBehaviourUltimateBase::initialise_parameters();
                PolyphonicBehaviour::initialise_parameters();

                this->parameters->add(outputs[0]);
                this->parameters->add(outputs[1]);
                this->parameters->add(outputs[2]);
                this->parameters->add(outputs[3]);

                // Add parameters specific to CVOutputBehaviour
                //this->init();

                // add parameters for controlling the slew rate of the outputs
                for (int i = 0 ; i < channel_count; i++) {
                    if (outputs[i] != nullptr) {
                        this->parameters->add(new LDataParameter<float>(
                            (String("Slew Rate ") + String(/*'A'+*/i)).c_str(),
                            [=](float v) -> void { outputs[i]->set_slew_rate_normal(v); },
                            [=]() -> float { return outputs[i]->get_slew_rate_normal(); },
                            0.0, 
                            1.0
                        ));

                        #if defined(ENABLE_ADVANCED_PITCHBEND)
                            this->ensure_pitch_bend_parameter(i);
                        #endif
                    }
                }

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

                // make controls for enabling/disabling gate outputs
                SubMenuItemBar *gate_output_menu = new SubMenuItemBar("Gate Outputs", true, true);
                for (int i=0; i<channel_count; i++) {
                    if (outputs[i] != nullptr) {
                        gate_output_menu->add(new LambdaToggleControl(
                            (String("Output ") + String(i+1)).c_str(),
                            [=](bool v) -> void { outputs[i]->set_gate_output_enabled(v); },
                            [=]() -> bool { return outputs[i]->get_gate_output_enabled(); }
                        ));
                    }
                }
                menuitems->add(gate_output_menu);

                // add a Slew separator
                menuitems->add(new SeparatorMenuItem("Slew Controls"));

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

                // make slew controls to set the slew rate for the outputs
                SubMenuItemBar *slew_rate_menu = new SubMenuItemBar("Slew Rate", true, true);
                for (int i=0; i<channel_count; i++) {
                    if (outputs[i] != nullptr) {
                        slew_rate_menu->add(new LambdaNumberControl<float>(
                            //(String("Output ") + String(i+1)).c_str(),
                            outputs[i]->label,
                            [=](float v) -> void { outputs[i]->set_slew_rate_normal(v); },
                            [=]() -> float { return outputs[i]->get_slew_rate_normal(); },
                            nullptr,
                            0.0, 
                            1.0, 
                            0.01,
                            true
                        ));
                    }
                }
                menuitems->add(slew_rate_menu);

                //MIDIBassBehaviour::make_menu_items();

                return menuitems;
            }
        #endif
};

#endif