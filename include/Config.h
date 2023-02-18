// set your config options here!

// clock source setting
#define DEFAULT_CLOCK_MODE  CLOCK_INTERNAL
//#define DEFAULT_CLOCK_MODE  CLOCK_EXTERNAL_USB_HOST

//#define WAIT_FOR_SERIAL                 // define this if you want arduino to stop at the top of setup() and wait for a serial monitor connection before continuing
//#define TEST_START_EVERY_FOUR_BEATS     // define this if you want to send a STOP/START to the beatstep every 4 beats (useful while testing if its working..)

// midi settings
//#define TRUE_MIDI_SERIAL true   // use true MIDI baud rate, for when eg uno controller has been flashed with usbmidiklik, or (untested) when using a real MIDI connection on the Arduino RX pins?
#define TRUE_MIDI_SERIAL false    // for using Hairless-MIDI to talk to PC over USB; sends debug stuff over serial USB/TX pin 

#define BAUDRATE_HAIRLESS 115200  // for use with Hairless MIDI Serial Bridge

// midi / time signature settings
#define PPQN  24
#define BEATS_PER_BAR   4
#define BARS_PER_PHRASE 4

// sequencer and clock options
//#define ENABLE_BPM              // enable BPM change via APCMINI controls
//#define ENABLE_SEQUENCER        // enable cv sequencer overlay onto clocks
//#define ENABLE_CLOCKS           // enable cv lock outputs

#define NUM_CLOCKS    4
#define NUM_SEQUENCES 4
#define NUM_STEPS     8

// clock trigger settings
#define MAX_CLOCK_DELAY   7     // maximum number of beats a clock can be delayed for before wrapping around
#define DEFAULT_CLOCK_MULTIPLIERS 5, 4, 3, 2  // default clock multipliers - two bar, bar, half bar, beat
//#define DEFAULT_CLOCK_MULTIPLIERS 6, 5, 4, 3  // slower - every phrase, every two bar, every bar, every half bar
//#define DEFAULT_CLOCK_MULTIPLIERS 7, 6, 5, 4    // glacial - every 2nd phrase, every phrase, every two bar, every bar
#define DEFAULT_CLOCK_DELAYS  0, 0, 0, 0      // how many beats to delay each clock by by default

// if want to use (experimental, nonworking) UCLOCK library
#ifdef USE_UCLOCK
  #include <uClock.h>
#else
  #define ATOMIC(X) X
#endif

// if should use USB Host Library 2.0 shield for USB MIDI devices
#define ENABLE_USB_HOST   // completely enable/disable USB Host support
#ifdef ENABLE_USB_HOST
    //#define ENABLE_APCMINI            // use apcmini
    #define ENABLE_BEATSTEP             // use beatstep
    //#define ENABLE_BAMBLE             // use bamble device
    //#define ENABLE_APCMINI_DISPLAY    // whether to enable APCMINI display
#endif

//#define DEBUG_TICKS
//#define DEBUG_SEQUENCER
