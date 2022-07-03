#ifndef BEHAVIOUR_BASE__INCLUDED
#define BEHAVIOUR_BASE__INCLUDED

#include <Arduino.h>

//#include <MIDI.hpp>
#include <USBHost_t36.h>

#include <LinkedList.h>

class DeviceBehaviourBase {
    public:
        const uint32_t vid = 0x0000, pid = 0x0000;
        //MIDIDevice *device = nullptr;
        MIDIDeviceBase *device = nullptr;
        //byte idx = 0xFF;

        DeviceBehaviourBase() = default;
        virtual ~DeviceBehaviourBase() = default;

        // interface methods - static
        virtual bool matches_identifiers(uint16_t vid, uint16_t pid) {
            return vid==this->vid && pid==this->pid;
        }
        virtual bool matches_identifiers(uint32_t packed_id) {
            return packed_id>>8 == this->vid && ((0b0000000011111111 & packed_id) == this->pid);
        }

        virtual void setup_callbacks() {}
        virtual void connect_device(MIDIDeviceBase *device) {
            this->device = device;
            this->setup_callbacks();
        }
        virtual void disconnect_device() {
            if (this->device!=nullptr) {
                // remove handlers that might already be set on this port -- new ones assigned below thru xxx_init() functions
                this->device->setHandleNoteOn(nullptr);
                this->device->setHandleNoteOff(nullptr);
                this->device->setHandleControlChange(nullptr);
                this->device->setHandleClock(nullptr);
                this->device->setHandleStart(nullptr);
                this->device->setHandleStop(nullptr);
            }
            this->device = nullptr;
        }

        /*// interface methods for instantiated devices
        virtual bool init(MIDIDevice *device) { 
            this->device = device;
            // add hooks to device
        };
        virtual bool deinit() {
            this->device = nullptr;
            // remove all hooks from device
        }*/
        virtual void read() {};
        virtual void send_clock(uint32_t ticks) {};
        virtual void loop(uint32_t ticks) {};
        virtual void on_tick(uint32_t ticks) {};
        virtual void on_restart() {};
        virtual void note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {}
        virtual void note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {}
        virtual void control_change (uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {}

};

class ClockedUSBMidiDevice : public DeviceBehaviourBase {
    public:
        virtual void send_clock(uint32_t ticks) override {
            if (this->device!=nullptr) {
                this->device->sendRealTime(MIDIDevice::Clock);
            }
        }
};

#endif