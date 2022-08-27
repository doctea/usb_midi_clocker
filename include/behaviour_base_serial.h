#include "behaviour_base.h"

class DeviceBehaviourSerialBase : virtual public DeviceBehaviourUltimateBase {
    public:
        midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> input_device = nullptr;
        midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> output_device = nullptr;

        DeviceBehaviourSerialBase() = default;
        virtual ~DeviceBehaviourSerialBase() = default;

        virtual bool is_connected() override {
            return this->output_device!=nullptr;
        }

        virtual void connect_device(midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *device) {
            //if (!is_connected()) return;

            Serial.printf("DeviceBehaviourSerialBase#connected_device connecting %p\n", device);
            this->output_device = device;
            this->setup_callbacks();
            this->init();
        }
        // remove handlers that might already be set on this port -- new ones assigned below thru setup_callbacks functions
        virtual void disconnect_device() {
            //if (this->device==nullptr) return;
            if (!is_connected()) return;
            
            this->output_device->setHandleNoteOn(nullptr);
            this->output_device->setHandleNoteOff(nullptr);
            this->output_device->setHandleControlChange(nullptr);
            this->output_device->setHandleClock(nullptr);
            this->output_device->setHandleStart(nullptr);
            this->output_device->setHandleStop(nullptr);
            this->output_device->setHandleSystemExclusive((void (*)(uint8_t *, unsigned int))nullptr);
            this->output_device = nullptr;
        }

        virtual void read() override {
            if (!is_connected()) return;

            while(this->input_device->read()); 
        };

        virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
            if (!is_connected()) return;
            this->output_device->sendNoteOn(note, velocity, channel);
        };
        virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
            if (!is_connected()) return;
            this->output_device->sendNoteOff(note, velocity, channel);
        };
        virtual void sendControlChange(uint8_t number, uint8_t value, uint8_t channel) override {
            if (!is_connected()) return;
            this->output_device->sendControlChange(number, value, channel);
        };
        virtual void sendRealTime(uint8_t message) override {
            if (!is_connected()) return;
            this->output_device->sendRealTime(message);
        };
};