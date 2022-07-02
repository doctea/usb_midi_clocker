#include <MIDI.hpp>
#include <USBHost_t36.h>

#include <LinkedList.h>

class IUSBMidiDevice {
    public:
        const uint32_t vid = 0x0000, pid = 0x0000;
        //MIDIDevice *device = nullptr;
        MIDIDeviceBase *device = nullptr;

        IUSBMidiDevice() = default;
        virtual ~IUSBMidiDevice() = default;

        // interface methods - static
        virtual bool matches_identifiers(uint32_t vid, uint32_t pid) {
            return vid==this->vid && pid==this->pid;
        }

        virtual void connect_device(MIDIDeviceBase *device) {
            this->device = device;
            // subclass to map the callbacks
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
        virtual bool read() = 0;
        virtual bool send_clock(uint32_t ticks) {
            if (this->device!=nullptr) {
                this->device->sendRealTime(MIDIDevice::Clock);
            }
        }
        virtual void loop(uint32_t ticks) = 0;
        virtual void on_restart() = 0;
};

class ClockedUSBMidiDevice : public IUSBMidiDevice {

};