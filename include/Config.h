
//#define DEFAULT_CLOCK_MODE  CLOCK_INTERNAL
#define DEFAULT_CLOCK_MODE  CLOCK_EXTERNAL_USB_HOST

//#define TRUE_MIDI_SERIAL
#define BAUDRATE_HAIRLESS 115200

#ifdef USE_UCLOCK
  #include <uClock.h>
#else
  #define ATOMIC(X) X
#endif

// enabled USB MIDI outputs
//#define ENABLE_USB_HOST
#ifdef ENABLE_USB_HOST
    #define ENABLE_APCMINI
    #define ENABLE_BEATSTEP
    #define ENABLE_BAMBLE
    // whether to enable APCMINI display
    #define ENABLE_APCMINI_DISPLAY
#endif

//#define ENABLE_BPM              // enable BPM change via APCMINI
//#define ENABLE_SEQUENCER        // enable sequencer overlay onto clocks
#define ENABLE_CLOCKS           // enable clock outputs

//#define DEBUG_TICKS
//#define DEBUG_SEQUENCER
