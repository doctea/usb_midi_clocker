#include <UHS2-MIDI.h>
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
UHS2MIDI_NAMESPACE::uhs2MidiTransport *MidiTransports[] {&__uhs2Midi1, &__uhs2Midi2, &__uhs2Midi3};
