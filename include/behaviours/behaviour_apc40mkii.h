// behaviour for APC40 MKII

#pragma once

#include <Arduino.h>
#include "Config.h"

#ifdef ENABLE_APC40MKII

#include "behaviours/behaviour_base_usb.h"
#include "usb/multi_usb_handlers.h"

// use parameter library adapter to handle cc events
#include "midi/midi_cc_source.h"

void apc40mkii_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void apc40mkii_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void apc40mkii_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);

#define SYSEX_MESSAGE_TYPE_INTRODUCTION_IDENTIFIER 0x60

#define SYSEX_APPLICATION_IDENTIFIER_MODE_0 0x40
#define SYSEX_APPLICATION_IDENTIFIER_MODE_1 0x41
#define SYSEX_APPLICATION_IDENTIFIER_MODE_2 0x42


#define APC40MKII_KNOB_CUE_CC_1      47
#define APC40MKII_KNOB_TOP_CC_1      48
#define APC40MKII_KNOB_TOP_CC_2      49
#define APC40MKII_KNOB_TOP_CC_3      50
#define APC40MKII_KNOB_TOP_CC_4      51
#define APC40MKII_KNOB_TOP_CC_5      52
#define APC40MKII_KNOB_TOP_CC_6      53
#define APC40MKII_KNOB_TOP_CC_7      54
#define APC40MKII_KNOB_TOP_CC_8      55
#define APC40MKII_KNOB_TOP_CC_MASTER 56

// master fader is cc 14 on ch1
#define APC40MKII_MASTER_FADER_CC 14

// crossfader CC = 15 on ch1
#define APC40MKII_CROSSFADER_CC 15

// faders are CC 7, on channel 1-8
#define APC40MKII_FADER_CC 7
// device knobs are CC 16-23 on channel determined by the channel selector buttons
// channel selector buttons are cc 23 on channel of the channel 1-8
#define APC40MKII_TEMPO_KNOB_CC 13

// tempo knob is CC 13 on channel 1, with values of 127=anti-clockwise, 1=clockwise
// APC40MKII CC received: channel=1, number=13, value=1


class DeviceBehaviour_APC40MKII : virtual public DeviceBehaviourUSBBase, virtual public MIDI_CC_Source {
    public:

        uint16_t vid = 0x09e8, pid = 0x0029;
        virtual uint32_t get_packed_id () override { return (this->vid<<16 | this->pid); }

        virtual const char *get_label() override {
            return "APC40MKii";
        }
        virtual bool receives_midi_notes() { return true; }

        bool apc40mkii_shift_held = false;

        DeviceBehaviour_APC40MKII() : DeviceBehaviourUSBBase() {
            // initialise the CCs that this device can translate into ParameterInputs
            this->addCCParameterInput("TopKnob1", "APC40 Knobs", (byte)APC40MKII_KNOB_TOP_CC_1, (byte)1);
            this->addCCParameterInput("TopKnob2", "APC40 Knobs", (byte)APC40MKII_KNOB_TOP_CC_2, (byte)1);
            this->addCCParameterInput("TopKnob3", "APC40 Knobs", (byte)APC40MKII_KNOB_TOP_CC_3, (byte)1);
            this->addCCParameterInput("TopKnob4", "APC40 Knobs", (byte)APC40MKII_KNOB_TOP_CC_4, (byte)1);
            this->addCCParameterInput("TopKnob5", "APC40 Knobs", (byte)APC40MKII_KNOB_TOP_CC_5, (byte)1);
            this->addCCParameterInput("TopKnob6", "APC40 Knobs", (byte)APC40MKII_KNOB_TOP_CC_6, (byte)1);
            this->addCCParameterInput("TopKnob7", "APC40 Knobs", (byte)APC40MKII_KNOB_TOP_CC_7, (byte)1);
            this->addCCParameterInput("TopKnob8", "APC40 Knobs", (byte)APC40MKII_KNOB_TOP_CC_8, (byte)1);
            
            this->addCCParameterInput("MasterFader", "APC40 Faders", (byte)APC40MKII_MASTER_FADER_CC, (byte)1);
                        
            this->addCCParameterInput("CueKnob", "APC40MKII", (byte)APC40MKII_KNOB_CUE_CC_1, (byte)1);
            // todo: make this output bipolar by default?
            this->addCCParameterInput("Crossfader", "APC40MKII", (byte)APC40MKII_CROSSFADER_CC, (byte)1);

            // @@TODO: set up the 3 banks of fixed knobs..
        }

        FLASHMEM 
        virtual void setup_callbacks() override {
            //behaviour_apc40mkii_instance = this;
            if (!is_connected()) return;

            this->device->setHandleControlChange(apc40mkii_control_change);
            this->device->setHandleNoteOn(apc40mkii_note_on);
            this->device->setHandleNoteOff(apc40mkii_note_off);

            // APC40 MkI Smart Mode Handshake Array
            // byte apc_handshake[] = {
            //     0xF0, 0x47, 0x7F, 0x29, 
            //     SYSEX_MESSAGE_TYPE_INTRODUCTION_IDENTIFIER, 0x00, 0x04, SYSEX_APPLICATION_IDENTIFIER_MODE_2, 0x00, 0x00, 0x00, 0xF7};
            Serial.printf("Sending APC40 MkII handshake SysEx\n");          
            // this->device->sendSysEx(sizeof(apc_handshake), apc_handshake, true);

            this->send_sysex_command(
                SYSEX_MESSAGE_TYPE_INTRODUCTION_IDENTIFIER,
                4,
                (const byte[]){
                    SYSEX_APPLICATION_IDENTIFIER_MODE_1, // mode
                    0x09, // major version
                    0x07, // minor version
                    0x01  // bugfix level
                }
            );

        };

        virtual void send_sysex_command(byte message_identifier, int16_t size, const byte *data) {
            byte sysex[size + 7];
            // sysex[0] = 0xF0;
            sysex[0] = 0x47;
            sysex[1] = 0x7F;
            sysex[2] = 0x29;
            sysex[3] = message_identifier;
            sysex[4] = size >> 8; // MSB of size
            sysex[5] = size & 0xFF; // LSB of size
            for (int i = 0; i < size; i++) {
                sysex[6 + i] = data[i];
            }
            // sysex[size + 7] = 0xF7;
            this->device->sendSysEx(size + 6, sysex, false);
        }

        FLASHMEM
        virtual void init() override {
            DeviceBehaviourUSBBase::init();
            // apc40mkii_display_initialise_last_sent();
            // apc40mkii_clear_display(true);
            // redraw_immediately = true;
            //apc40mkii_update_clock_display();
        }

        virtual void receive_note_on(byte inChannel, byte inNumber, byte inVelocity) override {
            Debug_printf(F("apc40mkii_note_on for %i, %i, %i\n"), inChannel, inNumber, inVelocity);

            // if (inNumber==APC40_BUTTON_SHIFT) {
            //     apc40mkii_shift_held = true;            
            //     Serial.print("APC40MKII Shift button pressed\n");
            // }
            Serial.printf("APC40MKII Note On received: channel=%d, number=%d, value=%d\n", inChannel, inNumber, inVelocity);
        }

        virtual void receive_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) override {
            // if (inNumber==APC40_BUTTON_SHIFT) {
            //     apc40mkii_shift_held = false;
            //     Serial.print("APC40MKII Shift button released\n");
            // }
            Serial.printf("APC40MKII Note Off received: channel=%d, number=%d, value=%d\n", inChannel, inNumber, inVelocity);
        }

        virtual void receive_control_change(uint8_t channel, uint8_t number, uint8_t value) override {
            // #ifdef ENABLE_BPM
            //     if (number==tempo_fader_cc/*APCMINI_FADER_CC_MASTER*/) {   // 56 == "master" fader set bpm 
            //         if (clock_mode==CLOCK_INTERNAL)
            //             set_bpm(map(value, 0, 127, BPM_MINIMUM, BPM_MAXIMUM)); // scale CC value
            //     }
            // #endif

            Serial.printf("APC40MKII CC received: channel=%d, number=%d, value=%d\n", channel, number, value);

            int steps = 0;
            if (value > 64) {
                // Turning Left (CC values 127, 126, 125...)
                steps = value - 128; // Results in negative step: -1, -2, -3...
            } else {
                // Turning Right (CC values 1, 2, 3...)
                steps = value;       // Results in positive step: 1, 2, 3...
            }
            Serial.printf("APC40MKII CC steps calculated: %d\n", steps);

            MIDI_CC_Source::update_parameter_inputs_cc(number, value, channel);
        }
};

extern DeviceBehaviour_APC40MKII *behaviour_apc40mkii;

#endif