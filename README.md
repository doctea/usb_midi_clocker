# usb_midi_clocker

proof-of-concept of clocking multiple USB MIDI devices and outputing Eurorack CV clock, using Arduino USB Host Shield on an Arduino Uno

- Works with Arturia Beatstep (requires receiving a MIDI start message externally before it will listen to external clock!)
- note: as of 2022-02-28, requires https://github.com/felis/USB_Host_Shield_2.0/pull/438 to be applied to the USB_Host_Shield_2.0 library if using Arturia Beatstep, otherwise it won't receive MIDI data or clock!

## Explanation

This allows an Arduino Uno with the USB Host Shield 2.0 to act as a USB *host* -- like an intelligent USB hub with multiple ports, so that USB MIDI devices connected to it (say keyboards or sequencers or synths) are sent MIDI clock.  

This video should demonstrate what I mean a bit:- https://photos.google.com/photo/AF1QipNL_kUVU3N1QzCV3z4exyQTlNcGLMTfbsU7jkwI

The Arduino you see at the start of the video has a "USB Host Shield 2.0" on top and then a DIY'd 4-output CV clock shield sitting on top of that.

There's an 8-port powered USB hub plugged into the port of the USB Host Shield. Connected to that USB hub is the Akai APCmini, the Beatstep, and another [https://github.com/doctea/drum2musocv](USB MIDI sequencer project of my own) (which in turn goes off to a Eurorack MIDI-CV interface and a BitBox mk2 sampler for syncing clock and sending triggers).

This project keeps time and sends clock divisions on the CV outs, but also detects the USB MIDI devices connected to the USB hub and does a different thing for each one:-

- For the Beatstep it sends MIDI start/stop and clock.
- For the Bamble it sends MIDI start/stop and clock.
- For the APCmini it sends a display indicating the settings of the clock outputs, receives MIDI messages from the APCmini to change those settings, and lights up to indicate the current step of the sequencer.
- There's also a function to resync - either immediately or at the start of the next bar -- that sends a stop followed immediately by a start, and resets the internal clock, so that everything restarts in sync at beat 1.

