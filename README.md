- This branch (main) is for a Teensy 4.1 using the [Deftaudio 8x8 midi+usb breakout board](https://www.tindie.com/products/deftaudio/teensy-41-midi-breakout-board-8in-8out-usb-host/)
- The [Arduino branch](https://github.com/doctea/usb_midi_clocker/tree/arduino_version) is lacking most of the extended features, but works on an Arduino Uno with a USB Host Shield.  (The Teensy version supports connecting a Uno running the Arduino version as a USB MIDI device to add extra clock outputs - use [USBMidiKliK](https://github.com/TheKikGen/USBMidiKliK) to turn the Arduino into a native USB MIDI device and configure it with pid=0x1337, vid=0x1337).

# usb_midi_clocker

This can be used as the clock generator and a USB MIDI hub to your Eurorack system, and more.  It allows making use of USB-MIDI devices (eg Akai APCMini and MPK49, original Arturia Beatstep and Keystep, Modal CraftSynth 2.0), syncing USB and din MIDI devices as well as Eurorack CV clock and triggers on multiple outputs with selectable divisions and sequencer controlled by APCMini interface.

Uses a TFT screen and a rotary encoder and buttons to control options.  Saves to SD with functions such as unlimited projects, per-project sequences that store and recall clock divisions and sequencer patterns, a MIDI looper, etc.  Option to auto-advance through these sequences/loops for rudimentary song chaining.

## Features

- Support for USB MIDI devices (more can be added in code):
  - Arturia BeatStep (original, clock+note routing/transposition, auto-advance pattern (see Known issues))
  - Arturia KeyStep (send clock, accept input)
  - Akai MPK49 (clock+MIDI looping functions, accept input)
  - Akai APCMini (used as interface for sequencer+clock divisions)
  - [Bambleweeny 57](https://github.com/doctea/drum2musocv/) (clock)
  - A [mystery device](https://github.com/doctea/usb_midi_clocker/tree/arduino_version) with USB MIDI VID=0x1337 PID=0x1337
  - Modal CraftSynth 2.0 (route input/looper to play CraftSynth notes)
- Support for MIDI DIN devices (clock and note in/outs), eg
  - Route USB MIDI from Beatstep to DIN Behringer Neutron, while transposing all notes to target octave range
  - Sync a 1010 bitbox mk2 with clock
  - Route a MIDI drumkit to my drum2musocv MidiMuso CV-12 triggers
  - Route MIDI input from a [LeStrum](https://www.tindie.com/products/hotchk155/le-strum-midi-strummed-chord-controller-kit/) to MIDI->CV interface or other synth
  - Route MIDI to/from a Disting Ex with the Expert Sleepers MIDI breakout board
- Re-sync clocked devices 'now' or 'on next bar start' (using ACPMini shift+Up and shift+Device respectively) that resets internal clock and sends stop/start messages to attached MIDI devices
- Feedback & configuration via UI
  - ST7899 display using ST7899_t3 for info + [custom menu system](https://github.com/doctea/mymenu)
  - Controlled with rotary encoder + buttons and via APCMini 
- Clock sync options
  - Sync to external MIDI clock and obey start/stop from USB host (ie sync to PC/DAW)
  - Or run from internal clock, with BPM controlled by APCMini slider
  - APCMini shortcuts to 'send restarts' to synced devices to get everything in sync
- CV outputs for clock divisions and sequencer tracks
  - on Teensy's digital pins via level shifter *TODO:* schematic/instructions 
  - 4 tracks of an 8-beat sequencer, configurable on-the-fly via APCMini
    - trigger once per beat, twice per beat, 4 times per beat, 0 times per beat
  - 4 clock division outputs, configurable on-the-fly via APCMini
    - 32nds, 16ths, 8ths, 4ths, whole note, half bar, bar, 2-bar, phrase, two phrases
    - offset up to +/-7 beats from start of phrase
- Configurable MIDI device routing (device:channel input to device:channel output)
- Save sequences, clock settings and device routings to SD card
  - Multiple (effectively unlimited?) projects
  - 8 save slots for sequencer+clock settings per project
  - auto-advance sequence option
- Transposition/'octave-locking' of routed MIDI (eg currently maps all notes incoming from Beatstep to C2-C3 range on MIDI serial output that goes to my Neutron)
- Autoplay through project sequences ie 'song mode', selectable from menu
- Record incoming MIDI loops from input
  - configurable transposition
  - configurable destination
  - store to SD card
  - with quantization on recording and recall
  - transpose loop (chromatic)
  - 8 slots per project
  - auto-advance loops option

## Requirements

- Teensy 4.1
  - Deftaudio 8x8 midi board (or DIY'd serial MIDI ins&outs)
- ST7789 oled screen + rotary encoder + two wired buttons for control
  - small screen option https://shop.pimoroni.com/products/adafruit-1-14-240x135-color-tft-display-microsd-card-breakout-st7789
  - larger screen option https://www.amazon.co.uk/Waveshare-TFT-Touch-Shield-Resolution/dp/B00W9BMTVG using "ST7789_t3_Big" menu
  - https://github.com/doctea/mymenu
- Rotary encoder and some buttons for controlling the screen and options
- Akai APCMini for controlling the sequencer and clocks
- SD card in the onboard Teensy SD card reader
- DIY'd circuit to shift 3.3v Teensy IO up to 5v to be used as clock/sequencer triggers
  - I'm using a couple of these https://shop.pimoroni.com/products/sparkfun-logic-level-converter-bi-directional?variant=7493045377
- Note: as of 2022-04-25, needs patched version of the usbhost_t36 library from here https://github.com/doctea/USBHost_t36 due to https://github.com/PaulStoffregen/USBHost_t36/issues/86

### Known issues (current)

- BeatStep auto-advance via SYSEX is unreliable/non-working -- had it working a few times, but couldn't work out rhyme or reason why it randomly stops working?  Left in as option.. maybe related to the same strange USB MIDI glitches as above.
 - Device stops responding/sending clocks for a moment while a new USB device is initialised -- think this is a limitation of the underlying library

## Known issues (may be solved)

- Although it has been known run for many hours solidly, been getting some occasional crashes lately (especially while working on the looper code, though unsure if this is related).  Might be because of bugs in my attempt to patch the problem with the USBHost_t36 USB MIDI clock problem...
- MIDI loop output to USB devices behaves very strangely... missed notes, stuck notes, glitching.... but it works more reliably if host is connected to the debug serial output?!  Think maybe a problem in the underlying USBHost_t36 code?
  - notes work fine though if sending clock is disabled?!
  - and MIDI that comes in from USB device eg beatstep and then sent to the same output device seems to work ok regardless?
  - something to do with USB/MIDI buffers I guess?  maybe isn't getting flushed properly or messages get trashed before being sent?
  - but why would that only affect loops that are playing back?
  - really don't know what's going on here :(
  - workaround for the time being is to use hardware serial MIDI when notes+clock are needed
  - hmm, may have worked around this by sending looper notes before sending clocks, in a do_pre_clock() function...?
    - OK so yeah, problem seems to have been solved by sending the note on/offs in a separate loop before then sending all the clocks in a separate loop.


### TODO/Future 

- Come up with a cooler name
- Update docs to reflect all features
- ~~Enable switching between multiple projects~~ done
- ~~Port to Teensy 4.1 and~~ (done - teensy version is now the main branch!)
- Visual control over the features of the [drum2musocv Bamblweeny](https://github.com/doctea/drum2musocv)?
- Merge functionality with [drum2musocv Bamblweeny](https://github.com/doctea/drum2musocv)
  - eg Euclidian sequencer
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
  - ~~rudimentary MIDI looper~~ working, and saves to SD, 8 slots per project, with auto-advance
  - improve by writing & saving real MIDI files?
  - Genericise MIDI looper functionality, so can eg record and loop drums 
- ~~TFT display~~ (working now using the Adafruit ST7789_t3 library and [mymenu](https://github.com/doctea/mymenu))
- ~~Encoder for controlling options and parameters~~
- Give better control over Beatstep via sysex if possible?
  - added auto-advance pattern mode, but isn't working reliably -- might be something weird with the USB MIDI problems discovered mentioned in Known issues?
  - ie, change pattern, change speed..?
- Better way to define custom behaviour in order to add new USB MIDI devices
- Configurable per-device MIDI clock divisions/multiplier
  - ~~Done for Subclocker and saves with Project settings~~ -- maybe it should save with pattern instead though - could then ?
- ~~Save and recall MIDI device+channel routings~~
  - Improve saving and recalling of MIDI device+channel routing, rather than having it hardcoded
- Full configuration of MIDI device+channel routing
- Allow input/loops to be redirected to multiple MIDI outputs.
- Output MIDI notes from the sequencer - so eg, assign kick to sequencer track#1, snare to sequencer track#2, output appropriate note on/offs on channel 10
- CV to MIDI for modulating MIDI devices from Eurorack CV (eg modulate the cutoff on CraftSynth from incoming CV; use the [parameters](https://github.com/doctea/parameters) library to do this)
- Record and playback CCs
- Option to 'lock/hold current' clock/sequencer/MIDI mapping settings etc when switching presets

## Explanation/demo

(Note that the below mostly applies to the Arduino Uno version, Teensy version has a lot more going on)

Turning into a bit of a sequencer/MIDI+USB interface/brain for controlling Eurorack and MIDI devices.

This allows a Teensy 4.1 equipped with a Deftaudio 8x8 MIDI+USB interface board to act as a USB MIDI *host* -- like an intelligent USB hub with multiple ports, so that USB MIDI devices connected to it (say keyboards or sequencers or synths) are sent MIDI clock.  This way you can sync USB MIDI devices, that don't have a normal MIDI DIN input, without connecting to a computer for DAWless usage.  Also outputs Eurorack CV clock/sequences.

This video should demonstrate what I mean a bit (old video from the Arduino Uno+USB Host Shield version!):- https://photos.google.com/photo/AF1QipNL_kUVU3N1QzCV3z4exyQTlNcGLMTfbsU7jkwI

The Arduino you see at the start of the video has a "USB Host Shield 2.0" on top and then a DIY'd 4-output CV clock shield sitting on top of that.

There's an 8-port powered USB hub plugged into the port of the USB Host Shield. Connected to that USB hub is the Akai APCmini, the Beatstep, and another [USB MIDI sequencer project of my own](https://github.com/doctea/drum2musocv) (which in turn goes off to a Eurorack MIDI-CV interface and a BitBox mk2 sampler for syncing clock and sending triggers).

This project keeps time and sends clock divisions on the CV outs, but also detects the USB MIDI devices connected to the USB hub and does a different thing for each one:-

- For the Beatstep, Keystep, Bamble, MPK49 and Subclocker it sends MIDI start, stop and clock.
- Modal CraftSynth 2.0 detected and used as a MIDI/looper output option.
- Configurable divider and delay for the Subclocker.
- MPK49 input can be recorded + saved as loops to be replayed out to selected device.
- For the APCmini it sends a display indicating the settings of the clock outputs, receives MIDI messages from the APCmini to change those settings, and lights up to indicate the current step of the sequence.
- Can use the 'master' fader on the APCmini to change tempo.
- There's also a function to resync - either immediately or at the start of the next bar -- that sends a stop followed immediately by a start to the MIDI devices, and resets the internal clock, so that everything restarts in sync (hopefully!) at beat 1.

It also has a very rudimentary sequencer, using Akai APCMini for input+display, currently overlaid over the clock outputs.
