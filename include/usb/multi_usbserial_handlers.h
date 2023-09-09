#ifndef MULTI_USBSERIAL_HANDLERS__INCLUDED
#define MULTI_USBSERIAL_HANDLERS__INCLUDED

#include "Config.h"

#ifdef ENABLE_USBSERIAL

#include "ConfigMidi.h"
#include "bpm.h"

#define NUM_USB_SERIAL_DEVICES 3

// assign device to port and set appropriate handlers
void setup_usbserial_midi_device(uint8_t idx, uint32_t packed_id);
void update_usbserial_device_connections();
//void read_midi_usb_devices();

//void global_on_restart();
void setup_multi_usbserial();

//#include <MIDI.h>
#include <USBHost_t36.h> // access to USB MIDI devices (plugged into 2nd USB port)

class USBSerialWrapper;

#include "multi_usbserial_wrapper.h"

class DeviceBehaviourUSBSerialBase;

struct usbserial_midi_slot {
    uint16_t vid = 0x0000;
    uint16_t pid = 0x0000;
    uint32_t packed_id = 0x00000000;
    USBSerialWrapper *usbdevice = nullptr;
    DeviceBehaviourUSBSerialBase *behaviour = nullptr;
};

extern usbserial_midi_slot usb_serial_slots[NUM_USB_SERIAL_DEVICES];

#endif
#endif