// behaviour for APC40 MKII

#pragma once

#include <Arduino.h>
#include "Config.h"

#ifdef ENABLE_APC40MKII

#ifndef APC40_INPUT_FEATURES_CONFIGURED_EXTERNALLY
    #define ENABLE_APC40_DEVICE_KNOBS
    // Enable these per app need; each group is independent.
    // #define ENABLE_APC40_CLIP_PADS
    // #define ENABLE_APC40_TRACK_BUTTONS
    // #define ENABLE_APC40_GENERAL_BUTTONS
#endif

#ifdef ENABLE_PARAMETERS
    #define APC40_ENABLE_PARAMETERS 1
#endif

#include "behaviours/behaviour_base_usb.h"
#include "usb/multi_usb_handlers.h"
#include "clock.h"
#include "transport_state.h"
#include <APC40Controller.h>
#include <APC40Transport.h>
#include <APC40Constants.h>

// use parameter library adapter to handle cc events
#include "midi/midi_cc_source.h"
#include <APC40ParameterInputs.h>

void apc40mkii_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void apc40mkii_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void apc40mkii_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);


class APC40USBHostTransport : public APC40Transport {
    private:
        MIDIDeviceBase *device = nullptr;

    public:
        void set_device(MIDIDeviceBase *connected_device);
        virtual bool open() override;
        virtual void close() override;
        virtual bool isConnected() const override;
        virtual bool sendMessage(const uint8_t *data, std::size_t size) override;
        bool receiveMidiMessage(uint8_t status, uint8_t channel, uint8_t data1, uint8_t data2);
};

class APC40TempoKnobWrapper {
    public:
    void apply_delta(int16_t delta) {
        set_bpm(get_bpm() + (float)delta);
    }
};

class DeviceBehaviour_APC40MKII : virtual public DeviceBehaviourUSBBase, virtual public MIDI_CC_Source {
    private:
        APC40USBHostTransport apc_transport;
        APC40Controller apc_controller;

        APC40CueKnobParameterInput *cue_knob_input = nullptr;
        APC40LogicalKnobParameterInput *track_knob_inputs[3][8] = {};
#ifdef ENABLE_APC40_DEVICE_KNOBS
        APC40LogicalKnobParameterInput *device_knob_inputs[8][8] = {};
#endif
        APC40TempoKnobWrapper *tempo_knob_input = nullptr;
        APC40ParameterInputMap apc_control_inputs;

        MIDICCParameterInput *track_fader_inputs[8] = {};
        MIDICCParameterInput *master_fader_input = nullptr;
        MIDICCParameterInput *crossfader_input = nullptr;

#ifdef ENABLE_APC40_DEVICE_KNOBS
        void initialize_device_knob_inputs() {
            for (uint8_t bank = 0; bank < 8; ++bank) {
                for (uint8_t knob = 0; knob < 8; ++knob) {
                    this->device_knob_inputs[bank][knob] = new APC40LogicalKnobParameterInput(
                        this->apc_controller, bank, knob
                    );
                    this->addParameterInput(this->device_knob_inputs[bank][knob]);
                    #ifdef ENABLE_PARAMETERS_MIDI_LEARN
                    this->apc_control_inputs.bind(this->device_knob_inputs[bank][knob]->get_control_id(),
                        this->device_knob_inputs[bank][knob]);
                    #endif
                }
            }
        }
#endif

#ifdef APC40_HAS_NOTE_INPUTS
        APC40NoteParameterInput *add_note_parameter_input(APC40ControlId control,
                const char *name, const char *group_name) {
            auto *input = new APC40NoteParameterInput(
                const_cast<char *>(name), group_name, control
            );
            if (!this->apc_control_inputs.bind_note(control, input)) {
                delete input;
                return nullptr;
            }
            this->addParameterInput(input);
            return input;
        }

        void initialize_note_parameter_inputs() {
            #ifdef ENABLE_APC40_CLIP_PADS
            for (uint8_t row = 0; row < 5; ++row) {
                for (uint8_t track = 0; track < 8; ++track) {
                    char name[17];
                    snprintf(name, sizeof(name), "Clip %u.%u",
                        static_cast<unsigned>(row + 1), static_cast<unsigned>(track + 1));
                    const uint8_t note = static_cast<uint8_t>((4 - row) * 8 + track);
                    this->add_note_parameter_input(
                        {APC40ControlKind::Pad, 0, note}, name, "APC40 Clip Pads"
                    );
                }
            }
            #endif

            #ifdef ENABLE_APC40_TRACK_BUTTONS
            static const uint8_t track_button_notes[] = {
                APC40MKII::tracks::BUTTON_ASSIGN_AB,
                APC40MKII::tracks::BUTTON_RECORD,
                APC40MKII::tracks::BUTTON_SOLO,
                APC40MKII::tracks::BUTTON_ACTIVATOR,
                APC40MKII::tracks::BUTTON_SELECTOR,
                APC40MKII::scene::BUTTON_CLIP_STOP
            };
            static const char *const track_button_names[] = {
                "Assign", "Record", "Solo", "Activator", "Select", "Clip Stop"
            };
            constexpr uint8_t track_button_count =
                sizeof(track_button_notes) / sizeof(track_button_notes[0]);
            for (uint8_t track = 0; track < 8; ++track) {
                for (uint8_t button = 0; button < track_button_count; ++button) {
                    char name[17];
                    snprintf(name, sizeof(name), "T%u %s",
                        static_cast<unsigned>(track + 1), track_button_names[button]);
                    this->add_note_parameter_input(
                        {APC40ControlKind::Button, track, track_button_notes[button]},
                        name, "APC40 Track Buttons"
                    );
                }
            }
            #endif

            #ifdef ENABLE_APC40_GENERAL_BUTTONS
            static const uint8_t general_button_notes[] = {
                APC40MKII::device::NOTE_BUTTON_1, APC40MKII::device::NOTE_BUTTON_2,
                APC40MKII::device::NOTE_BUTTON_3, APC40MKII::device::NOTE_BUTTON_4,
                APC40MKII::device::NOTE_BUTTON_5, APC40MKII::device::NOTE_BUTTON_6,
                APC40MKII::device::NOTE_BUTTON_7, APC40MKII::device::NOTE_BUTTON_8,
                APC40MKII::tracks::NOTE_BUTTON_PAN, APC40MKII::tracks::NOTE_BUTTON_SENDS,
                APC40MKII::tracks::NOTE_BUTTON_USER,
                APC40MKII::scene::BUTTON_CLIP_STOP_ALL,
                APC40MKII::scene::SCENE_LAUNCH_ROW_1, APC40MKII::scene::SCENE_LAUNCH_ROW_2,
                APC40MKII::scene::SCENE_LAUNCH_ROW_3, APC40MKII::scene::SCENE_LAUNCH_ROW_4,
                APC40MKII::scene::SCENE_LAUNCH_ROW_5,
                APC40MKII::tempo::NOTE_BUTTON_METRONOME, APC40MKII::tempo::NOTE_BUTTON_TAP_TEMPO,
                APC40MKII::tempo::NOTE_BUTTON_NUDGE_LEFT, APC40MKII::tempo::NOTE_BUTTON_NUDGE_RIGHT,
                APC40MKII::transport::NOTE_BUTTON_PLAY, APC40MKII::transport::NOTE_BUTTON_RECORD,
                APC40MKII::transport::NOTE_BUTTON_SESSION, NOTE_BUTTON_SHIFT,
                APC40MKII::device::NOTE_BUTTON_BANK_SELECT_UP,
                APC40MKII::device::NOTE_BUTTON_BANK_SELECT_DOWN,
                APC40MKII::device::NOTE_BUTTON_BANK_SELECT_RIGHT,
                APC40MKII::device::NOTE_BUTTON_BANK_SELECT_LEFT,
                APC40MKII::device::NOTE_BUTTON_BANK
            };
            constexpr uint8_t general_button_count =
                sizeof(general_button_notes) / sizeof(general_button_notes[0]);
            for (uint8_t index = 0; index < general_button_count; ++index) {
                char name[17];
                snprintf(name, sizeof(name), "Button %u",
                    static_cast<unsigned>(general_button_notes[index]));
                this->add_note_parameter_input(
                    {APC40ControlKind::Button, 0, general_button_notes[index]},
                    name, "APC40 Buttons"
                );
            }
            #endif
        }

        void update_note_parameter_inputs(uint8_t channel, uint8_t note, APC40InputPhase phase) {
            if (channel < 1 || channel > 16)
                return;
            const uint8_t zero_based_channel = channel - 1;
            for (APC40ControlKind kind : {APC40ControlKind::Button, APC40ControlKind::Pad}) {
                auto *input = static_cast<APC40NoteParameterInput *>(this->apc_control_inputs.resolve(
                    {kind, zero_based_channel, note}
                ));
                if (input != nullptr)
                    input->apply(phase);
            }
        }
#endif

        void synchronize_transport_state() {
            this->apc_controller.setPlayState(playing);
            this->apc_controller.setRecordState(transport_state.record_enabled);
        }

        #ifdef ENABLE_PARAMETERS_MIDI_LEARN
            uint32_t midi_learn_revision = 0;

            void synchronize_midi_learn_state() {
                const uint32_t revision = parameter_manager->get_midi_learn_revision();
                if (revision != this->midi_learn_revision) {
                    this->apc_controller.setMIDILearnDeviceState(false);
                    this->midi_learn_revision = revision;
                }
                this->apc_controller.setMIDILearnHostState(
                    parameter_manager->is_midi_learn_active()
                );
            }
        #endif
        
    public:

        uint16_t vid = 0x09e8, pid = 0x0029;
        virtual uint32_t get_packed_id () override { return (this->vid<<16 | this->pid); }

        virtual const char *get_label() override {
            return "APC40MKii";
        }
        virtual bool receives_midi_notes() { return true; }

        bool apc40mkii_shift_held = false;

        std::function<void()> on_session_pressed;

        void set_session_state(bool active) {
            this->apc_controller.setSessionState(active);
        }

        bool get_record_enabled() const {
            return transport_state.record_enabled;
        }

        void set_record_enabled(bool enabled) {
            transport_state.record_enabled = enabled;
            this->apc_controller.setRecordState(enabled);
        }

        APC40LogicalKnobParameterInput *get_track_knob_input(APC40TrackKnobBank bank, uint8_t track) const {
            const uint8_t bank_index = static_cast<uint8_t>(bank);
            return bank_index < 3 && track < 8 ? this->track_knob_inputs[bank_index][track] : nullptr;
        }

        APC40LogicalKnobParameterInput *get_device_knob_input(uint8_t bank, uint8_t knob) const {
#ifdef ENABLE_APC40_DEVICE_KNOBS
            return bank < 8 && knob < 8 ? this->device_knob_inputs[bank][knob] : nullptr;
#else
            (void)bank;
            (void)knob;
            return nullptr;
#endif
        }

        APC40NoteParameterInput *get_note_input(APC40ControlId control) const {
#ifdef APC40_HAS_NOTE_INPUTS
            if (control.kind != APC40ControlKind::Button && control.kind != APC40ControlKind::Pad)
                return nullptr;
            return static_cast<APC40NoteParameterInput *>(this->apc_control_inputs.resolve(control));
#else
            (void)control;
            return nullptr;
#endif
        }

        DeviceBehaviour_APC40MKII() : DeviceBehaviourUSBBase(), apc_controller(apc_transport) {

            this->apc_controller.setup_banks();
            for (uint8_t bank = 0; bank < 3; ++bank) {
                for (uint8_t track = 0; track < 8; ++track) {
                    this->track_knob_inputs[bank][track] = new APC40LogicalKnobParameterInput(
                        this->apc_controller,
                        static_cast<APC40TrackKnobBank>(bank),
                        track
                    );
                    this->addParameterInput(this->track_knob_inputs[bank][track]);
                    #ifdef ENABLE_PARAMETERS_MIDI_LEARN
                        this->apc_control_inputs.bind(this->track_knob_inputs[bank][track]->get_control_id(),
                            this->track_knob_inputs[bank][track]);
                    #endif
                }
            }

            #ifdef ENABLE_APC40_DEVICE_KNOBS
                this->initialize_device_knob_inputs();
            #endif
            #ifdef APC40_HAS_NOTE_INPUTS
                this->initialize_note_parameter_inputs();
            #endif

            for (uint8_t track = 0; track < 8; ++track) {
                this->track_fader_inputs[track] = this->addCCParameterInput(
                    APC40Controller::getTrackFaderName(track),
                    "APC40 Faders",
                    (byte)APC40MKII::tracks::CC_FADER,
                    (byte)(track + 1)
                );
                #ifdef ENABLE_PARAMETERS_MIDI_LEARN
                    this->apc_control_inputs.bind({APC40ControlKind::TrackFader, 0, track},
                        this->track_fader_inputs[track]);
                #endif
            }

            // Master fader.  normal cc input, handled as a regular MIDI CC input via midi_cc_source
            this->master_fader_input = this->addCCParameterInput(
                "MasterFader", 
                "APC40 Faders", 
                (byte)APC40MKII::tracks::CC_FADER_MASTER,
                (byte)APC40MKII::tracks::CHAN_FADER_MASTER
            );
                        
            // Cue knob.  logical input, handled via the APC40MKII library
            this->cue_knob_input = new APC40CueKnobParameterInput(
                (char *)"CueKnob", 
                "APC40MKII"
            );
            this->addParameterInput(this->cue_knob_input);

            // Crossfader. regular input, handled as a normal MIDI CC input via midi_cc_source
            // todo: make this output bipolar by default..
            // think will need to implement bipolar mode for MIDICCParameterInput inputs,
            // or maybe a new class for that
            this->crossfader_input = this->addCCParameterInput(
                "Crossfader", 
                "APC40MKII", 
                (byte)APC40MKII::CC_CROSSFADER,
                (byte)APC40MKII::CHAN_CROSSFADER
            );
            #ifdef ENABLE_PARAMETERS_MIDI_LEARN
                this->apc_control_inputs.bind({APC40ControlKind::MasterFader, 0, 0}, this->master_fader_input);
                this->apc_control_inputs.bind({APC40ControlKind::Crossfader, 0, 0}, this->crossfader_input);
                this->apc_control_inputs.bind({APC40ControlKind::CueEncoder, 0, 0}, this->cue_knob_input);
            #endif

            // Tempo knob - uses deltas, directly bound to the APC40MKII tempo encoder.
            // Wrapper calls the set_bpm adjustment. logical input handled via the APC40MKII library
            this->tempo_knob_input = new APC40TempoKnobWrapper();

            // todo: shift support and remaining APC40 controls...
        }

        FLASHMEM 
        virtual void setup_callbacks() override {
            //behaviour_apc40mkii_instance = this;
            if (!is_connected()) return;

            this->device->setHandleControlChange(apc40mkii_control_change);
            this->device->setHandleNoteOn(apc40mkii_note_on);
            this->device->setHandleNoteOff(apc40mkii_note_off);
        };

        virtual void send_sysex_command(byte message_identifier, int16_t size, const byte *data) {
            this->apc_controller.device.send_sysex_command(message_identifier, size, data);
        }

        FLASHMEM
        virtual void init() override {
            DeviceBehaviourUSBBase::init();

            this->apc_controller.onPlayPressed = [this] {
                if (playing)
                    clock_stop();
                else
                    clock_continue();
                this->apc_controller.setPlayState(playing);
            };
            this->apc_controller.onRecordPressed = [this] {
                this->set_record_enabled(!transport_state.record_enabled);
            };
            this->apc_controller.onSessionPressed = [this] {
                #ifdef ENABLE_PARAMETERS_MIDI_LEARN
                    this->synchronize_midi_learn_state();
                    this->apc_controller.setMIDILearnDeviceState(!this->apc_controller.midiLearnDeviceState);
                #endif
                if (this->on_session_pressed)
                    this->on_session_pressed();
            };
            #ifdef ENABLE_PARAMETERS_MIDI_LEARN
                this->apc_controller.setMillisProvider([] { return millis(); });
                this->apc_controller.onLearnCandidate = [this](const APC40ControlEvent& event) {
                    const bool assigned = parameter_manager->propose_midi_learn(
                        this->apc_control_inputs.resolve(event.control)
                    );
                    this->synchronize_midi_learn_state();
                    return assigned;
                };
                this->synchronize_midi_learn_state();
            #endif

            for (auto &bank_inputs : this->track_knob_inputs) {
                // TODO: why do we do this?
                // Loop through each input in the bank and call its loop method.
                // This ensures that any internal state or processing for the input is updated regularly.
                for (auto *input : bank_inputs) {
                    input->loop();
                }
            }
            #ifdef ENABLE_APC40_DEVICE_KNOBS
                for (auto &bank_inputs : this->device_knob_inputs) {
                    for (auto *input : bank_inputs) {
                        input->loop();
                    }
                }
            #endif

            this->apc_controller.onCueEncoderDelta = [this](int16_t delta) {
                this->cue_knob_input->apply_delta(delta);
            };
            this->apc_controller.onTempoEncoderDelta = [this](int16_t delta) {
                this->tempo_knob_input->apply_delta(delta);
            };

            this->apc_controller.onTrackFaderChanged = [this](uint8_t track, uint8_t value) {
                if (track < 8 && this->track_fader_inputs[track] != nullptr) {
                    this->track_fader_inputs[track]->receive_control_change(
                        APC40MKII::tracks::CC_FADER, 
                        value,
                        track + 1
                    );
                }
            };

            this->synchronize_transport_state();
            this->apc_transport.set_device(this->device);
            this->apc_controller.init();
        }

        virtual void loop(uint32_t ticks) override {
            DeviceBehaviourUSBBase::loop(ticks);
            this->synchronize_transport_state();
            #ifdef ENABLE_PARAMETERS_MIDI_LEARN
                this->synchronize_midi_learn_state();
            #endif
            this->apc_controller.loop();
        }

        virtual void disconnect_device() override {
    #ifdef APC40_HAS_NOTE_INPUTS
            this->apc_control_inputs.release_note_inputs();
#endif
            this->apc_transport.set_device(nullptr);
            DeviceBehaviourUSBBase::disconnect_device();
        }

        virtual void receive_note_on(byte inChannel, byte inNumber, byte inVelocity) override {
            #ifdef ENABLE_PARAMETERS_MIDI_LEARN
                // TODO: not sure we actually need this here?
                this->synchronize_midi_learn_state();
            #endif
            this->apc_transport.receiveMidiMessage(0x90, inChannel, inNumber, inVelocity);
#ifdef APC40_HAS_NOTE_INPUTS
            this->update_note_parameter_inputs(inChannel, inNumber,
                inVelocity == 0 ? APC40InputPhase::Release : APC40InputPhase::Press);
#endif
            Debug_printf(F("apc40mkii_note_on for %i, %i, %i\n"), inChannel, inNumber, inVelocity);

            // if (inNumber==APC40_BUTTON_SHIFT) {
            //     apc40mkii_shift_held = true;            
            //     Serial.print("APC40MKII Shift button pressed\n");
            // }
            if (Serial) Serial.printf("APC40MKII Note On received: channel=%d, number=%d, value=%d\n", inChannel, inNumber, inVelocity);
        }

        virtual void receive_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) override {
            this->apc_transport.receiveMidiMessage(0x80, inChannel, inNumber, inVelocity);
#ifdef APC40_HAS_NOTE_INPUTS
            this->update_note_parameter_inputs(inChannel, inNumber, APC40InputPhase::Release);
#endif
            // if (inNumber==APC40_BUTTON_SHIFT) {
            //     apc40mkii_shift_held = false;
            //     Serial.print("APC40MKII Shift button released\n");
            // }
            if (Serial) Serial.printf("APC40MKII Note Off received: channel=%d, number=%d, value=%d\n", inChannel, inNumber, inVelocity);
        }

        virtual void receive_control_change(uint8_t channel, uint8_t number, uint8_t value) override {
            if (Serial) Serial.printf("APC40MKII CC received: channel=%d, number=%d, value=%d\n", channel, number, value);
            #ifdef ENABLE_PARAMETERS_MIDI_LEARN
                // @@TODO: not sure we need this or here either?
                this->synchronize_midi_learn_state();
            #endif

            // so this will pick up the actual messages sent from the hardware, then sends to the 
            // APC40MKII transport to handle things like banked encoders.
            // the apc40mkii will then process the message accordingly and update the relevant 
            // APC40LogicalKnobParameterInput controls directly..

            // pass anything not handled by the APC40MKII transport to the regular MIDI CC input chain
            if (!this->apc_transport.receiveMidiMessage(0xB0, channel, number, value)) {
                MIDI_CC_Source::update_parameter_inputs_cc(number, value, channel);
            }
        }
};

extern DeviceBehaviour_APC40MKII *behaviour_apc40mkii;

#endif