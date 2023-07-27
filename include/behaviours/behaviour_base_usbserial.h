#ifndef BEHAVIOUR_BASE_USBSERIAL__INCLUDED
#define BEHAVIOUR_BASE_USBSERIAL__INCLUDED

#include "Config.h"
#ifdef ENABLE_USBSERIAL

#include "USBHost_t36.h"

#include "behaviours/behaviour_base.h"
#include "multi_usbserial_handlers.h"
#include "multi_usbserial_wrapper.h"

// USB device that presents as a Serial connection, instead of as a MIDI device
class DeviceBehaviourUSBSerialBase : virtual public DeviceBehaviourUltimateBase {
    public:

        DeviceBehaviourUSBSerialBase() = default;
        virtual ~DeviceBehaviourUSBSerialBase() = default;

        USBSerialWrapper *usbdevice = nullptr;

        const uint32_t vid = 0x0000, pid = 0x0000;
        virtual uint32_t get_packed_id() { return (this->vid<<16 | this->pid); }

        // interface methods - static
        virtual bool matches_identifiers(uint16_t vid, uint16_t pid) {
            return vid==this->vid && pid==this->pid;
        }
        virtual bool matches_identifiers(uint32_t packed_id) {
            return (this->get_packed_id()==packed_id);
        }

        virtual int getType() override {
            return BehaviourType::usbserial;
        }

        virtual const char *get_label() {
            return "USBSerialBase";
        }

        virtual int getConnectionBaudRate() {
            return 115200;
        }
        virtual int getConnectionFormat() {
            return USBHOST_SERIAL_8N1;
        }

        virtual void connect_device(USBSerialWrapper *usbdevice) {
            Serial.printf(F("DeviceBehaviour_USBSerialBase#connect_device for %s connecting %p\n"), this->get_label(), usbdevice);

            if (this->usbdevice != nullptr && this->usbdevice != usbdevice) {
                Serial.printf(F("\tit is already connected to a different usbdevice! disconnecting first.."));
                this->disconnect_device();
            }

            this->usbdevice = usbdevice;

            // do this in subdevices now, since it seems to cause some problems?
            //usbdevice->begin(this->getConnectionBaudRate(), this->getConnectionFormat());
            //usbdevice->setTimeout(0);

            this->init();
            this->setup_callbacks();
        }

        virtual void disconnect_device() {
            if (!this->is_connected()) return;
            //if (this->usbdevice) this->usbdevice->end();
            this->usbdevice = nullptr;
        }

        virtual bool is_connected() override {
            //return this->output_device!=nullptr || this->input_device!=nullptr;
            return this->usbdevice!=nullptr && this->usbdevice; // connected_flag;
        }

        virtual void init() override {
            // do anything we need to do after we've just connected a new serial midi device..?
        }

        // some serial devices may crash if we don't read from their serial devices, apparently?
        virtual void read() override {
            if (!is_connected() || this->usbdevice==nullptr) return;
            //Serial.println("DeviceBehaviourUSBSerialBase#read() about to read()..");
            while(this->usbdevice->available() && this->usbdevice->read()); 
            //Serial.println("DeviceBehaviourUSBSerialBase#read() came out of read()..");
        };
};

// USB device that presents as Serial, but supports MIDI (for eg plain Arduino, Hairless-MIDI-alike, OpenTheremin v4 with MIDI code)
class DeviceBehaviourUSBSerialMIDIBase : virtual public DeviceBehaviourUSBSerialBase {
    public:
        midi::MidiInterface<USBSerialWrapper> *midi_interface = nullptr;

        DeviceBehaviourUSBSerialMIDIBase() = default;
        virtual ~DeviceBehaviourUSBSerialMIDIBase() = default;

        virtual int getType() override {
            return BehaviourType::usbserialmidi;
        }

        virtual const char *get_label() {
            return "USBSerialMIDIBase";
        }

        //virtual bool has_input()    { return this->input_interface!=nullptr; }
        //virtual bool has_output()   { return this->output_interface!=nullptr; }

        virtual void init() override {
            
            if (this->usbdevice==nullptr) {
                Serial.printf(F("DeviceBehaviourUSBSerialMIDIBase#init() in %s failed - usbdevice is nullptr!\n"), this->get_label());
                return;
            }

            if (this->midi_interface!=nullptr) {
                Serial.printf(F("DeviceBehaviourUSBSerialMIDIBase#init() in %s already has a midi_interface - disconnecting it!\n"), this->get_label());
                this->disconnect_device();
            }

            usbdevice->begin(this->getConnectionBaudRate(), this->getConnectionFormat());
            usbdevice->setTimeout(0);

            this->midi_interface = new midi::MidiInterface<USBSerialWrapper>(*this->usbdevice);
        }

        // remove handlers that might already be set on this port -- new ones assigned below thru setup_callbacks functions
        virtual void disconnect_device() {
            //if (this->device==nullptr) return;
            if (!is_connected()) return;
            DeviceBehaviourUSBSerialBase::disconnect_device();

            if (this->midi_interface==nullptr) return;

            this->midi_interface->setHandleNoteOn(nullptr);
            this->midi_interface->setHandleNoteOff(nullptr);
            this->midi_interface->setHandleControlChange(nullptr);
            this->midi_interface->setHandleClock(nullptr);
            this->midi_interface->setHandleStart(nullptr);
            this->midi_interface->setHandleStop(nullptr);
            this->midi_interface->setHandleSystemExclusive((void (*)(uint8_t *, unsigned int))nullptr);
            this->midi_interface->setHandlePitchBend(nullptr);
            this->midi_interface = nullptr;

            delete this->midi_interface;
        }

        virtual void read() override {
            if (!is_connected() || this->midi_interface==nullptr) return;
            //Serial.println("DeviceBehaviourSerialMIDIBase#read() about to go into loop..");
            while(this->midi_interface->read()); 
            //Serial.println("DeviceBehaviourSerialMIDIBase#read() came out of loop..");
        };

        virtual void actualSendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {
            if (!is_connected() || this->midi_interface==nullptr) return;
            if (this->debug) Serial.printf(F("DeviceBehaviour_USBSerialMIDIBase#sendNoteOn(%i, %i, %i)!\n"), note, velocity, channel);
            this->midi_interface->sendNoteOn(note, velocity, channel);
        };
        virtual void actualSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {
            if (!is_connected() || this->midi_interface==nullptr) return;
            this->midi_interface->sendNoteOff(note, velocity, channel);
        };
        virtual void actualSendControlChange(uint8_t number, uint8_t value, uint8_t channel = 0) override {
            if (!is_connected() || this->midi_interface==nullptr) return;
            this->midi_interface->sendControlChange(number, value, channel);
        };
        virtual void actualSendRealTime(uint8_t message) override {
            if (!is_connected() || this->midi_interface==nullptr) return;
            this->midi_interface->sendRealTime((midi::MidiType)message);
        };
        virtual void actualSendPitchBend(int16_t bend, uint8_t channel = 0) {
            if (!is_connected() || this->midi_interface==nullptr) return;
            this->midi_interface->sendPitchBend(bend, channel);
        }
};

#endif
#endif