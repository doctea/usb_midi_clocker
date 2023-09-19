![eurorack prototype](https://github.com/doctea/usb_midi_clocker/blob/main/media/eurorack%20prototype.jpg)

- This branch is for a Teensy 4.1
  - 'prototype' build profile uses the [Deftaudio 8x8 midi+usb breakout board](https://www.tindie.com/products/deftaudio/teensy-41-midi-breakout-board-8in-8out-usb-host/), with a st7789 screen wired up over SPI, Pimoroni +/-24v ADC over i2c, and gate outputs on GPIOs
  - 'pcb' build profile uses prototype [Eurorack module boards](https://github.com/doctea/usb_midi_clocker_hardware), with an ili9341 screen, 2xPimoroni +/-24v ADC, and gate outputs on MCP23s17
- The old [Arduino branch](https://github.com/doctea/usb_midi_clocker/tree/arduino_version) is lacking most of the features, but works on an Arduino Uno with a USB Host Shield, CV outputs, and an Akai APCMini over USB.  (The Teensy version supports connecting a Uno running the Arduino version as a USB MIDI device to add extra clock outputs - use [USBMidiKliK](https://github.com/TheKikGen/USBMidiKliK) to turn the Arduino into a native USB MIDI device and configure it with pid=0x1337, vid=0x1337).
- This project should be considered work-in-progress/beta -- don't trust it for anything mission-critical!  Although it has been known to run for ~~12~~ 63 hour stretches without crashing, it undoubtedly has bugs and crashes (seem particularly common when the USB connection is a bit dodgy)

# usb_midi_clocker

This can be used as the clock generator and a USB-MIDI and DIN-MIDI hub for your Eurorack system, and more.

It allows making use of USB-MIDI devices (eg Akai APCMini and MPK49, original Arturia Beatstep and Keystep, Modal CraftSynth 2.0), syncing USB and DIN MIDI devices as well as Eurorack CV clock and triggers on multiple outputs with selectable divisions and sequencer controlled by APCMini interface.

Fundamentally I guess the aim is to tie together and make useful, in a Eurorack context, multiple cheap MIDI devices by clocking serial MIDI, USB MIDI, and eurorack from the same clock source.

Uses a TFT screen and a rotary encoder and buttons to control options.  Saves to SD with functions such as unlimited projects, per-project sequences that store and recall clock divisions and sequencer patterns, a MIDI looper, etc.  Option to auto-advance through these sequences/loops for rudimentary song chaining.

# Use and contributing

Both are encouraged, I would love to have this be useful to others and to accept contributions to add features, fix bugs, make it easier to use, tweakable for your needs.  Drop me a message or open an issue if you're thinking of giving it a try or need any assistance!

## Features

- Support for USB MIDI devices (more can be added in code):
  - Arturia BeatStep (original, clock+note routing/transposition, auto-advance pattern (see Known issues))
  - Arturia KeyStep (send clock, accept input)
  - Akai MPK49 (clock+MIDI looping functions, accept input)
  - Akai APCMini (used as interface for sequencer+clock divisions)
  - [Bambleweeny 57](https://github.com/doctea/drum2musocv/) (clock)
  - A [mystery device](https://github.com/doctea/usb_midi_clocker/tree/arduino_version) with USB MIDI VID=0x1337 PID=0x1337
    - With divider control and offset delay
  - Modal CraftSynth 2.0 (route an input or looper to play CraftSynth notes, with tempo sync for arpeggiation etc)
  - M-Vave/Cuvave Chocolate footswitch controller (controls the MIDI looper from left to right: play (toggle), record (toggle), overwrite (momentary), record (momentary))
    - Use the app to configure it as a custom config that outputs note on/note offs on notes 0,1,2,3
- Support for USB serial devices that don't present as proper MIDI devices, eg OpenTheremin, Arduino Uno with CH340, á la Hairless MIDI
- Send and receive MIDI events to host PC
- Support for MIDI DIN devices (clock and note in/outs), eg
  - Route USB MIDI from Beatstep to DIN Behringer Neutron, while transposing all notes to target octave range
  - Sync a 1010 bitbox mk2 with clock
  - Route a MIDI drumkit to my drum2musocv MidiMuso CV-12 triggers
  - Route MIDI input from a [LeStrum](https://www.tindie.com/products/hotchk155/le-strum-midi-strummed-chord-controller-kit/) to MIDI->CV interface or other synth
  - Route MIDI to/from a Disting Ex with the Expert Sleepers MIDI breakout board
- A MIDI routing matrix, to route notes from any of the configured sources (usb-midi, serial-midi, looper) and output it to any of the configured outputs (usb-midi, serial-midi, loopers)
- 'Bass drone' feature on the Neutron output where it'll drone on the first note played of a bar
- Per-device synced clock divider, pause, saved to sequencer pattern
- Re-sync clocked devices 'now' or 'on next bar start' (using ACPMini shift+Up and shift+Device respectively) that resets internal clock and sends stop/start messages to attached MIDI devices
- Feedback & configuration via UI
  - ST7899 display using ST7899_t3 library for info via [custom menu system](https://github.com/doctea/mymenu)
  - Controlled with rotary encoder + buttons, via APCMini and/or via a USB typing keyboard
- Clock sync options
  - Run from internal clock, with BPM controlled by APCMini slider
  - Or sync to external MIDI clock and obey start/stop from USB host (ie sync to PC/DAW)
  - APCMini shortcuts to 'send restarts' to synced devices to get everything in sync
- CV trigger outputs for clock divisions and sequencer tracks
  - on Teensy's digital pins, via level shifter (see wiring diagram/schematic below)
  - or via MCP23s17 over SPI on custom PCB
  - 4 tracks of an 8-beat sequencer, configurable on-the-fly via APCMini
    - trigger once per beat, twice per beat, 4 times per beat, 0 times per beat
  - 4 clock division outputs, configurable on-the-fly via APCMini
    - 32nds, 16ths, 8ths, 4ths, whole note, half bar, bar, 2-bar, phrase, every two phrases
    - offset up to +/-7 beats from start of phrase
   - extra dedicated reset outputs on PCB
- Save sequences, clock settings and device matrix routings to SD card
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
  - auto-advance looper option
  - control via the M-Vave footswitch buttons, including momentary hold for overwriting + recording
  - recorded notes are shown on display in piano roll style
  - playhead vertical line indicating playing / overwriting / recording / recording+overwriting
  - momentary overwrite mode that cuts notes
- CV to MIDI CC 
  - Allow CV input to be mapped to modulate CC outputs or internal parameters
  - ~~Only CraftSynth currently has Parameters~~
  - Working 1V/oct input that can be routed to MIDI outputs; ~~some latency though~~
  - Quantise pitches to selectable scale
  - Outputs selectable/modulatable chords and inversions

### Behaviours

- behaviour_apcmini: for Akai APCMini as sequencer/clock sequencer
- behaviour_bamble: sends clock and sets up some parameters for [drum2musocv](https://github.com/doctea/drum2musocv)
- behaviour_beatstep: for Arturia Beatstep to sync clock and send MIDI notes through to other devices
- behaviour_bitbox: send clock to 1010 Music Bitbox mk2, also notes, over midi DIN
- behaviour_chocolate: for the M-Vave Chocolate footswitch to control looper
- behaviour_clocked: base classes for handling midi clock delay and divison
- behaviour_craftsynth: send notes to the Modal CraftSynth 2.0, also CC parameters that can be modulated from CV input
- behaviour_drumkit: input from midi drumkit over DIN
- behaviour_dptlooper: send clock and bar/phrase starts for [DPTLooper](https://github.com/doctea/dptlooper) [experimental/WIP]
- behaviour_keystep: sends clock and receives notes
- behaviour_lestrum: input from LeStrum on both channels
- behaviour_microlidian: send clock to [Microlidian](https://github.com/doctea/Microlidian) device
- behaviour_microlidian: send clock to [MIDILights](https://github.com/doctea/midilights) device
- behaviour_mpk49: input from Akai MK49, including control over looper
- behaviour_neutron: bass drone, octave-forcing, and clock sent to Neutron over MIDI DIN
- behaviour_midibass: base classes for octave-locking and droning 
- behaviour_opentheremin: accepts MIDI input from [OpenTheremin V4 with MIDI](https://github.com/MrDham/OpenTheremin_V4_with_MIDI)
- behaviour_subclocker: send clock to an attached Arduino usb_midi_clocker, with delay / clock division
- behaviour_base_serial: base class for DIN MIDI serial devices
- behaviour_base_usb: base class for USB MIDI devices
- behaviour_base_usbserial: base class for USB serial and USB serial MIDI devices
- ClockedBehaviour: send clock messages

## Requirements

- Teensy 4.1
  - Deftaudio 8x8 midi board (or DIY'd serial MIDI ins&outs) or [Eurorack module boards]([https://github.com/doctea/usb_midi_clocker_hardware](https://github.com/doctea/usb_midi_clocker_hardware))
- ST7789 or ILI9341 oled screen for display
  - st7789 option https://www.amazon.co.uk/Waveshare-TFT-Touch-Shield-Resolution/dp/B00W9BMTVG using "ST7789_t3_Big" menu (current prototype version much prefers this)
  - ILI9341 screen option https://www.aliexpress.com/item/1005003005413104.html 'Module With Touch' version for PCB
- Rotary encoder + two wired buttons for control and/or USB typing keyboard
  - and [mymenu](https://github.com/doctea/mymenu) library
- [midihelpers](https://github.com/doctea/midihelpers) library
- Akai APCMini for controlling the sequencer and clocks (although not needed for running the rest of it)
- SD card in the onboard Teensy SD card reader, for saving projects, sequences, screenshots, loops and CV calibration
- Gate outputs
  - Prototype version: DIY'd circuit to shift 3.3v Teensy IO up to 5v to be used as clock/sequencer triggers, see 'Suggested wiring' section below
    - I am currently using one of these in prototype: [Level shifter breakout](https://coolcomponents.co.uk/products/level-shifter-8-channel-txs01018e) (see 'Suggested wiring', these need extra resistors in the output path to work properly!)
    - I was previously using a couple of these [Sparkfun level shifter](https://shop.pimoroni.com/products/sparkfun-logic-level-converter-bi-directional?variant=7493045377) -- these work reliably without needing the extra resistor on each output (although you probably should still add one)
  - Prototype Eurorack module for gate output using MCP23s17 (in theory supports 16 gates but currently seems flakey.  could perhaps be configured as inputs instead but haven't attempted this)
- For CV input: [Pimoroni +/-24v 1015 module](https://shop.pimoroni.com/products/ads1015-adc-breakout?variant=27859155026003) and my [parameters](https://github.com/doctea/parameters) library
- Note: as of 2022-04-25, needs patched version of the usbhost_t36 library from here https://github.com/doctea/USBHost_t36 due to https://github.com/PaulStoffregen/USBHost_t36/issues/86
  - on 2022-12-23, I recommend you use the maybe_fixed_4_stable branch as this seems like it might be more stable..?
- As of 2022-12-23, using [this LinkedList fork](https://github.com/vortigont/LinkedList) to try and improve stuff

## Eurorack modules / PCB / gerbers

- Experimental KiCad files + gerbers for PCB version at [https://github.com/doctea/usb_midi_clocker_hardware](https://github.com/doctea/usb_midi_clocker_hardware)
  - Some problems with these initial versions, but kinda working with a bit of hackery

## Suggested wiring - breadboard prototype

- Careful with this, consider it untested for now -- this is how I have mine connected up.  If you follow this then double-check that it actually makes sense when looking at the datasheets+pin diagrams for the actual Teensy and level-shifter boards before you power it on and risk frying pins on your Teensy!  I accept no responsibility if this breaks anything!

![example wiring](https://github.com/doctea/usb_midi_clocker/blob/main/media/suggested%20circuit%20connections.png "Suggested wiring diagram")

## Known issues (current)

- Occasional freezes/crashes... pretty sure related to something in the USB library
- GDB debug mode doesn't work - might just simply be because we don't have enough RAM to do this now?
- MIDI looper quantiser: Some notes get lost/mangled when quantising looper; need a bit cleverer logic to ensure that a playable note is always created
- Think it may be a couple of BPM slower in practice than what is actually set -- maybe rounding error in how tick length is calculated?  or due to a loop taking too long and missing the tick?
- ~~USBMIDI devices (CraftSynth, Beatstep) don't seem to receive Note messages sent from the 'CV Input' behaviour, while SerialMIDI devices eg Neutron and Bitbox play them without any problem.  Have checked and the messages /are/ being sent, with correct channel too, so bit of a mystery why they aren't being acted on..~~ <- hmm, fixed this by making CVInput behaviour send message to midi_matrix_manager on channel 0 instead of channel 1..?  but not sure why this would be, since the channel looked correct in DeviceBehaviourUSBBase#sendNoteOn() !
- Crash on trying to screenshot on ili9341

## Known issues (may be solved)

- Believe solved now (actual fix needed in parameters library, due to on_add not being called/uninitialised pointer to tft) Sometimes crashes the first time that one of the Parameter submenus is opened... but works after its rebooted..
- Although it has been known run for many hours solidly, been getting some occasional crashes lately (especially while working on the looper code, though unsure if this is related).  Might be because of bugs in my attempt to patch the problem with the USBHost_t36 USB MIDI clock problem...
- ~~MIDI loop output to USB devices behaves very strangely... missed notes, stuck notes, glitching.... but it works more reliably if host is connected to the debug serial output?!  Think maybe a problem in the underlying USBHost_t36 code?~~
  - notes work fine though if sending clock is disabled?!
  - and MIDI that comes in from USB device eg beatstep and then sent to the same output device seems to work ok regardless?
  - something to do with USB/MIDI buffers I guess?  maybe isn't getting flushed properly or messages get trashed before being sent?
  - but why would that only affect loops that are playing back?
  - really don't know what's going on here :(
  - workaround for the time being is to use hardware serial MIDI when notes+clock are needed
  - hmm, may have worked around this by sending looper notes before sending clocks, in a do_pre_clock() function...?
    - OK so yeah, problem seems to have been solved by sending the note on/offs in a separate loop before then sending all the clocks in a separate loop.
- ~~Beatstep sysex commands don't work, probably for the same reason?~~
- MIDI looper uses a LOT of RAM (~48k for 1 phrase -- 384 (ticks) * 127 (notes)) - less memory-hungry polyphony can be used, but drawing the pianoroll to screen would become more intensive...
- ~~Voltage Source Calibration UI is ugly / changing sizes constantly~~
- ~~BeatStep auto-advance via SYSEX is unreliable/non-working -- had it working a few times, but couldn't work out rhyme or reason why it randomly stops working?  Left in as option.. maybe related to the same strange USB MIDI glitches as mentioned below.~~

## Configuration

- see `include/Config.h` for lots of switches to enable/disable features, set pin assignments, and the like
- `include/BootConfig.h` to turn on 'wait for serial on boot'
- see also `include/ConfigMidi.h` for a few more switches
- `src/midi_mapper_matrix_manager.cpp` is where a lot of midi device input/output mappings are initialised and defaults assigned - tweak stuff here to match your available devices
- `src/behaviour_manager.cpp` is where devices are set up and configured, so this may need tweaking to match your hardware too
- `src/interfaces/interfaces.cpp` is where gate output/MCP23s17 gates are configured
- `src/menu.cpp` is where the display and menu system are initialised and menu items configured, so you may need to tweak stuff here too to match what other features you have enabled

### TODO/Future 

- Quantise all melodic outputs to a chosen scale
- Write up controls/instructions/etc
- Come up with a cooler name (maybe Nexus6 as that's what i've put on the pcb panel?)
- Update docs to reflect all features
- Merge functionality with [drum2musocv Bamblweeny](https://github.com/doctea/drum2musocv)
  - eg Euclidian sequencer
  - Am actually doing this in a separate module now, [Microlidian](https://github.com/doctea/Microlidian)
    - Move the Euclidian sequencer stuff from Microlidian into a library so that it could be used here as well
  - ~~Or at least add some controls via the APCMini sliders, eg over the envelopes~~
- Improve stability of clock by getting it working in uClock/interrupts mode without crashes
- Sync from external clock input (CV)
  - proof of concept implemented using the support in [midihelpers library](https://github.com/doctea/midihelpers), no reset functionality yet though
- ~~More outputs~~ - done, now works with 4 clock outs and 4 separate sequencer track outs
  - ~~add 2 more for use as resets~~
  - ~~add reset/further divisions support via the MCP23s17 gate support on PCB~~
  - make 8 clocks / 8 sequencer outputs controllable from APC Mini?
- Better sequencer
  - better how?
  - configurable chaining of sequences ie 'song mode'?
  - sequencer mutations / fills
  - allow offbeat tracks or individual steps
  - stutter trigger mode
- Sequencer/looper that records and playback CV/modulation
  - improve by writing & saving real MIDI files?
  - Make a looper manager or something to allow easier control of multiple loopers
  - Variable loop length
  - Variable repeats
  - more efficient memory/cpu usage
- Give better control over Beatstep via sysex if possible?
  - ~~auto-advance pattern mode~~
  - ie, ~~change speed..?~~
  - ~~note length~~
  - ~~legato~~
  - ~~shuffle~~
  - option to rotate sequence to change the downbeat - read in all the current pad values, ie rotate and write them back out
- ~~Better way to define custom behaviour in order to add new USB MIDI devices~#
  - ~~Partially done, can be improved further~~
  - Write up docs on how to add a new device behaviour
- Configurable per-device MIDI clock divisions/multiplier
  - tricks like 'half-time beatstep for last bar of phrase'
    - DividedClockBehaviours already do this?
  - Faster-than-BPM multipliers..?
- Full configuration of MIDI device+channel routing
- Output MIDI notes from the clock/trigger sequencer - so eg, assign kick to sequencer track#1, snare to sequencer track#2, output appropriate note on/offs on channel 10
- CC modulation
  - ~~CV-to-MIDI, for modulating MIDI devices from Eurorack CV (eg modulate the cutoff on CraftSynth from incoming CV; use the [parameters](https://github.com/doctea/parameters) library to do this)~~
    - this is mostly working now..!
      - some performance issues in reading the data, though
        - maybe if the performance problem is actually with the reading of the data, then we could hand that off to a 328p on a nano/uno to do the raw ADC processing, and communicate to the teensy via uart, or even usbserial now?
        - (or just get a dedicated CV-to-MIDI module and feed it in to the MIDI inputs)
    - some todos remain:
      - ~~reassign modulation sources~~
      - ~~load/save modulation settings~~
      - lock/hold modulation settings
      - auto-advance modulation settings?
      - modulation sources other than voltages
        - ~~MIDI CCs~~ done for first 4 faders
      - tempo-synced LFOs etc...
      - Record and playback CCs as well as MIDI
- ~~Option to 'lock/hold current' clock/sequencer/MIDI mapping settings etc when switching presets~~
- Subclocker clock multipliers as well as division (need to calculate time between ticks, and send clock on steps in between...)
- Looper improvements
  - Improve quantizer (take note length into consideration)
  - Optimise memory usage with more efficient data structure
- MIDI control over [r_e_c_u_r](https://github.com/cyberboy666/r_e_c_u_r)
- TODO: usbserial for Arduino Unos without needing to use USB Midi Klik 
  - todo: see whether we need to use device unique id rather than pid+vid to ensure correct detection of mltiple devices
  - or for things that don't even talk MIDI, like eg my [veboard](https://github.com/doctea/veboard) project, or even direct Panasonic MX serial control
  - todo: implement a proof-of-concept non-MIDI usb serial device
- Transposition of everything (Beatstep etc?) in chord progressions
- Make the pc_usb connections work using behaviours
- Make the MIDI looper use behaviours
- ~~Allow to control via USB typing keyboard~~
  - ~~control the menus so that I can still fiddle with the device without disturbing my cat~~
  - shortcuts for different functions would make this something that could be performed with
  - also opens the door to allow naming of patterns/projects?
- maybe even also add VGA/HDMI output so that we're not tied to a tiny little screen..?
- Figure out how to better support MIDI features like:-
  - NPRNs
  - ~~Pitch bend~~ kinda support this now -- it is passed through at least, and allowed to be Proxied to add modulation on top of it
    - might be good to be able to enable/disable it for certain mappings tho?
  - Aftertouch
- Allow Behaviours to register CCs they respond to and can generate, so that can remap these as sources and targets
  - ~~eg remap Bamble envelopes to CraftSynth cutoff, etc...~~
    - this should be possible already, as Microlidian envelopes are exposed as modulation sources, and CraftSynth cutoff can have modulation sources applied to them
  - expose internal settings as Parameters, so that can have modulation mapped to them too...
- Reassignable Parameters that can be pointed at any given Behaviour+CC, configured from menus instead of code
  - expose internal settings (like bpm, clock delays...?) as Parameters, so that can have modulation mapped to them too...
- Reassignable ParameterInputs, that can be set to take their values from an incoming MIDI stream
  - Register them with the midi_matrix_mapper_manager so that when it receives a CC value change, it updates the ParameterInput current_value
- Beatstep features
  - 'offline editing' features, since going realtime fucks up sync too badly
  - like being able to load + save sequences, rotate sequences
- CraftSynth
  - machinegun mode didn't work when I previously tried to add it to CraftSynth - why?
  - CraftSynth is USB while Neutron is MIDISerial, maybe something in that?
- BitBox features:
  - sequence triggering pads via midi
  - play pads from APC Mini?
  - clocked control
  - record commands
- ~~Bambleweeny~~ kinda moved on from this to Microlidian now, which has similar features but better
  - add the other envelope controls
  - make the UI nicer
- global 'enable recall' panels to freeze certain parameters
- 'stick on sequence' buttons
  - A+B bounce sequences
  - stick count 
  - non-linear sequence progression
- gate inputs via the mcp23s17, if the hardware is working to support that
  - has test code support in MCP23S17InputBankInterface+TEST_MCP23s17_INPUT flag, but hardware seems to let us down by reading high whenever something is connected..?
  - take midi clock and reset from gate inputs
- CV Input behaviour:
  - support multiple inputs, so it can play chords, with note allocation
  - ~~add scale-quantisation~~
  - ~~velocity from second selectable input~~ (needs testing with device that supports velocity to make sure its working!)
  - ~~play chords from scale from root note~~
  - ~~choose chord type to play from another CV input~~
  - Arpeggiator 
  - Inversions
  - Move quantising/chord generation stuff into a more general 'MIDI filter' behaviour type that can be chained with other behaviours?
  - Port quantising/chord options to Microlidian

## Done

- ~~Port menu etc to use ili9340 screen + touchscreen -- **done**, needs testing~~
- ~~Panic / all notes off action~~
- ~~Add 'all notes off' on 'extra button' press in midi_matrix_mapper menu~~
  - done -- added a 'PANIC' menu option and keyboard shortcut (p)
- ~~Move bass transposition options into the OutputWrapper?~~ done!
 - ~~remove debug output for this~~
 - ~~set default transposition~~
 - save transposition info to project?
 - ~~should actually probably be part of the DeviceBehaviour instead now?~~
 - ~~More efficient Akai APCMini display output (don't need to send as many messages as we do, can just update when it needs to)~~
- ~~Enable switching between multiple projects~~ done
- ~~Port to Teensy 4.1 and~~ (done - teensy version is now the main branch!)
- ~~Sequencer/looper that records and playback MIDI notes~~
  - ~~rudimentary MIDI looper~~ working, and saves to SD, 8 slots per project, with auto-advance
  - ~~Genericise MIDI looper functionality, so can eg record and loop drums ~~
- Bambleweeny controls
  - ~~Playback mode~~
  - ~~Fills on/off~~
  - ~~Density~~
  - ~~Low mutate/High mutate~~
  - ~~Pattern enables~~
  - ~~Select/lock seed~~
- ~~Sync from external clock input (MIDI)~~ done
- ~~MIDI DIN or TRS input + output~~ now using Deftaudio 8x8 interface board so get 8 MIDI DIN-ins and 8 MIDI DIN-outs!
- ~~TFT display~~ (working now using the Adafruit ST7789_t3 library and [mymenu](https://github.com/doctea/mymenu))
- ~~Encoder for controlling options and parameters~~
- ~~Configurable per-device MIDI clock divisions~~
  - ~~Done for Subclocker and saves with Project settings~~ ~~-- maybe it should save with pattern instead though - could then ? ~~
  - ~~add it for devices it might be useful for, eg CraftSynth~~
- ~~Save and recall MIDI device+channel routings~~
  - ~~Improve saving and recalling of MIDI device+channel routing, rather than having the names hardcoded~~
- ~~Allow input/loops to be redirected to multiple MIDI outputs.~~
  - ~~MIDI input/output matrix?~~
- ~~Treat serial USB devices as MIDI devices -- for devices that can behave like MIDI devices, but that don't expose the correct USB device enumerations~~
- ~~todo: improve this by not requiring the behaviour to implement MIDI -- could possibly let the behaviour itself deal with that, just have the usbserial connection stuff pass the usb device~~
  - DONE ~~eg for the [OpenTheremin v4 with USB MIDI modification](https://github.com/MrDham/OpenTheremin_V4_with_MIDI)~~
- ~~Make DeviceBehaviours work on serial inputs/outputs too, ...?~~
- ~~Bass drone mode~~
 - ~~toggle on/off, tracks the first/lowest note played in a phrase/bar and retrigger it on the start of every bar..~~
- ~~CraftSynth has Parameters, can assign modulation to them~~
- ~~eg remap APCMini faders to Bamble options~~
- stutter/machinegun mode? (eg play 32nd stabs instead of drone)
- ~~Visual control over the features of the [drum2musocv Bamblweeny](https://github.com/doctea/drum2musocv)?~~
  - ~~Control over the envelopes AHDSR + modulation~~
- ~~The 'delay' menu item for ClockedBehaviours gets set to an incorrect value when opened?~~

---


## Explanation/demo (very much out of date!)

![my unit](https://github.com/doctea/usb_midi_clocker/blob/main/media/my%20unit.jpg "My unit")

(Note that the below mostly applies to the Arduino Uno version, Teensy version has a lot more going on, info here is getting a bit long in the tooth now!)

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
