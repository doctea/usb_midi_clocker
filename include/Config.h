//#define USE_UCLOCK  // experimental: crashes a lot

///// DEBUG options
//#define WAIT_FOR_SERIAL       // wait for serial terminal before starting setup -- for debugging startup
//#define DEBUG_LOOP_LOADER     // debug LOOPER file loading
//#define DEBUG_LOOPER          // debug LOOPER playback

#define DEFAULT_CLOCK_MODE CLOCK_INTERNAL
#define DEFAULT_SUBCLOCKER_DIVISOR      4
#define DEFAULT_SUBCLOCKER_DELAY_TICKS    0   // don't send clock to the subclocker device until this many ticks have elapsed

#define ENABLE_SCREEN       // tft
#ifdef ENABLE_SCREEN
    #define TFT_ST7789_T3
    #define TFT_ST7789_T3_BIG
    #define MENU_MS_BETWEEN_REDRAW  75
#endif

#ifndef ENABLE_SCREEN
    #define tft_print(X) Serial.println(X)
#endif

#ifdef ENABLE_SCREEN
    #define ENCODER_STEP_DIVISOR    4
    #define PIN_BUTTON_A    4   // encoder button
    #define PIN_BUTTON_B    5   // back button
    #define PIN_BUTTON_C    26  // right-hand / save button
    #define ENCODER_KNOB_L  2
    #define ENCODER_KNOB_R  3
#endif

#define ENABLE_USB
#define ENABLE_CV

// enable MIDI looping for MPK49
#define ENABLE_LOOPER

#ifdef ENABLE_USB
    #define ENABLE_APCMINI
    #define ENABLE_BEATSTEP
    #define ENABLE_BAMBLE
    #define ENABLE_MPK49
    #define ENABLE_KEYSTEP
    #define ENABLE_SUBCLOCKER

    #define ENABLE_APCMINI_DISPLAY
#endif

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
        #define PIN_SEQUENCE_1 36
        #define PIN_SEQUENCE_2 37
        #define PIN_SEQUENCE_3 38
        #define PIN_SEQUENCE_4 39
    #endif
#endif
