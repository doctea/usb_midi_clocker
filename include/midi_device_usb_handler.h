#include "midi_device_usb.h"

#include <LinkedList.h>

class USBMidiDeviceManager {
    public:
        static USBMidiDeviceManager* getInstance();

        LinkedList<IUSBMidiDevice *> devices = LinkedList<IUSBMidiDevice *>();

        void registerDevice(IUSBMIDIDevice *device) {
            this->devices->add(device);
        }

    private:
        static USBMidiDeviceManager* inst_;
        USBMidiDeviceManager() {}
        USBMidiDeviceManager(const USBMidiDeviceManager&);
        USBMidiDeviceManager& operator=(const USBMidiDeviceManager&);
};

USBMidiDeviceManager* USBMidiDeviceManager::inst_ = nullptr;

USBMidiDeviceManager* USBMidiDeviceManager::getInstance() {
    if (inst_ == nullptr) {
        inst_ = new USBMidiDeviceManager();
    }
    return inst_;
}