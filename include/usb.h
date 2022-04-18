#ifndef USB__INCLUDED
#define USB__INCLUDED

#include <MIDI.h>
#include <USBHost_t36.h> // access to USB MIDI devices (plugged into 2nd USB port)

// Create the ports for USB devices plugged into Teensy's 2nd USB port (via hubs)
extern USBHost Usb;
extern USBHub hub1;
extern USBHub hub2;
extern USBHub hub3;
extern USBHub hub4;
extern MIDIDevice midi01;
extern MIDIDevice midi02;
extern MIDIDevice midi03;
extern MIDIDevice midi04;
extern MIDIDevice midi05;
extern MIDIDevice midi06;
extern MIDIDevice midi07;
extern MIDIDevice midi08;
extern MIDIDevice * usb_midi_device[8];/* = {
  &midi01, &midi02, &midi03, &midi04, &midi05, &midi06, &midi07, &midi08,
};*/

extern uint64_t usb_midi_connected[8]; //= { 0,0,0,0,0,0,0,0 };

#endif