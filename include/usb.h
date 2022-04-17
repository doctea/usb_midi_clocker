#include <MIDI.h>
#include <USBHost_t36.h> // access to USB MIDI devices (plugged into 2nd USB port)

// Create the ports for USB devices plugged into Teensy's 2nd USB port (via hubs)
USBHost Usb;
USBHub hub1(Usb);
USBHub hub2(Usb);
USBHub hub3(Usb);
USBHub hub4(Usb);
MIDIDevice midi01(Usb);
MIDIDevice midi02(Usb);
MIDIDevice midi03(Usb);
MIDIDevice midi04(Usb);
MIDIDevice midi05(Usb);
MIDIDevice midi06(Usb);
MIDIDevice midi07(Usb);
MIDIDevice midi08(Usb);
MIDIDevice * usbmidilist[8] = {
  &midi01, &midi02, &midi03, &midi04, &midi05, &midi06, &midi07, &midi08,
};

uint64_t usb_midi_connected[8] = { 0,0,0,0,0,0,0,0 };