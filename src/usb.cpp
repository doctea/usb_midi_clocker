#include "usb.h"

USBHost Usb;
USBHub hub1(Usb);
USBHub hub2(Usb);
USBHub hub3(Usb);
USBHub hub4(Usb);
MIDIDevice_BigBuffer midi01(Usb);
MIDIDevice_BigBuffer midi02(Usb);
MIDIDevice_BigBuffer midi03(Usb);
MIDIDevice_BigBuffer midi04(Usb);
MIDIDevice_BigBuffer midi05(Usb);
MIDIDevice_BigBuffer midi06(Usb);
MIDIDevice_BigBuffer midi07(Usb);
MIDIDevice_BigBuffer midi08(Usb);

MIDIDevice_BigBuffer * usb_midi_device[8] = {
  &midi01, &midi02, &midi03, &midi04, &midi05, &midi06, &midi07, &midi08,
};

uint64_t usb_midi_connected[8] = { 0,0,0,0,0,0,0,0 };
