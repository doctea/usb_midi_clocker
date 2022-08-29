//#define USE_UCLOCK  // experimental: crashes a lot

///// DEBUG options
//#define WAIT_FOR_SERIAL       // wait for serial terminal before starting setup -- for debugging startup
//#define DEBUG_LOOP_LOADER     // debug LOOPER file loading
//#define DEBUG_LOOPER          // debug LOOPER playback

//#define GDB_DEBUG             // to enable TeensyDebug 

#define use_MIDIDevice_BigBuffer    MIDIDevice_BiggerBuffer
//#define use_MIDIDevice_BigBuffer    MIDIDevice_BigBuffer

#define DEFAULT_CLOCK_MODE CLOCK_INTERNAL
#define DEFAULT_SUBCLOCKER_DIVISOR          4
#define DEFAULT_SUBCLOCKER_DELAY_TICKS      0   // don't send clock to the subclocker device until this many ticks have elapsed

#define ENABLE_SCREEN       // tft
#ifdef ENABLE_SCREEN
    #define DisplayTranslator_Configured DisplayTranslator_STeensy_Big
    #define TFT_ST7789_T3
    #define TFT_ST7789_T3_BIG
    #define MENU_MS_BETWEEN_REDRAW  75
#endif

#ifndef ENABLE_SCREEN
    #define tft_print(X) Serial.println(X)
#endif

#ifdef ENABLE_SCREEN
    #define ENCODER_STEP_DIVISOR    4
    #define PIN_BUTTON_A    4   // pin to receive encoder button
    #define PIN_BUTTON_B    5   // pin to receive back button
    #define PIN_BUTTON_C    26  // pin to receive right-hand / save button
    #define ENCODER_KNOB_L  2   // pin to receive left-encoder pulses
    #define ENCODER_KNOB_R  3   // pin to receive right-encoder pulses
#endif

#define ENABLE_USB
#define ENABLE_CV

// enable MIDI looping for MPK49
#define ENABLE_LOOPER
#define ENABLE_LOOPER_PIANOROLL

// enable MIDI looping for drumkit
//#define ENABLE_DRUM_LOOPER

// enable transposing beatstep to a particular octave
#define ENABLE_BASS_TRANSPOSE MIDI3
#define DEFAULT_NEUTRON_OCTAVE 2    // set to 1 for 'disabled'

//#define ENABLE_LESTRUM    // these are defined in ConfigMidi.h instead
//#define ENABLE_DRUMKIT    // these are defined in ConfigMidi.h instead

//#define ENABLE_CRAFTSYNTH   // serial MIDI version

#ifdef ENABLE_USB
    #define ENABLE_APCMINI
    #define ENABLE_BEATSTEP
    //#define ENABLE_BEATSTEP_SYSEX   // extra beatstep functionality
    #define ENABLE_BAMBLE
    #define ENABLE_MPK49
    #define ENABLE_KEYSTEP
    #define ENABLE_SUBCLOCKER
    #define ENABLE_CRAFTSYNTH_USB
    #define ENABLE_CHOCOLATEFEET_USB

    #define ENABLE_APCMINI_DISPLAY
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
#endif

#ifdef ENABLE_SEQUENCER
    #define NUM_SEQUENCES 4
    #define NUM_STEPS 8
    #ifdef SEPARATE_SEQUENCER_AND_CLOCKS   
        // new 'reversed' pin ordering
        #define PIN_SEQUENCE_1 39
        #define PIN_SEQUENCE_2 38
        #define PIN_SEQUENCE_3 37
        #define PIN_SEQUENCE_4 36
        
        /*
        // original ordering
        #define PIN_SEQUENCE_1 36
        #define PIN_SEQUENCE_2 37
        #define PIN_SEQUENCE_3 38
        #define PIN_SEQUENCE_4 39
        */
    #endif
#endif
