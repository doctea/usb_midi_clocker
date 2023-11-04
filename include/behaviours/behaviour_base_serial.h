#ifndef BEHAVIOUR_BASE_SERIAL__INCLUDED
#define BEHAVIOUR_BASE_SERIAL__INCLUDED

#include "behaviours/behaviour_base.h"

#include "midi/midi_mapper_matrix_types.h"
#include "midi/midi_mapper_matrix_manager.h"

#include <util/atomic.h>

// a hardware MIDI device over HardwareSerial uart (ie on DIN or stereo jack)
class DeviceBehaviourSerialBase : virtual public DeviceBehaviourUltimateBase {
    public:
        serial_midi_number_t input_midi_number = -1;
        serial_midi_number_t output_midi_number = -1;
        midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *input_device = nullptr;
        midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output_device = nullptr;

        DeviceBehaviourSerialBase() = default;
        virtual ~DeviceBehaviourSerialBase() = default;

        virtual bool receives_midi_notes()  { return this->input_device!=nullptr; }
        virtual bool transmits_midi_notes() { return this->output_device!=nullptr; }
        //char indicator_text[5];
        virtual const char *get_indicator() override;

        /*DeviceBehaviourSerialBase (
            midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *input_device, 
            midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output_device
        ) : DeviceBehaviourSerialBase () {
            this->input_device = input_device;
            this->output_device = output_device;
        }*/
        bool connected_flag = false;

        virtual bool is_connected() override {
            //return this->output_device!=nullptr || this->input_device!=nullptr;
            return this->connected_flag;
        }

        virtual int getType() override {
            return BehaviourType::serial;
        }

        //FLASHMEM
        virtual void connect_device_output(midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *device) {
            //if (!is_connected()) return;

            //if (this->debug) Serial.printf(F("DeviceBehaviourSerialBase#connect_device_output connecting device %p\n"), device);
            this->output_device = device;
            this->connected_flag = true;
            this->output_midi_number = midi_matrix_manager->get_serial_midi_number_for_device(device);

            this->indicator_done = false;
            this->init();
        }
        //FLASHMEM
        virtual void connect_device_input(midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *device) {
            //if (!is_connected()) return;

            //device->begin(MIDI_CHANNEL_OMNI);
            //device->turnThruOff();

            //if (this->debug) Serial.printf(F("DeviceBehaviourSerialBase#connect_device_input connecting %p\n"), device);
            this->input_device = device;
            this->connected_flag = true;
            this->input_midi_number = midi_matrix_manager->get_serial_midi_number_for_device(device);
            
            this->indicator_done = false;
            //Serial.printf(F("about to call setup_callbacks on %s..\n"), this->get_label()); Serial_flush();
            this->setup_callbacks();
            //Serial.printf(F("about to call init on %s..\n"), this->get_label()); Serial_flush();
            this->init();
        }
        
        // remove handlers that might already be set on this port -- new ones assigned below thru setup_callbacks functions
        FLASHMEM
        virtual void disconnect_device() {
            //if (this->device==nullptr) return;
            if (!is_connected()) return;

            this->connected_flag = false;
            
            if (this->input_device==nullptr) return;

            this->input_device->setHandleNoteOn(nullptr);
            this->input_device->setHandleNoteOff(nullptr);
            this->input_device->setHandleControlChange(nullptr);
            this->input_device->setHandleClock(nullptr);
            this->input_device->setHandleStart(nullptr);
            this->input_device->setHandleStop(nullptr);
            this->input_device->setHandleSystemExclusive((void (*)(uint8_t *, unsigned int))nullptr);
            this->input_device->setHandlePitchBend(nullptr);
            this->input_device = nullptr;
        }

        virtual void read() override {
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                //if (!is_connected() || this->input_device==nullptr) return;
                //Serial.println("DeviceBehaviourSerialBase#read() about to go into loop..");
                //while(this->input_device->read()); 
                //Serial.println("DeviceBehaviourSerialBase#read() came out of loop..");
            }
        };

        virtual void actualSendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {
            if (!is_connected() || this->output_device==nullptr) return;
            if (this->debug) Serial.printf(F("DeviceBehaviour_SerialBase#%s#sendNoteOn(%i, %i, %i)!\n"), get_label(), note, velocity, channel);
            this->output_device->sendNoteOn(note, velocity, channel);
        };
        virtual void actualSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {
            if (!is_connected() || this->output_device==nullptr) return;
            if (this->debug) Serial.printf("%s#actualSendNoteOff(%i, %i, %i)\n", get_label(), note, velocity, channel);
            this->output_device->sendNoteOff(note, velocity, channel);
        };
        virtual void actualSendControlChange(uint8_t number, uint8_t value, uint8_t channel = 0) override {
            if (!is_connected() || this->output_device==nullptr) return;
            this->output_device->sendControlChange(number, value, channel);
        };
        virtual void actualSendRealTime(uint8_t message) override {
            if (!is_connected() || this->output_device==nullptr) return;
            this->output_device->sendRealTime((midi::MidiType)message);
        };
        virtual void actualSendPitchBend(int16_t bend, uint8_t channel = 0) {
            if (!is_connected() || this->output_device==nullptr) return;
            this->output_device->sendPitchBend(bend, channel);
        }
};

#endif