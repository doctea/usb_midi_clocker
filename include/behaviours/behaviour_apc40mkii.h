// behaviour for APC40 MKII

#pragma once

#include <Arduino.h>
#include "Config.h"

#ifdef ENABLE_APC40MKII

#include "behaviours/behaviour_base_usb.h"
#include "usb/multi_usb_handlers.h"
#include "clock.h"
#include "transport_state.h"
#include <APC40Controller.h>
#include <APC40Transport.h>
#include <APC40Constants.h>

// use parameter library adapter to handle cc events
#include "midi/midi_cc_source.h"
#define APC40_ENABLE_PARAMETERS 1
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
        APC40TempoKnobWrapper *tempo_knob_input = nullptr;

        MIDICCParameterInput *track_fader_inputs[8] = {};
        MIDICCParameterInput *master_fader_input = nullptr;
        MIDICCParameterInput *crossfader_input = nullptr;

        void synchronize_transport_state() {
            this->apc_controller.setPlayState(playing);
            this->apc_controller.setRecordState(transport_state.record_enabled);
        }

        #ifdef ENABLE_PARAMETERS_MIDI_LEARN
            APC40ParameterInputMap midi_learn_inputs;
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

        DeviceBehaviour_APC40MKII() : DeviceBehaviourUSBBase(), apc_controller(apc_transport) {

            // set up the track fader banks
            this->apc_controller.setup_banks();
            // initialise the CCs that this device can translate into ParameterInputs
            // processed as logical knob inputs so that the APC40MKII library can correctly bank them
            for (uint8_t bank = 0; bank < 3; ++bank) {
                for (uint8_t track = 0; track < 8; ++track) {
                    this->track_knob_inputs[bank][track] = new APC40LogicalKnobParameterInput(
                        this->apc_controller,
                        static_cast<APC40TrackKnobBank>(bank), 
                        track
                    );
                    this->addParameterInput(this->track_knob_inputs[bank][track]);
                    #ifdef ENABLE_PARAMETERS_MIDI_LEARN
                        this->midi_learn_inputs.bind(this->track_knob_inputs[bank][track]->get_control_id(),
                            this->track_knob_inputs[bank][track]);
                    #endif
                }
            }
            
            // initialise the track fader inputs
            // added as normal midi CC inputs BUT they actually get updated via the
            // APC40MKII library with the callback onTrackFaderChanged so that the MIDI channel is 
            // irrelevant for the actual fader value updates.
            for (uint8_t track = 0; track < 8; ++track) {
                this->track_fader_inputs[track] = this->addCCParameterInput(
                    APC40Controller::getTrackFaderName(track),
                    "APC40 Faders",
                    (byte)APC40MKII::tracks::CC_FADER, 
                    (byte)(track + 1)
                );
                #ifdef ENABLE_PARAMETERS_MIDI_LEARN
                    this->midi_learn_inputs.bind({APC40ControlKind::TrackFader, 0, track},
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
                this->midi_learn_inputs.bind({APC40ControlKind::MasterFader, 0, 0}, this->master_fader_input);
                this->midi_learn_inputs.bind({APC40ControlKind::Crossfader, 0, 0}, this->crossfader_input);
                this->midi_learn_inputs.bind({APC40ControlKind::CueEncoder, 0, 0}, this->cue_knob_input);
            #endif

            // Tempo knob - uses deltas, directly bound to the APC40MKII tempo encoder.
            // Wrapper calls the set_bpm adjustment. logical input handled via the APC40MKII library
            this->tempo_knob_input = new APC40TempoKnobWrapper();

            // todo: add support for other APC40MKII controls
            // todo: shift support?
            // todo: device knobs and all the rest...
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
                        this->midi_learn_inputs.resolve(event.control));
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
            this->apc_transport.set_device(nullptr);
            DeviceBehaviourUSBBase::disconnect_device();
        }

        virtual void receive_note_on(byte inChannel, byte inNumber, byte inVelocity) override {
            #ifdef ENABLE_PARAMETERS_MIDI_LEARN
                // TODO: not sure we actually need this here?
                this->synchronize_midi_learn_state();
            #endif
            this->apc_transport.receiveMidiMessage(0x90, inChannel, inNumber, inVelocity);
            Debug_printf(F("apc40mkii_note_on for %i, %i, %i\n"), inChannel, inNumber, inVelocity);

            // if (inNumber==APC40_BUTTON_SHIFT) {
            //     apc40mkii_shift_held = true;            
            //     Serial.print("APC40MKII Shift button pressed\n");
            // }
            if (Serial) Serial.printf("APC40MKII Note On received: channel=%d, number=%d, value=%d\n", inChannel, inNumber, inVelocity);
        }

        virtual void receive_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) override {
            this->apc_transport.receiveMidiMessage(0x80, inChannel, inNumber, inVelocity);
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