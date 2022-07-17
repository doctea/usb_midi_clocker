- This branch for porting to Teensy 4.1 using [Deftaudio 8x8 midi+usb breakout board](https://www.tindie.com/products/deftaudio/teensy-41-midi-breakout-board-8in-8out-usb-host/)
- Look in branches for the version that works on an Arduino Uno with a USB Host Shield.  Worked pretty well, but wanted to expand it to do more advanced sequencing and deal with more devices, so this branch is for porting to Teensy instead, with many more features.

# usb_midi_clocker

The purpose is to make use of USB-MIDI devices (eg Akai APCMini and Arturia Beatstep), acting as master clock for USB MIDI, to output Eurorack CV clock, with shiftable generator/divider, and other neat features like a sequencer, looping, stuff like that..

(Way more than a) proof-of-concept of clocking multiple USB MIDI devices and outputting Eurorack CV clock, using a Teensy 4.1 mounted to a Deftaudio 8x8 breakout board (available on Tindie).  

- Works with Arturia Beatstep (requires receiving a MIDI start message externally before it will listen to external clock!)
- Note: as of 2022-04-25, needs patched version of the usbhost_t36 library from here https://github.com/doctea/USBHost_t36/tree/fixing-midi-clock due to https://github.com/PaulStoffregen/USBHost_t36/issues/86

## Requirements

- Teensy 4.1 with Deftaudio 8x8 midi board
- ST7789 oled screen (Adafruit is the one i'm using) + rotary encoder + two wired buttons for control
- https://github.com/doctea/mymenu
- Rotary encoder
- SD card in the onboard Teensy SD card reader
- DIY'd circuit to shift 3.3v Teensy IO up to 5v to be used as clock/sequencer triggers
- Note: as of 2022-04-25, needs patched version of the usbhost_t36 library from here https://github.com/doctea/USBHost_t36 due to https://github.com/PaulStoffregen/USBHost_t36/issues/86

## Features

- Support USB MIDI devices (more can be added in code):
  - Arturia BeatStep (original, clock+note routing/transposition)
  - Arturia KeyStep (clock)
  - Akai MPK49 (clock+MIDI looping functions)
  - Akai APCMini (used as interface for sequencer+clock divisions)
  - [Bambleweeny 57](https://github.com/doctea/drum2musocv/) (clock)
  - A [mystery device](https://github.com/doctea/usb_midi_clocker/tree/arduino_version) with USB MIDI VID=0x1337 PID=0x1337
  - Modal CraftSynth 2.0
- Feedback & configuration via UI
  - ST7899 display using ST7899_t3 for info + [custom menu system](https://github.com/doctea/mymenu)
  - Controlled with rotary encoder + buttons and via APCMini 
- Option to sync to external MIDI clock and obey start/stop from USB host (ie PC)
- Or play from internal clock, with BPM controlled by APCMini slider
  - With APCMini shortcuts to 'send restarts' to synced devices to get everything in sync
- CV outputs for clock divisions and sequencer tracks
  - on Teensy's digital pins via level shifter *TODO:* schematic/instructions- 
  - 4 tracks of an 8-beat sequencer, controllable via APCMini
    - trigger once per beat, twice per beat, 4 times per beat, 0 times per beat
  - 4 clock division outputs, configurable on-the-fly via APCMini
    - 32nds, 16ths, 8ths, 4ths, whole note, half bar, bar, 2-bar, phrase, two phrases
    - offset up to +/-7 beats from start of phrase
  - Save sequences and clock settings to SD card
  - 8 save slots for sequencer+clock settings per project
- Multiple (effectively unlimited?) projects
- Configurable MIDI device routing (device:channel input to device:channel output)
- Transposition of routed MIDI (eg currently maps all notes incoming from Beatstep to C2-C3 range on MIDI serial output that goes to my Neutron)
- Autoplay through project sequences ie 'song mode', selectable from menu
- Record incoming MIDI loops from input
  - configurable transposition
  - configurable destination
  - store to SD card
  - with quantization on recording and recall
  - 8 slots per project

### TODO/Future 

- Update docs to reflect all features
- ~~Enable switching between multiple projects~~ done
- ~~Port to Teensy 4.1 and~~ (done - teensy version is now the main branch!)
- Merge functionality with [drum2musocv Bamblweeny](https://github.com/doctea/drum2musocv)
  - Or at least add some controls via the APCMini sliders, eg over the envelopes
- Get it working in uClock/interrupts mode without crashes
- ~~Sync from external input (MIDI)~~ done
  - Sync from external input (CV)
- ~~More outputs~~ - done, now works with 4 clock outs and 4 separate sequencer track outs
- ~~MIDI DIN or TRS input + output~~ now using Deftaudio 8x8 interface board so get 8 MIDI DIN-ins and 8 MIDI DIN-outs!
- Better sequencer
  - better how?
  - configurable chaining of sequences ie 'song mode'?
- ~~Configurable routing of MIDI data and notes from device X to device Y?~~ Basically done but UI could probably do with improvement
- Sequencer that records and playback MIDI notes or CV
  - ~~rudimentary MIDI looper~~ working, and saves to SD
  - improve by writing & saving real MIDI files?
  - Genericise MIDI looper functionality, so can eg record and loop drums 
- ~~TFT display~~ (working now using the Adafruit ST7789_t3 library and [mymenu](https://github.com/doctea/mymenu))
- ~~Encoder for controlling options and parameters~~
- Visual control over the features of the [drum2musocv Bamblweeny](https://github.com/doctea/drum2musocv)?
- Give better control over Beatstep via sysex if possible?
  - ie, change pattern, change speed..?
- Better way to define custom behaviour in order to add new USB MIDI devices
- Configurable per-device MIDI clock divisions/multiplier
  - ~~Done for Subclocker and saves with Project settings~~ -- maybe it should save with pattern instead though - could then ?
- ~~Save and recall MIDI device+channel routings~~
  - Improve saving and recalling of MIDI device+channel routing, rather than having it hardcoded
- Full configuration of MIDI device+channel routing


## Explanation/demo

(Note that the below mostly applies to the Arduino Uno version, Teensy version has a lot more going on)

This allows a Teensy 4.1 equipped with a Deftaudio 8x8 MIDI+USB interface board to act as a USB MIDI *host* -- like an intelligent USB hub with multiple ports, so that USB MIDI devices connected to it (say keyboards or sequencers or synths) are sent MIDI clock.  This way you can sync USB MIDI devices, that don't have a normal MIDI DIN input, without connecting to a computer for DAWless usage.  Also outputs Eurorack CV clock/sequences.

This video should demonstrate what I mean a bit (old video from the Arduino Uno+USB Host Shield version!):- https://photos.google.com/photo/AF1QipNL_kUVU3N1QzCV3z4exyQTlNcGLMTfbsU7jkwI

The Arduino you see at the start of the video has a "USB Host Shield 2.0" on top and then a DIY'd 4-output CV clock shield sitting on top of that.

There's an 8-port powered USB hub plugged into the port of the USB Host Shield. Connected to that USB hub is the Akai APCmini, the Beatstep, and another [USB MIDI sequencer project of my own](https://github.com/doctea/drum2musocv) (which in turn goes off to a Eurorack MIDI-CV interface and a BitBox mk2 sampler for syncing clock and sending triggers).

This project keeps time and sends clock divisions on the CV outs, but also detects the USB MIDI devices connected to the USB hub and does a different thing for each one:-

- For the Beatstep, Keystep, Bamble, MPK49 and Subclocker it sends MIDI start, stop and clock.
- Configurable divider and delay for the Subclocker.
- For the APCmini it sends a display indicating the settings of the clock outputs, receives MIDI messages from the APCmini to change those settings, and lights up to indicate the current step of the sequence.
- Can use the 'master' fader on the APCmini to change tempo.
- There's also a function to resync - either immediately or at the start of the next bar -- that sends a stop followed immediately by a start to the MIDI devices, and resets the internal clock, so that everything restarts in sync (hopefully!) at beat 1.

It also has a very rudimentary sequencer, using Akai APCMini for input+display, currently overlaid over the clock outputs.
