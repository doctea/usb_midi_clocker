#include "Config.h"
#include "ConfigMidi.h"
#include "bpm.h"
#include "midi_outs.h"

/*
usb_midi_device[0] is 1C75:0288 aka Arturia:Arturia KeyStep 32
usb_midi_device[1] is 2886:800B aka The Tyrell Corporation:Bambleweeny57
usb_midi_device[2] is 1C75:0206 aka Arturia:Arturia BeatStep
usb_midi_device[3] is 09E8:0028 aka AKAI PROFESSIONAL,LP:APC MINI       
usb_midi_device[4] is 09E8:006B aka Akai:Akai MPK49
*/

#define NUM_USB_DEVICES 8

// assign device to port and set appropriate handlers
void setup_usb_midi_device(uint8_t idx, uint32_t packed_id);
void update_usb_device_connections();
void read_midi_usb_devices();
void loop_midi_usb_devices();
void send_midi_usb_clocks();

void global_on_restart();
void setup_multi_usb();

#include <MIDI.h>
#include <USBHost_t36.h> // access to USB MIDI devices (plugged into 2nd USB port)

// Create the ports for USB devices plugged into Teensy's 2nd USB port (via hubs)
extern USBHost Usb;
extern USBHub hub1;
extern USBHub hub2;
extern USBHub hub3;
extern USBHub hub4;
extern MIDIDevice_BigBuffer midi01;
extern MIDIDevice_BigBuffer midi02;
extern MIDIDevice_BigBuffer midi03;
extern MIDIDevice_BigBuffer midi04;
extern MIDIDevice_BigBuffer midi05;
extern MIDIDevice_BigBuffer midi06;
extern MIDIDevice_BigBuffer midi07;
extern MIDIDevice_BigBuffer midi08;
extern MIDIDevice_BigBuffer * usb_midi_device[NUM_USB_DEVICES];
extern uint64_t usb_midi_connected[NUM_USB_DEVICES];
