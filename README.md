- This branch for porting to Teensy 4.1 using [Deftaudio 8x8 midi+usb breakout board](https://www.tindie.com/products/deftaudio/teensy-41-midi-breakout-board-8in-8out-usb-host/)
- ~~This branch now includes optional experimental uClock mode -- which works so long as you don't send any CC messages :( https://github.com/midilab/uClock/issues/4#issuecomment-1066212517~~
- Non-uClock mode seems stable, but sending CC messages (eg faders on AKAI APCMini) causes noticeable stutter, lag and timing problems
- This is the version that works on an Arduino Uno with a USB Host Shield.  Worked pretty well, but wanted to expand it to do more advanced sequencing and deal with more devices, now porting to [teensy8x8 branch](https://github.com/doctea/usb_midi_clocker/tree/teensy_8x8) instead.

# usb_midi_clocker

Proof-of-concept of clocking multiple USB MIDI devices and outputing Eurorack CV clock, using a Teensy 4.1 mounted to a Deftaudio 8x8 breakout board (available on Tindie).  The purpose is to make use of USB-MIDI devices (eg Akai APCMini and Arturia Beatstep), acting as master clock for USB MIDI, to output Eurorack CV clock, with shiftable generator/divider, and other neat features like sequencer, looping, stuff like that..

- Works with Arturia Beatstep (requires receiving a MIDI start message externally before it will listen to external clock!)
- Note: as of 2022-04-25, needs patched version of the usbhost_t36 library from here https://github.com/doctea/USBHost_t36 due to https://github.com/PaulStoffregen/USBHost_t36/issues/86

## Explanation

This allows a Teensy 4.1 equipped with a Deftaudio 8x8 MIDI+USB interface board to act as a USB MIDI *host* -- like an intelligent USB hub with multiple ports, so that USB MIDI devices connected to it (say keyboards or sequencers or synths) are sent MIDI clock.  This way you can sync USB MIDI devices, that don't have a normal MIDI DIN input, without connecting to a computer for DAWless usage.  Also outputs Eurorack CV clock/sequences.

This video should demonstrate what I mean a bit (old video from the Arduino Uno+USB Host Shield version!):- https://photos.google.com/photo/AF1QipNL_kUVU3N1QzCV3z4exyQTlNcGLMTfbsU7jkwI

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

- ~~Port to Teensy 4.1 and~~ (done!)
- Merge functionality with [drum2musocv Bamblweeny](https://github.com/doctea/drum2musocv)
- Get it working in uClock/interrupts mode without crashes
- Sync from external input (MIDI and CV)
- More outputs - done now works with 4 clock outs and 4 separate sequencer track outs
- ~~MIDI DIN or TRS input + output~~ Using Deftaudio 8x8 interface board so get 8 MIDI DIN-ins and 8 MIDI DIN-outs!
- Better sequencer
  - better how?
- Configurable routing of MIDI data and notes from device X to device Y?
- Sequencer that records and playback MIDI notes or CV
  - rudimentary MIDI looper working
- TFT display (basically working now using the Adafruit ST7789_t3 library)
- Encoder for controlling options and parameters
- Visual control over the features of the [drum2musocv Bamblweeny](https://github.com/doctea/drum2musocv)?

