// set your config options here!

// clock source setting
//#define DEFAULT_CLOCK_MODE  CLOCK_INTERNAL
#define DEFAULT_CLOCK_MODE  CLOCK_EXTERNAL_USB_HOST

// midi settings
//#define TRUE_MIDI_SERIAL        // use true MIDI baud rate, for when eg uno controller has been flashed with usbmidiklik
//#define TRUE_MIDI_SERIAL true   // use true MIDI baud rate, for when eg uno controller has been flashed with usbmidiklik
#define TRUE_MIDI_SERIAL false  // for using Hairless

#define BAUDRATE_HAIRLESS 115200  // for use with Hairless MIDI Serial Bridge

// midi / time signature settings
#define PPQN  24
#define BEATS_PER_BAR   4
#define BARS_PER_PHRASE 4

// sequencer and clock options
//#define ENABLE_BPM              // enable BPM change via APCMINI controls
//#define ENABLE_SEQUENCER        // enable sequencer overlay onto clocks
#define ENABLE_CLOCKS           // enable clock outputs

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
//#define ENABLE_USB_HOST   // completely enable/disable
#ifdef ENABLE_USB_HOST
    #define ENABLE_APCMINI
    #define ENABLE_BEATSTEP
    #define ENABLE_BAMBLE
    #define ENABLE_APCMINI_DISPLAY    // whether to enable APCMINI display
#endif

//#define DEBUG_TICKS
//#define DEBUG_SEQUENCER
