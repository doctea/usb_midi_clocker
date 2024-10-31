#pragma once

///// DEBUG options
/////////////////// now moved to BootConfig.h  #define WAIT_FOR_SERIAL         // wait for serial terminal before starting setup -- for debugging startup
//#define DEBUG_LOOP_LOADER     // debug LOOPER file loading
//#define DEBUG_LOOPER          // debug LOOPER playback
//#define USBHOST_PRINT_DEBUG   // not sure if this will actually work here? may need to be put into the build options
//#define GDB_DEBUG             // to enable TeensyDebug (don't need to set this if building using the 'debug' build profile)

// NOTE that some of these options, especially ones that affect library functionality, need to be set in build_flags in platformio.ini!

//// CV input options
#ifdef PCB_STUDIO
    // these are now defined in build flags platformio.ini
    //#define ENABLE_CV_INPUT     0x48                // specify the i2c address of the input board
    //#define ENABLE_CV_INPUT_2   0x49
#elif defined(PCB_GO)
    // these are now defined in build flags platformio.ini
#else
    #define ENABLE_CV_INPUT 0x49
#endif
#define TIME_BETWEEN_CV_INPUT_UPDATES 1 //25    
#ifndef FAST_VOLTAGE_READS
    #define FAST_VOLTAGE_READS                  // disabling averaging of voltage reading - is now configured in platformio.ini build flags
#endif

#ifdef ENABLE_SD
    #ifndef ENABLE_CALIBRATION_STORAGE
        #define ENABLE_CALIBRATION_STORAGE          // enable save/recall of calibration data to SD card file
    #endif
    #ifndef LOAD_CALIBRATION_ON_BOOT
        #define LOAD_CALIBRATION_ON_BOOT            // whether to attempt to load calibration from SD card on boot
    #endif
#endif

// choose which MIDIDevice object from the USBHost_t36 library to use; seems to work completely fine using just the plain MIDIDevice, and saves a ton of flash memory too
//#define use_MIDIDevice_BigBuffer    MIDIDevice_BiggerBuffer
//#define use_MIDIDevice_BigBuffer    MIDIDevice_BigBuffer
#define use_MIDIDevice_BigBuffer      MIDIDevice

#define DEFAULT_CLOCK_MODE  CLOCK_INTERNAL      

#ifdef ENABLE_SCREEN 
    #ifdef TFT_ST7789_T3
        //#define TFT_ST7789_T3_BIG
        #define DisplayTranslator_Configured DisplayTranslator_STeensy_Big
        #define MENU_MS_BETWEEN_REDRAW  75
    #elif defined (TFT_BODMER)
        #define DisplayTranslator_Configured DisplayTranslator_Bodmer
        #define MENU_MS_BETWEEN_REDRAW  75
    #elif defined(TFT_ILI9341_T3N)
        #define DisplayTranslator_Configured DisplayTranslator_ILI9341_T3N
        #define MENU_MS_BETWEEN_REDRAW  20
    #endif
#endif

#ifndef ENABLE_SCREEN
    //#define tft_print(X) Serial.println(X)
    //void tft_print(const char *text);
#endif

#ifdef ENABLE_SCREEN
    #define ENCODER_STEP_DIVISOR    4
    #ifdef PROTOTYPE
        #define ENCODER_KNOB_L  2   // pin to receive left-encoder pulses
        #define ENCODER_KNOB_R  3   // pin to receive right-encoder pulses

        #define PIN_BUTTON_A    4   // pin to receive encoder button
        #define PIN_BUTTON_B    5   // pin to receive back button
        #define PIN_BUTTON_C    26  // pin to receive right-hand / save button
    #elif defined(PCB)
        #define ENCODER_KNOB_L  3   // pin to receive left-encoder pulses
        #define ENCODER_KNOB_R  2   // pin to receive right-encoder pulses

        #define PIN_BUTTON_A    4   // pin to receive encoder button
        #define PIN_BUTTON_B    5   // pin to receive back button
        #define PIN_BUTTON_C    6   // pin to receive select button
    #endif
#endif

#define ENABLE_USB          // USB host behaviours
#define ENABLE_CV_OUTPUT    // clock and sequencer outputs

#ifdef ENABLE_USB
    // enable a generic USB typing keyboard as a control method (see include/input_keyboard.h)
    //#define ENABLE_TYPING_KEYBOARD

    // enable a USB AliExpress keyboard eg https://www.aliexpress.com/item/1005006353228764.html
    // todo: automatic detection of appropriate device
    #define ENABLE_CONTROLLER_KEYBOARD

    #if defined(PCB_STUDIO)
        // enable USB devices that present as serial interfaces but support MIDI
        #define ENABLE_USBSERIAL
        #ifdef ENABLE_USBSERIAL
            #define ENABLE_OPENTHEREMIN
        #endif
    #endif
#endif

// enable MIDI looping for MPK49
#ifdef PCB_STUDIO
    #define ENABLE_LOOPER
    #define ENABLE_LOOPER_PIANOROLL // enable piano roll looper display
#endif

// enable MIDI looping for drumkit - experimental
//#define ENABLE_DRUM_LOOPER

// enable Neutron behaviour
#ifdef PCB_STUDIO
    //#define ENABLE_NEUTRON    // now done in ConfigMidi.h
    #define DEFAULT_NEUTRON_OCTAVE 3    // set to -1 for 'disabled'
#endif

//#define ENABLE_DPT_LOOPER
#ifdef ENABLE_DPT_LOOPER
    #ifdef PROTOTYPE
        #undef ENABLE_DPT_LOOPER
        #define ENABLE_DPT_LOOPER   MIDI8   // enable experimental behaviour (is just a very simple behaviour that sends start/continues on bar/phrases - used by my DPT Looper Daisy sketch, but probably not of much use for anything else!)
    #elif defined(PCB)
        #undef ENABLE_DPT_LOOPER
        #define ENABLE_DPT_LOOPER   MIDI8   // enable experimental behaviour (is just a very simple behaviour that sends start/continues on bar/phrases - used by my DPT Looper Daisy sketch, but probably not of much use for anything else!)
    #endif
#endif

#ifdef PCB_STUDIO
    //#define ENABLE_MAMMB33      MIDI2
    //#define ENABLE_MIDIMUSO     MIDI4   // enable quick haxx to set mode of a connected midimuso cv-12 https://midimuso.co.uk/tools/
    //#define ENABLE_MIDIMUSO_4PV   MIDI4
    #define ENABLE_MIDIMUSO_4MV MIDI4
#endif

// serial MIDI devices
// these are now defined in ConfigMidi.h instead

#ifdef ENABLE_USB
    #define ENABLE_APCMINI
    #define ENABLE_APCMINI_DISPLAY

    #define ENABLE_BEATSTEP
    //#define ENABLE_BEATSTEP_2       // experimental support for having two beatsteps attached
    #define ENABLE_BEATSTEP_SYSEX   // extra beatstep functionality
    ////#define ENABLE_BAMBLE
    ////#define ENABLE_BAMBLE_INPUT   // for collecting input from bambleweeny
    ////#define ENABLE_BAMBLE_OUTPUT  // for sending on the bamble ch1-4
    ////#define ENABLE_MPK49
    ////#define ENABLE_SUBCLOCKER
    ////#define ENABLE_SUBCLOCKER_DEDICATED
    #define ENABLE_KEYSTEP

    #ifdef PCB_STUDIO
        //#define ENABLE_KEYSTEP
        //#define ENABLE_CRAFTSYNTH_USB
        //#define ENABLE_SKULPTSYNTH_USB
        #define ENABLE_CHOCOLATEFEET_USB
        #define ENABLE_MIDILIGHTS
        //#define ENABLE_MIDILIGHTS_DEDICATED
        //#define ENABLE_BEHRINGER_EDGE_USB
        //#define ENABLE_BEHRINGER_EDGE_USB_DEDICATED
    #endif

    #ifdef PCB_GO
        #define ENABLE_KEYSTEP
        //#define ENABLE_MIDILIGHTS
    #endif
    
    #ifndef ENABLE_EUCLIDIAN
        #define ENABLE_MICROLIDIAN
    #endif
    #define ENABLE_MIDILIGHTS
#endif

#ifdef PCB_STUDIO
    #define ENABLE_BEHRINGER_EDGE_SERIAL    MIDI5
    //#define ENABLE_BEHRINGER_EDGE_SERIAL_DEDICATED    MIDI5
#endif

#if defined(ENABLE_CRAFTSYNTH) && defined(ENABLE_CRAFTSYNTH_USB)
    #error You should define ENABLE_CRAFTSYNTH_USB or ENABLE_CRAFTSYNTH, but not both
#endif
//#define ENABLE_CRAFTSYNTH_CLOCKTOGGLE   // enable menu item to enable/disable clock output to CraftSynth - only really useful for debug i think?

#define ENABLE_BPM
#define ENABLE_SEQUENCER
#define ENABLE_CLOCKS

#define SEPARATE_SEQUENCER_AND_CLOCKS   // define this if we have 8 distinct cv outputs available
                                        // first set of four become clock, second set of four for sequencer
                                        // if not defined then the four sequencer outputs are overlaid over the clock outputs
#ifdef ENABLE_CLOCKS
    #ifdef PROTOTYPE
        // prototype uses GPIO pins for the gate outs, so define which pins to use 
        #define NUM_CLOCKS 4              // 4 clocks (+ 1 reset if PIN_CLOCK_RESET is also set)
        //#define NUM_CLOCKS 7            // 7 clocks + 1 reset

        #define PIN_CLOCK_START  30
        #define PIN_CLOCK_1   30
        #define PIN_CLOCK_2   31
        #define PIN_CLOCK_3   32
        #define PIN_CLOCK_4   33
        #if NUM_CLOCKS > 4
            #define PIN_CLOCK_5   36  // carefully avoiding TX8+RX8, as those are needed for the 8th MIDI in/outs
        #endif
        #if NUM_CLOCKS > 5
            #define PIN_CLOCK_6   37
        #endif
        #if NUM_CLOCKS > 6
            #define PIN_CLOCK_7   38
        #endif
        #if NUM_CLOCKS > 7
            #define PIN_CLOCK_8   39
        #endif

        //#define PIN_CLOCK_RESET 40
    #elif defined(PCB)
        // PCB + expander uses SPI MCP23s17 for output, don't need to define pins because that's all handled by the MCP23s17
        #define NUM_CLOCKS 8                    // number of custom clocks (not including resets)
        /*#define PIN_CLOCK_RESET_PHRASE      4   // 5th output used as phrase reset
        #define PIN_CLOCK_RESET_HALF_PHRASE 5   // 6th output used as 2-bar reset
        #define PIN_CLOCK_RESET_BAR         6   // 7th output used as 2-bar reset
        #define PIN_CLOCK_RESET_BEAT        7   // 8th output used as locked every beat output*/
    #endif
#endif

#ifdef ENABLE_SEQUENCER
    #define NUM_SEQUENCES 8
    #define NUM_STEPS 8
    #ifdef PROTOTYPE
        #ifdef SEPARATE_SEQUENCER_AND_CLOCKS   
            // new 'reversed' pin ordering
            #define PIN_SEQUENCE_1 39
            #define PIN_SEQUENCE_2 38
            #define PIN_SEQUENCE_3 37
            #define PIN_SEQUENCE_4 36
        #endif
    #elif defined(PCB)
        // shouldn't need to define pins as should all be handled by teh MCP23s17
        //#define ENABLE_MORE_CLOCKS // output a couple of extra clocks (16th and 8th notes) on the extra sequencer pins
    #endif
#endif

// turn off IRQs while checking changed USB devices 
// seems to prevent crashes on reconnection, but causes pipes to freeze up, it seems
#define IRQ_PROTECT_USB_CHANGES
