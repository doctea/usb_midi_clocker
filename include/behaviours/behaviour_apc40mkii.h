// behaviour for APC40 MKII

#pragma once

#include <Arduino.h>
#include "Config.h"

#ifdef ENABLE_APC40MKII

#include "behaviours/behaviour_base_usb.h"
#include "usb/multi_usb_handlers.h"
#include <APC40Controller.h>
#include <APC40Transport.h>
#include <APC40Constants.h>

// use parameter library adapter to handle cc events
#include "midi/midi_cc_source.h"


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
        void receiveMidiMessage(uint8_t status, uint8_t channel, uint8_t data1, uint8_t data2);
};

class APC40LogicalKnobParameterInput : public MIDIParameterInputBase {
    private:
        APC40Controller &controller;
        const APC40TrackKnobBank bank;
        const uint8_t track;

    public:
        APC40LogicalKnobParameterInput(char *name, const char *group_name,
                APC40Controller &controller, APC40TrackKnobBank bank, uint8_t track)
            : MIDIParameterInputBase(name, group_name), controller(controller), bank(bank), track(track) {
            this->loop();
        }

        APC40TrackKnobBank get_bank() const { return this->bank; }
        uint8_t get_track() const { return this->track; }

        uint8_t get_value() const {
            APC40VirtualEncoderState state;
            return this->controller.getTrackKnobState(this->bank, this->track, state) ? state.value : 0;
        }

        bool set_value(uint8_t value) {
            APC40VirtualEncoderState state;
            if (!this->controller.getTrackKnobState(this->bank, this->track, state)) {
                return false;
            }
            value = value > 127 ? 127 : value;
            const APC40EncoderPolarity polarity = this->input_type == BIPOLAR
                ? APC40EncoderPolarity::Bipolar : APC40EncoderPolarity::Unipolar;
            if (state.value == value && state.polarity == polarity &&
                    state.ringMode == APC40EncoderRingMode::Automatic) {
                return true;
            }
            state.value = value;
            state.polarity = polarity;
            state.ringMode = APC40EncoderRingMode::Automatic;
            return this->controller.setTrackKnobState(this->bank, this->track, state);
        }

        virtual void loop() override {
            this->set_value(this->get_value());
        }

        virtual const char *getInputValue() override {
            static char formatted_value[20];
            snprintf(formatted_value, sizeof(formatted_value), "[%3u]",
                static_cast<unsigned>(this->get_value()));
            return formatted_value;
        }

        virtual const char *getOutputValue() override {
            static char formatted_value[20];
            const float value = this->input_type == BIPOLAR
                ? this->get_normal_value_bipolar() : this->get_normal_value_unipolar();
            snprintf(formatted_value, sizeof(formatted_value), "[%-4i%%]",
                static_cast<int>(value * 100.0f));
            return formatted_value;
        }

        virtual bool supports_bipolar_input() override {
            return true;
        }

        virtual float get_normal_value_unipolar() override {
            return static_cast<float>(this->get_value()) / 127.0f;
        }

        virtual float get_normal_value_bipolar() override {
            const float bipolar = (static_cast<float>(this->get_value()) - 64.0f) / 63.0f;
            return constrain(bipolar, -1.0f, 1.0f);
        }

        virtual const char *getInputInfo() override {
            static char input_info[24];
            const char *bank_name = "Unknown";
            switch (this->bank) {
                case APC40TrackKnobBank::Pan: bank_name = "Pan"; break;
                case APC40TrackKnobBank::Sends: bank_name = "Sends"; break;
                case APC40TrackKnobBank::User: bank_name = "User"; break;
            }
            snprintf(input_info, sizeof(input_info), "%s knob %u", bank_name,
                static_cast<unsigned>(this->track + 1));
            return input_info;
        }
};

class APC40CueKnobParameterInput : public MIDICCParameterInput {
    public:
        APC40CueKnobParameterInput(char *name, const char *group_name)
            : MIDICCParameterInput(name, group_name, APC40MKII::tracks::CC_CUE_LEVEL,
                APC40MKII::tracks::CHAN_KNOBS) {}

        virtual bool responds_to_cc(byte, byte) override { return false; }
        virtual void receive_control_change(byte, byte, byte) override {}

        void apply_delta(int16_t delta) {
            const int value = static_cast<int>(this->currentValue) + delta;
            this->currentValue = static_cast<int8_t>(constrain(value, 0, 127));
        }

        virtual const char *getInputInfo() override {
            return "APC40 relative Cue";
        }
};

class DeviceBehaviour_APC40MKII : virtual public DeviceBehaviourUSBBase, virtual public MIDI_CC_Source {
    private:
        APC40USBHostTransport apc_transport;
        APC40Controller apc_controller;
        APC40LogicalKnobParameterInput *track_knob_inputs[3][8] = {};
        MIDICCParameterInput *track_fader_inputs[8] = {};
        APC40CueKnobParameterInput *cue_knob_input = nullptr;

    public:

        uint16_t vid = 0x09e8, pid = 0x0029;
        virtual uint32_t get_packed_id () override { return (this->vid<<16 | this->pid); }

        virtual const char *get_label() override {
            return "APC40MKii";
        }
        virtual bool receives_midi_notes() { return true; }

        bool apc40mkii_shift_held = false;

        APC40LogicalKnobParameterInput *get_track_knob_input(APC40TrackKnobBank bank, uint8_t track) const {
            const uint8_t bank_index = static_cast<uint8_t>(bank);
            return bank_index < 3 && track < 8 ? this->track_knob_inputs[bank_index][track] : nullptr;
        }

        DeviceBehaviour_APC40MKII() : DeviceBehaviourUSBBase(), apc_controller(apc_transport) {
            this->apc_controller.setup_banks();
            // initialise the CCs that this device can translate into ParameterInputs
            static const char *const knob_groups[] = {"APC40 Pan", "APC40 Sends", "APC40 User"};
            static const char *const knob_names[3][8] = {
                {"PanKnob1", "PanKnob2", "PanKnob3", "PanKnob4", "PanKnob5", "PanKnob6", "PanKnob7", "PanKnob8"},
                {"SendsKnob1", "SendsKnob2", "SendsKnob3", "SendsKnob4", "SendsKnob5", "SendsKnob6", "SendsKnob7", "SendsKnob8"},
                {"UserKnob1", "UserKnob2", "UserKnob3", "UserKnob4", "UserKnob5", "UserKnob6", "UserKnob7", "UserKnob8"}
            };
            static const char *const fader_names[] = {
                "TrackFader1", "TrackFader2", "TrackFader3", "TrackFader4",
                "TrackFader5", "TrackFader6", "TrackFader7", "TrackFader8"
            };
            for (uint8_t bank = 0; bank < 3; ++bank) {
                for (uint8_t track = 0; track < 8; ++track) {
                    this->track_knob_inputs[bank][track] = new APC40LogicalKnobParameterInput(
                        (char *)knob_names[bank][track],
                        knob_groups[bank], this->apc_controller,
                        static_cast<APC40TrackKnobBank>(bank), track
                    );
                    this->addParameterInput(this->track_knob_inputs[bank][track]);
                }
            }
            
            for (uint8_t track = 0; track < 8; ++track) {
                this->track_fader_inputs[track] = new MIDICCParameterInput(
                    (char *)fader_names[track], "APC40 Faders",
                    (byte)APC40MKII::tracks::CC_FADER, (byte)(track + 1));
                this->addParameterInput(this->track_fader_inputs[track]);
            }

            this->addCCParameterInput("MasterFader", "APC40 Faders", (byte)APC40MKII::tracks::CC_FADER_MASTER,
                (byte)APC40MKII::tracks::CHAN_FADER_MASTER);
                        
            this->cue_knob_input = new APC40CueKnobParameterInput((char *)"CueKnob", "APC40MKII");
            this->addParameterInput(this->cue_knob_input);
            // todo: make this output bipolar by default?
            this->addCCParameterInput("Crossfader", "APC40MKII", (byte)APC40MKII::CC_CROSSFADER,
                (byte)APC40MKII::CHAN_CROSSFADER);
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
            for (auto &bank_inputs : this->track_knob_inputs) {
                for (auto *input : bank_inputs) {
                    input->loop();
                }
            }
            this->apc_controller.onCueEncoderDelta = [this](int16_t delta) {
                this->cue_knob_input->apply_delta(delta);
            };
            this->apc_controller.onTrackFaderChanged = [this](uint8_t track, uint8_t value) {
                if (track < 8 && this->track_fader_inputs[track] != nullptr) {
                    this->track_fader_inputs[track]->receive_control_change(
                        APC40MKII::tracks::CC_FADER, value,
                        track + 1
                    );
                }
            };
            this->apc_transport.set_device(this->device);
            this->apc_controller.init();
        }

        virtual void disconnect_device() override {
            this->apc_transport.set_device(nullptr);
            DeviceBehaviourUSBBase::disconnect_device();
        }

        virtual void receive_note_on(byte inChannel, byte inNumber, byte inVelocity) override {
            this->apc_transport.receiveMidiMessage(0x90, inChannel, inNumber, inVelocity);
            Debug_printf(F("apc40mkii_note_on for %i, %i, %i\n"), inChannel, inNumber, inVelocity);

            // if (inNumber==APC40_BUTTON_SHIFT) {
            //     apc40mkii_shift_held = true;            
            //     Serial.print("APC40MKII Shift button pressed\n");
            // }
            Serial.printf("APC40MKII Note On received: channel=%d, number=%d, value=%d\n", inChannel, inNumber, inVelocity);
        }

        virtual void receive_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) override {
            this->apc_transport.receiveMidiMessage(0x80, inChannel, inNumber, inVelocity);
            // if (inNumber==APC40_BUTTON_SHIFT) {
            //     apc40mkii_shift_held = false;
            //     Serial.print("APC40MKII Shift button released\n");
            // }
            Serial.printf("APC40MKII Note Off received: channel=%d, number=%d, value=%d\n", inChannel, inNumber, inVelocity);
        }

        virtual void receive_control_change(uint8_t channel, uint8_t number, uint8_t value) override {
            Serial.printf("APC40MKII CC received: channel=%d, number=%d, value=%d\n", channel, number, value);
            this->apc_transport.receiveMidiMessage(0xB0, channel, number, value);

            const bool is_track_fader = number == APC40MKII::tracks::CC_FADER;
            if (!is_track_fader) {
                MIDI_CC_Source::update_parameter_inputs_cc(number, value, channel);
            }
        }
};

extern DeviceBehaviour_APC40MKII *behaviour_apc40mkii;

#endif