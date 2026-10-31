#include "behaviours/behaviour_apc40mkii.h"

#ifdef ENABLE_APC40MKII

void APC40USBHostTransport::set_device(MIDIDeviceBase *connected_device) {
    const bool was_connected = this->isConnected();
    this->device = connected_device;
    const bool is_connected = this->isConnected();
    if (was_connected != is_connected && this->onConnectionChanged) {
        this->onConnectionChanged(is_connected);
    }
}

bool APC40USBHostTransport::open() {
    return this->isConnected();
}

void APC40USBHostTransport::close() {
    this->set_device(nullptr);
}

bool APC40USBHostTransport::isConnected() const {
    return this->device != nullptr;
}

bool APC40USBHostTransport::sendMessage(const uint8_t *data, std::size_t size) {
    if (!this->isConnected() || data == nullptr || size == 0) {
        return false;
    }

    if (data[0] == 0xF0) {
        if (size < 2 || data[size - 1] != 0xF7) {
            return false;
        }
        this->device->sendSysEx(size, data, true);
        return true;
    }

    if (size != 3 || data[1] > 127 || data[2] > 127) {
        return false;
    }

    const uint8_t status = data[0] & 0xF0;
    const uint8_t channel = (data[0] & 0x0F) + 1;
    if (status == 0x80) {
        this->device->sendNoteOff(data[1], data[2], channel);
    } else if (status == 0x90) {
        if (data[2] == 0) {
            this->device->sendNoteOff(data[1], 0, channel);
        } else {
            this->device->sendNoteOn(data[1], data[2], channel);
        }
    } else if (status == 0xB0) {
        this->device->sendControlChange(data[1], data[2], channel);
    } else {
        return false;
    }

    return true;
}

void APC40USBHostTransport::receiveMidiMessage(
    uint8_t status, uint8_t channel,
    uint8_t data1, uint8_t data2
) {
    if (!this->onMessage || channel < 1 || channel > 16 ||
            data1 > 127 || data2 > 127 ||
            (status != 0x80 && status != 0x90 && status != 0xB0)) {
        return;
    }

    const uint8_t message[] = {
        static_cast<uint8_t>(status | (channel - 1)), data1, data2
    };
    this->onMessage(message, sizeof(message));
}

DeviceBehaviour_APC40MKII *behaviour_apc40mkii;

void apc40mkii_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    if (behaviour_apc40mkii!=nullptr) behaviour_apc40mkii->receive_control_change(inChannel, inNumber, inValue);
}

void apc40mkii_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_apc40mkii!=nullptr) behaviour_apc40mkii->receive_note_on(inChannel, inNumber, inVelocity);
}

void apc40mkii_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_apc40mkii!=nullptr) behaviour_apc40mkii->receive_note_off(inChannel, inNumber, inVelocity);
}

#endif