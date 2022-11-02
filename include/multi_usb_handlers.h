#ifndef MULTI_USB_HANDLERS__INCLUDED
#define MULTI_USB_HANDLERS__INCLUDED

#include "Config.h"
#include "ConfigMidi.h"
#include "bpm.h"
#include "midi/midi_outs.h"


//#include "usb_device_handler.h"

/*
usb_midi_device[0] is 1C75:0288 aka Arturia:Arturia KeyStep 32
usb_midi_device[1] is 2886:800B aka The Tyrell Corporation:Bambleweeny57
usb_midi_device[2] is 1C75:0206 aka Arturia:Arturia BeatStep
usb_midi_device[3] is 09E8:0028 aka AKAI PROFESSIONAL,LP:APC MINI       
usb_midi_device[4] is 09E8:006B aka Akai:Akai MPK49
*/

#define NUM_USB_DEVICES 10

// assign device to port and set appropriate handlers
void setup_usb_midi_device(uint8_t idx, uint32_t packed_id);
void update_usb_device_connections();
//void read_midi_usb_devices();

void global_on_restart();
//FLASHMEM  // // error: void setup_multi_usb() causes a section type conflict with virtual void DeviceBehaviourUltimateBase::setup_callbacks()
void setup_multi_usb();

//#include <MIDI.h>
#include <USBHost_t36.h> // access to USB MIDI devices (plugged into 2nd USB port)

// Create the ports for USB devices plugged into Teensy's 2nd USB port (via hubs)
extern USBHost Usb;
extern USBHub hub1;
extern USBHub hub2;
extern USBHub hub3;
extern USBHub hub4;
extern use_MIDIDevice_BigBuffer midi01;
extern use_MIDIDevice_BigBuffer midi02;
extern use_MIDIDevice_BigBuffer midi03;
extern use_MIDIDevice_BigBuffer midi04;
extern use_MIDIDevice_BigBuffer midi05;
extern use_MIDIDevice_BigBuffer midi06;
extern use_MIDIDevice_BigBuffer midi07;
extern use_MIDIDevice_BigBuffer midi08;
extern use_MIDIDevice_BigBuffer midi09;
extern use_MIDIDevice_BigBuffer midi10;
/*extern use_MIDIDevice_BigBuffer midi11;
extern use_MIDIDevice_BigBuffer midi12;
extern use_MIDIDevice_BigBuffer midi13;
extern use_MIDIDevice_BigBuffer midi14;
extern use_MIDIDevice_BigBuffer midi15;*/

//extern MIDIDevice_BigBuffer * usb_midi_device[NUM_USB_DEVICES];
//extern uint64_t usb_midi_connected[NUM_USB_DEVICES];

//#include "behaviours/behaviour_base.h"

class DeviceBehaviourUSBBase;

struct usb_midi_slot {
    uint16_t vid = 0x00;
    uint16_t pid = 0x00;
    uint32_t packed_id = 0x00;
    MIDIDeviceBase *device = nullptr;
    DeviceBehaviourUSBBase *behaviour = nullptr;
};

extern usb_midi_slot usb_midi_slots[NUM_USB_DEVICES];

#endif