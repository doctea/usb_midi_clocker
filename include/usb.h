/*#include <UHS2-MIDI.h>
#include <usbhub.h>

USB Usb;
USBHub  Hub1(&Usb);
USBHub  Hub2(&Usb);

#define NUMBER_OF_DEVICES 3
UHS2MIDI_CREATE_INSTANCE(&Usb, 0, Midi1);
UHS2MIDI_CREATE_INSTANCE(&Usb, 0, Midi2);
UHS2MIDI_CREATE_INSTANCE(&Usb, 0, Midi3);

// The Midi[] array holds the MidiInterface objects (that you can call sendNoteOn(), sendClock(), setHandleNoteOn() etc on)
MIDI_NAMESPACE::MidiInterface<UHS2MIDI_NAMESPACE::uhs2MidiTransport> *Midi[] {&Midi1, &Midi2, &Midi3};
//The instance name of uhs2MidiTransport is prefixed with __uhs2.  Can also fetch this from eg Midi1->getTransport()
UHS2MIDI_NAMESPACE::uhs2MidiTransport *MidiTransports[] {&__uhs2Midi1, &__uhs2Midi2, &__uhs2Midi3};*/

#include <MIDI.h>
#include <USBHost_t36.h> // access to USB MIDI devices (plugged into 2nd USB port)
/*
// Create the Serial MIDI ports
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI1);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI2);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial3, MIDI3);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial4, MIDI4);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial5, MIDI5);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial6, MIDI6);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial7, MIDI7);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial8, MIDI8);
*/

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

uint16_t usb_midi_connected[8] = { 0,0,0,0,0,0,0,0 };