#ifndef BEHAVIOUR_BASE_SERIAL__INCLUDED
#define BEHAVIOUR_BASE_SERIAL__INCLUDED

#include "behaviour_base.h"

class DeviceBehaviourSerialBase : virtual public DeviceBehaviourUltimateBase {
    public:
        midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *input_device = nullptr;
        midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output_device = nullptr;

        DeviceBehaviourSerialBase() = default;
        virtual ~DeviceBehaviourSerialBase() = default;

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

        virtual void connect_device_output(midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *device) {
            //if (!is_connected()) return;

            Serial.printf("DeviceBehaviourSerialBase#connect_device_output connecting device %p\n", device);
            this->output_device = device;
            this->connected_flag = true;
            //this->setup_callbacks();
            this->init();
        }
        virtual void connect_device_input(midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *device) {
            //if (!is_connected()) return;

            //device->begin(MIDI_CHANNEL_OMNI);
            //device->turnThruOff();

            Serial.printf("DeviceBehaviourSerialBase#connect_device_input connecting %p\n", device);
            this->input_device = device;
            this->connected_flag = true;
            this->setup_callbacks();
            this->init();
        }
        // remove handlers that might already be set on this port -- new ones assigned below thru setup_callbacks functions
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
            this->input_device = nullptr;
        }

        virtual void read() override {
            if (!is_connected() || this->input_device==nullptr) return;
            //Serial.println("DeviceBehaviourSerialBase#read() about to go into loop..");
            while(this->input_device->read()); 
            //Serial.println("DeviceBehaviourSerialBase#read() came out of loop..");
        };

        virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {
            if (!is_connected() || this->output_device==nullptr) return;
            this->output_device->sendNoteOn(note, velocity, channel);
        };
        virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {
            if (!is_connected() || this->output_device==nullptr) return;
            this->output_device->sendNoteOff(note, velocity, channel);
        };
        virtual void sendControlChange(uint8_t number, uint8_t value, uint8_t channel = 0) override {
            if (!is_connected() || this->output_device==nullptr) return;
            this->output_device->sendControlChange(number, value, channel);
        };
        virtual void sendRealTime(uint8_t message) override {
            if (!is_connected() || this->output_device==nullptr) return;
            this->output_device->sendRealTime((midi::MidiType)message);
        };
};

#endif