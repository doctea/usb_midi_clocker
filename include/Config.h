//#define USE_UCLOCK  // experimental: crashes a lot

#define ENABLE_USB
#define ENABLE_CV

#define ENABLE_RECORDING

#ifdef ENABLE_USB
    #define ENABLE_APCMINI
    #define ENABLE_BEATSTEP
    #define ENABLE_BAMBLE
    #define ENABLE_MPK49
    #define ENABLE_KEYSTEP

    #define ENABLE_APCMINI_DISPLAY
#endif

#define ENABLE_BPM
#define ENABLE_SEQUENCER
#define ENABLE_CLOCKS

#define NUM_CLOCKS 4              // 4 clocks (+ 1 reset if PIN_CLOCK_RESET is also set)
//#define NUM_CLOCKS 7            // 7 clocks + 1 reset
#define NUM_SEQUENCES 4
#define NUM_STEPS 8
