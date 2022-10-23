#ifndef BEHAVIOUR_BASE_USBSERIAL__INCLUDED
#define BEHAVIOUR_BASE_USBSERIAL__INCLUDED

#include "Config.h"
#ifdef ENABLE_USBSERIAL

#include "USBHost_t36.h"

#include "behaviours/behaviour_base.h"
#include "multi_usbserial_handlers.h"
#include "multi_usbserial_wrapper.h"

class DeviceBehaviourUSBSerialBase : virtual public DeviceBehaviourUltimateBase {
    public:
        midi::MidiInterface<USBSerialWrapper> *input_interface = nullptr;
        midi::MidiInterface<USBSerialWrapper> *output_interface = nullptr;

        DeviceBehaviourUSBSerialBase() = default;
        virtual ~DeviceBehaviourUSBSerialBase() = default;

        const uint32_t vid = 0x0000, pid = 0x0000;
        virtual uint32_t get_packed_id() { return (this->vid<<16 | this->pid); }
    
        // interface methods - static
        virtual bool matches_identifiers(uint16_t vid, uint16_t pid) {
            return vid==this->vid && pid==this->pid;
        }
        virtual bool matches_identifiers(uint32_t packed_id) {
            return (this->get_packed_id()==packed_id);
        }

        virtual int getConnectionBaudRate() {
            return 115200;
        }
        virtual int getConnectionFormat() {
            return USBHOST_SERIAL_8N1;
        }

        virtual bool has_input() { return this->input_interface!=nullptr; }
        virtual bool has_output() { return this->output_interface!=nullptr; }

        bool connected_flag = false;

        virtual bool is_connected() override {
            //return this->output_device!=nullptr || this->input_device!=nullptr;
            return this->connected_flag;
        }

        virtual int getType() override {
            return BehaviourType::usbserial;
        }

        virtual void connect_device_output(USBSerialWrapper *usbdevice, midi::MidiInterface<USBSerialWrapper> *midiinterface) {
            //if (!is_connected()) return;

            if (this->debug) Serial.printf("DeviceBehaviour_USBSerialBase#connect_device_output connecting device %p\n", usbdevice);
            //this->output_device = usbdevice;
            this->output_interface = midiinterface;
            this->connected_flag = true;
            this->init();
        }
        virtual void connect_device_input(USBSerialWrapper *usbdevice, midi::MidiInterface<USBSerialWrapper> *midiinterface) {
            //if (!is_connected()) return;

            //device->begin(MIDI_CHANNEL_OMNI);
            //device->turnThruOff();

            if (this->debug) Serial.printf("DeviceBehaviourUSBSerialBase#connect_device_input connecting %p\n", usbdevice);
            this->input_interface = midiinterface;
            this->connected_flag = true;
            Serial.printf("about to call setup_callbacks on %s..\n", this->get_label()); Serial.flush();
            this->setup_callbacks();
            Serial.printf("about to call init on %s..\n", this->get_label()); Serial.flush();
            this->init();
        }
        virtual void connect_device(USBSerialWrapper *usbdevice, midi::MidiInterface<USBSerialWrapper> *midiinterface) {
            Serial.printf("DeviceBehaviour_USBSerialBase#connect_device connecting %p\n", usbdevice);
            usbdevice->begin(this->getConnectionBaudRate(), this->getConnectionFormat());

            this->connect_device_input(usbdevice, midiinterface);
            this->connect_device_output(usbdevice, midiinterface);
        }

        // remove handlers that might already be set on this port -- new ones assigned below thru setup_callbacks functions
        virtual void disconnect_device() {
            //if (this->device==nullptr) return;
            if (!is_connected()) return;

            this->connected_flag = false;
            
            if (this->input_interface==nullptr) return;

            this->input_interface->setHandleNoteOn(nullptr);
            this->input_interface->setHandleNoteOff(nullptr);
            this->input_interface->setHandleControlChange(nullptr);
            this->input_interface->setHandleClock(nullptr);
            this->input_interface->setHandleStart(nullptr);
            this->input_interface->setHandleStop(nullptr);
            this->input_interface->setHandleSystemExclusive((void (*)(uint8_t *, unsigned int))nullptr);
            this->input_interface = nullptr;
            this->output_interface = nullptr;
        }

        virtual void read() override {
            if (!is_connected() || this->input_interface==nullptr) return;
            //Serial.println("DeviceBehaviourSerialBase#read() about to go into loop..");
            while(this->input_interface->read()); 
            //Serial.println("DeviceBehaviourSerialBase#read() came out of loop..");
        };

        virtual void actualSendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {
            if (!is_connected() || this->output_interface==nullptr) return;
            if (this->debug) Serial.printf("DeviceBehaviour_USBSerialBase#sendNoteOn(%i, %i, %i)!\n", note, velocity, channel);
            this->output_interface->sendNoteOn(note, velocity, channel);
        };
        virtual void actualSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {
            if (!is_connected() || this->output_interface==nullptr) return;
            this->output_interface->sendNoteOff(note, velocity, channel);
        };
        virtual void actualSendControlChange(uint8_t number, uint8_t value, uint8_t channel = 0) override {
            if (!is_connected() || this->output_interface==nullptr) return;
            this->output_interface->sendControlChange(number, value, channel);
        };
        virtual void actualSendRealTime(uint8_t message) override {
            if (!is_connected() || this->output_interface==nullptr) return;
            this->output_interface->sendRealTime((midi::MidiType)message);
        };
};

#endif
#endif