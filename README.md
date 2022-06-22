- This version is for running on an Arduino Uno with a USB Host Shield 2.0.  I'm now working mostly on the Teensy version but this branch might still be of interest and use to someone.
- This branch now includes optional experimental uClock mode -- which works so long as you don't send any CC messages :( https://github.com/midilab/uClock/issues/4#issuecomment-1066212517
- Non-uClock mode seems stable, but sending CC messages (eg faders on AKAI APCMini) causes noticeable stutter, lag and timing problems
- Can sync to incoming clock from a USB MIDI host (ie, PC, or maybe the Teensy version of this same app!)
- Look in Config.h to configure features & settings, including turning on USB Host mode!
- Get in touch if you are interested in using or contributing

# usb_midi_clocker

Proof-of-concept of clocking multiple USB MIDI devices and outputing Eurorack CV clock, using Arduino USB Host Shield on an Arduino Uno.  The purpose is to make use of USB-MIDI devices (eg Akai APCMini and Arturia Beatstep), acting as master clock for USB MIDI and Eurorack CV clock, with shiftable generator/divider.

- Works with Arturia Beatstep (requires receiving a MIDI start message externally before it will listen to external clock!)
- note: as of 2022-02-28, requires https://github.com/felis/USB_Host_Shield_2.0/pull/438 to be applied to the USB_Host_Shield_2.0 library if using Arturia Beatstep, otherwise it won't receive MIDI data or clock!
- now syncs with external MIDI clock over incoming USB serial from host (check Config.h to switch between using Hairless Serial Bridge and real MIDI baudrate

## Explanation

This allows an Arduino Uno with the USB Host Shield 2.0 to act as a USB *host* -- like an intelligent USB hub with multiple ports, so that USB MIDI devices connected to it (say keyboards or sequencers or synths) are sent MIDI clock.  This way you can sync USB MIDI devices, that don't have a normal MIDI DIN input, without connecting to a computer for DAWless usage.

This video should demonstrate what I mean a bit:- https://photos.google.com/photo/AF1QipNL_kUVU3N1QzCV3z4exyQTlNcGLMTfbsU7jkwI

The Arduino you see at the start of the video has a "USB Host Shield 2.0" on top and then a DIY'd 4-output CV clock shield sitting on top of that.

There's an 8-port powered USB hub plugged into the port of the USB Host Shield. Connected to that USB hub is the Akai APCmini, the Beatstep, and another [USB MIDI sequencer project of my own](https://github.com/doctea/drum2musocv) (which in turn goes off to a Eurorack MIDI-CV interface and a BitBox mk2 sampler for syncing clock and sending triggers).

This project keeps time and sends clock divisions on the CV outs, but also detects the USB MIDI devices connected to the USB hub and does a different thing for each one:-

- For the Beatstep it sends MIDI start/stop and clock.
- For the Bamble it sends MIDI start/stop and clock.
- For the APCmini it sends a display indicating the settings of the clock outputs, receives MIDI messages from the APCmini to change those settings, and lights up to indicate the current step of the sequence.
- Can use the 'master' fader on the APCmini to change tempo.
- There's also a function to resync - either immediately or at the start of the next bar -- that sends a stop followed immediately by a start to the MIDI devices, and resets the internal clock, so that everything restarts in sync (hopefully!) at beat 1.

It also has a very rudimentary sequencer, using Akai APCMini for input+display, currently overlaid over the clock outputs.

### TODO/Future 

- Port to Teensy 4.1 DONE in the main branch where most dev will continue - Uno became too limited for what I wanted to do
- Merge functionality with [drum2musocv Bamblweeny](https://github.com/doctea/drum2musocv)
- Get it working in uClock mode without crashes
- Sync from external CV
- More outputs
- MIDI DIN or TRS input + output
- Better sequencer
- Configurable routing of MIDI data and notes from device X to device Y?
- Sequencer that records and playback MIDI notes or CV
- Visual control over the features of the [drum2musocv Bamblweeny](https://github.com/doctea/drum2musocv)?
