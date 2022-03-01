# usb_midi_clocker

proof-of-concept of clocking multiple USB MIDI devices and outputing Eurorack CV clock, using Arduino USB Host Shield on an Arduino Uno

- Works with Arturia Beatstep (requires receiving a MIDI start message externally before it will listen to external clock!)
- note: as of 2022-02-28, requires https://github.com/felis/USB_Host_Shield_2.0/pull/438 to be applied to the USB_Host_Shield_2.0 library if using Arturia Beatstep, otherwise it won't receive MIDI data or clock!

