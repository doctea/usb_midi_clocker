#pragma once

//#define ENABLE_CV_OUTPUT 0x4C
#include "Config.h"

#if defined(CONFIG_CV_OUTPUT_1) || defined(CONFIG_CV_OUTPUT_2) || defined(CONFIG_CV_OUTPUT_3)
    
#include "behaviour_base.h"
#include "behaviours/behaviour_polyphonic.h"
#include "parameter_inputs/VoltageParameterInput.h"
#include "parameters/CVOutputParameter.h"
#include "ParameterManager.h"

#include "Wire.h"

#include "interfaces/interfaces.h"

#ifdef ENABLE_SCREEN
    #include "submenuitem_bar.h"
    #include "behaviours/cv_output_channel_submenu.h"
#endif

#include "cv_output.h"

#ifdef ENABLE_SCALES
    #include "conductor.h"
#endif

extern ParameterManager *parameter_manager;


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

        // Per-channel note limits (applied in actualSendNoteOn/Off, after global limits)
        // Base values — user-set, saved/loaded directly.
        int8_t per_channel_lowest_note[channel_count]  = { MIDI_MIN_NOTE, MIDI_MIN_NOTE, MIDI_MIN_NOTE, MIDI_MIN_NOTE };
        int8_t per_channel_highest_note[channel_count] = { MIDI_MAX_NOTE, MIDI_MAX_NOTE, MIDI_MAX_NOTE, MIDI_MAX_NOTE };
        NOTE_LIMIT_MODE per_channel_lowest_mode[channel_count]  = { NOTE_LIMIT_MODE::IGNORE, NOTE_LIMIT_MODE::IGNORE, NOTE_LIMIT_MODE::IGNORE, NOTE_LIMIT_MODE::IGNORE };
        NOTE_LIMIT_MODE per_channel_highest_mode[channel_count] = { NOTE_LIMIT_MODE::IGNORE, NOTE_LIMIT_MODE::IGNORE, NOTE_LIMIT_MODE::IGNORE, NOTE_LIMIT_MODE::IGNORE };
        // Effective (post-modulation) note limits — used by apply_per_channel_limits().
        // Initialised equal to base; modulation writes here via ProxyNoteParameter.
        int8_t effective_per_channel_lowest_note[channel_count]  = { MIDI_MIN_NOTE, MIDI_MIN_NOTE, MIDI_MIN_NOTE, MIDI_MIN_NOTE };
        int8_t effective_per_channel_highest_note[channel_count] = { MIDI_MAX_NOTE, MIDI_MAX_NOTE, MIDI_MAX_NOTE, MIDI_MAX_NOTE };
        // Per-channel slew rate base values (user-set/saved); effective slew lives on outputs[i]->effective_slew_rate_normal.
        float slew_base_normal[channel_count] = { 1.0f, 1.0f, 1.0f, 1.0f };

        // UI-only filtered parameter lists (view subsets of this->parameters).
        // per_pool_parameters: pool-level params (populated after base class initialise_parameters()).
        // per_channel_parameters[i]: per-channel params for the channel Mod sub-page.
        LinkedList<FloatParameter*> *per_pool_parameters = new LinkedList<FloatParameter*>();
        LinkedList<FloatParameter*> *per_channel_parameters[channel_count] = { nullptr, nullptr, nullptr, nullptr };

        // Last note played per channel; persists after NoteOff so collapsed sub-menu can show it
        int8_t last_channel_note[channel_count] = { NOTE_OFF, NOTE_OFF, NOTE_OFF, NOTE_OFF };

        GateManager *gate_manager = nullptr;
        int8_t gate_bank = -1, gate_offset = 0;

        DeviceBehaviour_CVOutput(const char *label, uint8_t address, uint8_t dac_extended_address, const char *parameter_label_prefix = "CVO-", TwoWire *wire = &Wire, const cv_channel_range_t *channels = nullptr)
            : DeviceBehaviourUltimateBase() {
            if (label != nullptr)
                strncpy(this->label, label, MAX_LABEL_LENGTH);
            this->dac_output = new DACClass(address, wire);
            this->dac_output->setExtendedAddress(dac_extended_address);

            this->parameter_label_prefix = parameter_label_prefix;
            //this->debug = true;
            this->create_output_parameters(channels);
            this->init();
        }
            
        DeviceBehaviour_CVOutput(const char *label, DACClass *dac_output) : DeviceBehaviourUltimateBase() {
            if (label != nullptr)
                strncpy(this->label, label, MAX_LABEL_LENGTH);
            this->dac_output = dac_output;
            //this->debug = true;
            this->create_output_parameters(nullptr);
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
                if (debug && Serial) Serial.println("DeviceBehaviour_CVOutput telling dac_output to start..");
                Wire.begin();
                dac_output->begin();
                if (debug && Serial) Serial.println("DeviceBehaviour_CVOutput#init() finished.");
            } else {
                if (debug && Serial) Serial.printf("WARNING: DeviceBehaviour_CVOutput '%s' has null dac_output!\n", this->label);
                messages_log_add("WARNING: CVOutputBehaviour couldn't initialise DAC output due to nullness!");
            }

            #ifdef ENABLE_SCALES
                if (conductor != nullptr) {
                    conductor->register_harmony_change_callback([this](const scale_identity_t&, const chord_identity_t&) {
                        for (int i = 0; i < channel_count; i++)
                            park_channel(i);
                    });
                }
            #endif
        };

        char label[MAX_LABEL_LENGTH] = "CV Output";
        virtual const char *get_label() override {
            return label;
        }

        virtual int getType() override {
            return BehaviourType::virt;
        }

        virtual bool transmits_midi_notes() override { return true;}
        virtual bool supports_note_limits() { return true; }

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
                snprintf(parameter_label, MENU_C_MAX, "Ch %c Pitch Bend", (unsigned int)('A' + output_index));
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

                pitch_bend_parameters[output_index]->float_unit = 'c'; // cents
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

        // Apply per-channel note limits to a note, returning NOTE_OFF if dropped or the (possibly transposed) note.
        // Must be called identically in both actualSendNoteOn and actualSendNoteOff for NoteOff symmetry.
        inline int8_t apply_per_channel_limits(uint8_t note, uint8_t ch_idx) {
            return apply_note_limits(
                (int8_t)note,
                per_channel_lowest_mode[ch_idx],   per_channel_highest_mode[ch_idx],
                effective_per_channel_lowest_note[ch_idx], effective_per_channel_highest_note[ch_idx]
            );
        }

        virtual void actualSendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
            if (debug) Serial_printf("DeviceBehaviour_CVOutput#actualSendNoteOn (%i aka %s, %i, %i)\n", note, get_note_name_c(note), velocity, channel);
            if (channel > channel_count) {
                // this shouldn't happen?
                Serial_printf("WARNING: DeviceBehaviour_CVOutput#actualSendNoteOn (%i, %i, %i) got invalid channel!\n", note, velocity, channel);
            } else {
                const int8_t limited_note = apply_per_channel_limits(note, channel - 1);
                if (!is_valid_note(limited_note)) return;
                last_channel_note[channel - 1] = limited_note;
                if (outputs[channel-1] != nullptr) outputs[channel-1]->sendNoteOn(limited_note, velocity, channel);
            }
        }

        virtual void actualSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
            if (debug) Serial_printf("DeviceBehaviour_CVOutput#actualSendNoteOff(%i aka %s, %i, %i)\n", note, get_note_name_c(note), velocity, channel);
            if (channel > channel_count) {
                // this shouldn't happen?
                Serial_printf("WARNING: DeviceBehaviour_CVOutput#actualSendNoteOff(%i, %i, %i) got invalid channel!\n", note, velocity, channel);
            } else {
                // Apply the same per-channel transform as actualSendNoteOn so NoteOff targets the right note.
                // If the note was dropped (IGNORE mode), limited_note == NOTE_OFF; we still forward the
                // original note so CVOutputParameter can safely no-op (current_pitch_note won't match).
                const int8_t limited_note = apply_per_channel_limits(note, channel - 1);
                if (outputs[channel-1] != nullptr)
                    outputs[channel-1]->sendNoteOff(is_valid_note(limited_note) ? limited_note : (int8_t)note, velocity, channel);
                #ifdef ENABLE_SCALES
                    park_channel(channel - 1);
                #endif
            }
        }

        #ifdef ENABLE_SCALES
        void park_channel(int ch_idx) {
            if (ch_idx < 0 || ch_idx >= channel_count) return;
            if (outputs[ch_idx] == nullptr) return;
            if (!outputs[ch_idx]->get_park_enabled()) return;
            if (!allow_voice_for_auto[ch_idx]) return;          // only pool channels
            if (outputs[ch_idx]->is_note_active()) return;      // don't park if note is still sounding
            if (conductor == nullptr) return;
            if (conductor->get_global_quantise_mode() == QUANTISE_MODE_NONE) return;
            const int8_t ref = last_channel_note[ch_idx];
            if (!is_valid_note(ref)) return;
            const int8_t parked = (conductor->get_global_quantise_mode() == QUANTISE_MODE_CHORD)
                ? conductor->quantise_to_chord(ref, 3)
                : conductor->quantise_to_scale(ref);
            outputs[ch_idx]->park_pitch(parked);
            last_channel_note[ch_idx] = parked;
        }
        #endif

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
                        (String("Ch ") + String('A' + i) + " Slew").c_str(), "Slews",
                        &outputs[i]->slew_enabled), SL_SCOPE_SCENE);
            }
            for (int i = 0 ; i < channel_count; i++) {
                if (outputs[i] != nullptr)
                    register_setting(new LSaveableSetting<float>(
                        (String("Ch ") + String('A' + i) + " Slew Rate").c_str(), "Slews", nullptr,
                        [=](float v) { slew_base_normal[i] = v; outputs[i]->effective_slew_rate_normal = v; },
                        [=]() -> float { return slew_base_normal[i]; }
                    ), SL_SCOPE_SCENE);
            }
            for (int i = 0 ; i < channel_count; i++) {
                if (outputs[i] != nullptr)
                    register_setting(new LSaveableSetting<bool>(
                        (String("Ch ") + String('A' + i) + " Gate Output").c_str(), "Gates", nullptr,
                        [=](bool v) { outputs[i]->set_gate_output_enabled(v); },
                        [=]() -> bool { return outputs[i]->get_gate_output_enabled(); }
                    ), SL_SCOPE_SCENE);
            }
            for (int i = 0 ; i < channel_count; i++) {
                register_setting(new LSaveableSetting<int8_t>(
                    (String("Ch ") + String('A' + i) + " Lowest Note").c_str(), "ChLimits", nullptr,
                    [=](int8_t v) { per_channel_lowest_note[i] = v; effective_per_channel_lowest_note[i] = v; },
                    [=]() -> int8_t { return per_channel_lowest_note[i]; }
                ), SL_SCOPE_SCENE);
                register_setting(new LSaveableSetting<int8_t>(
                    (String("Ch ") + String('A' + i) + " Highest Note").c_str(), "ChLimits", nullptr,
                    [=](int8_t v) { per_channel_highest_note[i] = v; effective_per_channel_highest_note[i] = v; },
                    [=]() -> int8_t { return per_channel_highest_note[i]; }
                ), SL_SCOPE_SCENE);
                register_setting(new LSaveableSetting<NOTE_LIMIT_MODE>(
                    (String("Ch ") + String('A' + i) + " Lowest Note Mode").c_str(), "ChLimits", nullptr,
                    [=](NOTE_LIMIT_MODE v) { this->per_channel_lowest_mode[i]  = v; },
                    [=]() -> NOTE_LIMIT_MODE { return this->per_channel_lowest_mode[i];  }
                ), SL_SCOPE_SCENE);
                register_setting(new LSaveableSetting<NOTE_LIMIT_MODE>(
                    (String("Ch ") + String('A' + i) + " Highest Note Mode").c_str(), "ChLimits", nullptr,
                    [=](NOTE_LIMIT_MODE v) { this->per_channel_highest_mode[i] = v; },
                    [=]() -> NOTE_LIMIT_MODE { return this->per_channel_highest_mode[i]; }
                ), SL_SCOPE_SCENE);
            }
        }

        #ifdef ENABLE_PARAMETERS
            bool already_initialised = false;
            virtual LinkedList<FloatParameter*> *initialise_parameters() {
                // Initialise per_channel_parameters lists
                for (int i = 0; i < channel_count; i++) {
                    if (per_channel_parameters[i] == nullptr)
                        per_channel_parameters[i] = new LinkedList<FloatParameter*>();
                }

                // Let base classes add pool-level parameters (e.g. lowest_note/highest_note ProxyNoteParams)
                DeviceBehaviourUltimateBase::initialise_parameters();
                PolyphonicBehaviour::initialise_parameters();

                // Snapshot pool-level params (added by base classes above)
                for (auto* p : *this->parameters)
                    per_pool_parameters->add(p);

                // Add per-channel parameters: CVOutput, slew proxy, note limit proxies, pitch bend
                const char *chan_labels[4] = { "A", "B", "C", "D" };
                for (int i = 0; i < channel_count; i++) {
                    if (outputs[i] == nullptr) continue;

                    // Pitch CV output parameter
                    this->parameters->add(outputs[i]);
                    per_channel_parameters[i]->add(outputs[i]);

                    // Slew rate as a proxy: base in slew_base_normal[i], effective on outputs[i]->effective_slew_rate_normal
                    this->parameters->add(new ProxyParameter<float>(
                        (String("Ch ") + String(chan_labels[i]) + " Slew Rate").c_str(),
                        &slew_base_normal[i],
                        &outputs[i]->effective_slew_rate_normal,
                        0.0f,
                        1.0f
                    ));
                    per_channel_parameters[i]->add(this->parameters->get(this->parameters->size() - 1));

                    // Per-channel lowest note proxy: base in per_channel_lowest_note[i], effective in effective_per_channel_lowest_note[i]
                    this->parameters->add(new ProxyNoteParameter<int8_t>(
                        (String("Ch ") + String(chan_labels[i]) + " Lowest Note").c_str(),
                        &per_channel_lowest_note[i],
                        &effective_per_channel_lowest_note[i]
                    ));
                    per_channel_parameters[i]->add(this->parameters->get(this->parameters->size() - 1));

                    // Per-channel highest note proxy
                    this->parameters->add(new ProxyNoteParameter<int8_t>(
                        (String("Ch ") + String(chan_labels[i]) + " Highest Note").c_str(),
                        &per_channel_highest_note[i],
                        &effective_per_channel_highest_note[i]
                    ));
                    per_channel_parameters[i]->add(this->parameters->get(this->parameters->size() - 1));

                    #if defined(ENABLE_ADVANCED_PITCHBEND)
                        this->ensure_pitch_bend_parameter(i);
                        if (pitch_bend_parameters[i] != nullptr)
                            per_channel_parameters[i]->add(pitch_bend_parameters[i]);
                    #endif
                }

                return parameters;
            }
        #endif

        // Pool-level parameters page is built manually in make_menu_items() using per_pool_parameters.
        // The standard dedicated parameters page (which would show all parameters) is suppressed.
        virtual bool show_dedicated_parameters_page() override {
            return false;
        }

        // CV output channel sub-menus are compact enough that scrolling is not needed.
        virtual bool is_page_scrollable() override { return false; }

        #ifdef ENABLE_SCREEN
            virtual LinkedList<MenuItem*> *make_menu_items() override {
                LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();

                // Initialise shared lowmemory modulation controls once (reused by every channel sub-menu)
                ensure_shared_lowmemory_controls();

                // Pool-level parameters page (global note limits etc.) if any pool params exist
                if (per_pool_parameters != nullptr && per_pool_parameters->size() > 0)
                    menuitems->add(parameter_manager->getModulatableParameterSubMenuItems(nullptr, this->get_label(), per_pool_parameters));

                // One sub-menu per CV output channel containing all per-channel controls
                const char *chan_labels[4] = { "Ch A", "Ch B", "Ch C", "Ch D" };
                for (int i = 0; i < channel_count; i++) {
                    if (outputs[i] == nullptr) continue;
                    char *item_label = new char[strlen(this->get_label()) + strlen(chan_labels[i]) + 2];
                    snprintf(item_label, strlen(this->get_label()) + strlen(chan_labels[i]) + 2, "%s %s", this->get_label(), chan_labels[i]);
                    menuitems->add(new CVOutputChannelSubMenuItem<DACClass>(
                        item_label,
                        outputs[i],
                        &this->allow_voice_for_auto[i],
                        &this->last_channel_note[i],
                        &this->per_channel_lowest_note[i],
                        &this->per_channel_highest_note[i],
                        &this->per_channel_lowest_mode[i],
                        &this->per_channel_highest_mode[i],
                        per_channel_parameters[i],
                        &this->effective_per_channel_lowest_note[i],
                        &this->effective_per_channel_highest_note[i],
                        &this->slew_base_normal[i],
                        true,
                        false
                    ));
                }

                return menuitems;
            }
        #endif

    private:
        void create_output_parameters(const cv_channel_range_t *channels) {
            if (dac_output == nullptr) return;
            const cv_channel_range_t defaults[channel_count] = CV_ALL_UNIPOLAR;
            if (channels == nullptr) channels = defaults;
            const char *chan_labels[4] = { "A", "B", "C", "D" };
            for (int i = 0; i < channel_count; i++) {
                const VALUE_TYPE pol = (channels[i].floor < 0.0f) ? VALUE_TYPE::BIPOLAR : VALUE_TYPE::UNIPOLAR;
                outputs[i] = new CVOutputParameter<DACClass,float>(
                    (String(parameter_label_prefix) + String(chan_labels[i])).c_str(),
                    dac_output, i, pol, channels[i].inverted,
                    channels[i].floor, channels[i].ceil
                );
            }
            if (this->debug && outputs[0] != nullptr) outputs[0]->debug = true;
        }
};

#endif