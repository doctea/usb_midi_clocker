#ifndef MULTI_USBSERIAL_HANDLERS__INCLUDED
#define MULTI_USBSERIAL_HANDLERS__INCLUDED

#include "Config.h"

#ifdef ENABLE_USBSERIAL

#include "ConfigMidi.h"
#include "bpm.h"

/*
usb_midi_device[0] is 1C75:0288 aka Arturia:Arturia KeyStep 32
*/

#define NUM_USBSERIAL_DEVICES 3

// assign device to port and set appropriate handlers
void setup_usbserial_midi_device(uint8_t idx, uint32_t packed_id);
void update_usbserial_device_connections();
//void read_midi_usb_devices();

//void global_on_restart();
void setup_multi_usbserial();

//#include <MIDI.h>
#include <USBHost_t36.h> // access to USB MIDI devices (plugged into 2nd USB port)

class DeviceBehaviourUSBSerialBase;
class USBSerialWrapper;

#include "multi_usbserial_wrapper.h"

// TODO: deal with midiinterface in the specific DeviceBehaviour..
struct usbserial_midi_slot {
    uint16_t vid = 0x00;
    uint16_t pid = 0x00;
    uint32_t packed_id = 0x00;
    USBSerialWrapper *usbdevice = nullptr;
    MidiInterface<USBSerialWrapper> *midiinterface = nullptr;
    DeviceBehaviourUSBSerialBase *behaviour = nullptr;
};

extern usbserial_midi_slot usbserial_slots[NUM_USBSERIAL_DEVICES];

#endif
#endif