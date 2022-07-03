#include <Arduino.h>

#include <MIDI.hpp>
#include <USBHost_t36.h>

#include <LinkedList.h>

class IUSBMidiDevice {
    public:
        const uint32_t vid = 0x0000, pid = 0x0000;
        //MIDIDevice *device = nullptr;
        MIDIDeviceBase *device = nullptr;
        //byte idx = 0xFF;

        IUSBMidiDevice() = default;
        virtual ~IUSBMidiDevice() = default;

        // interface methods - static
        virtual bool matches_identifiers(uint32_t vid, uint32_t pid) {
            return vid==this->vid && pid==this->pid;
        }

        virtual void setup_callbacks() {}
        virtual void connect_device(MIDIDeviceBase *device) {
            this->device = device;
            this->setup_callbacks();
        }
        virtual void disconnect_device() {
            if (this->device!=nullptr) {
                // unmap the callbacks
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
        virtual bool read() {};
        virtual bool send_clock(uint32_t ticks) {};
        virtual void loop(uint32_t ticks) {};
        virtual void on_tick(uint32_t ticks) {};
        virtual void on_restart() {};
        virtual void note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {}
        virtual void note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {}
        virtual void control_change (uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {}

};

class ClockedUSBMidiDevice : public IUSBMidiDevice {
    public:
        virtual bool send_clock(uint32_t ticks) {
            if (this->device!=nullptr) {
                this->device->sendRealTime(MIDIDevice::Clock);
            }
        }
};