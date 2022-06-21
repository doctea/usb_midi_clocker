// note: as of 2022-02-17, requires https://github.com/felis/USB_Host_Shield_2.0/pull/438 to be applied to the USB_Host_Shield_2.0 library if using Arturia Beatstep, otherwise it won't receive MIDI data or clock!
// proof of concept for syncing multiple USB Midi devices wit

#include "debug.h"
#include "storage.h"

//#define USE_UCLOCK  // experimental: crashes a lot when receiving CC messages from APCMini

#define ENABLE_APCMINI
#define ENABLE_BEATSTEP
#define ENABLE_BAMBLE

#define ENABLE_APCMINI_DISPLAY
#define ENABLE_BPM
#define ENABLE_SEQUENCER
#define ENABLE_CLOCKS

//#define DEBUG_TICKS
//#define DEBUG_SEQUENCER

void on_restart();
void do_tick(uint32_t ticks);

#ifdef USE_UCLOCK
  #include <uClock.h>
#else
  #define ATOMIC(X) X
#endif

int duration = 2;

#include "usb.h"

#include "bpm.h"
#include "clock.h"

#ifdef ENABLE_SEQUENCER
  #include "sequencer.h"
#endif
#include "cv_outs.h"

#include "midi_beatstep.h"
#include "midi_apcmini.h"
#include "midi_bamble.h"

#include "multi_usb_handlers.h"

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  pinMode(PIN_CLOCK_1, OUTPUT);
  pinMode(PIN_CLOCK_2, OUTPUT);

  pinMode(PIN_CLOCK_3, OUTPUT);
  pinMode(PIN_CLOCK_4, OUTPUT);

  setup_multi_usb();
  delay( 1000 );
  
  Serial.println(F("Arduino ready."));

  #ifdef ENABLE_SEQUENCER
    init_sequence();
  #endif

  #ifdef USE_UCLOCK
    Serial.println(F("Initialising uClock.."));
    setup_uclock();
  #else
    setup_cheapclock();
  #endif

  Serial.println(F("Arduino ready."));
}

//long loop_counter = 0;

// -----------------------------------------------------------------------------`
//
// -----------------------------------------------------------------------------
void loop()
{
  //if (loop_counter%100==0) Serial.println(F("100th loop()"));
  ATOMIC(
    Usb.Task();
  )

  #ifdef ENABLE_BEATSTEP
      beatstep_loop();
  #endif

  #ifdef ENABLE_APCMINI
      apcmini_loop();
  #endif

  #ifdef ENABLE_BAMBLE
      bamble_loop();
  #endif

  #ifndef USE_UCLOCK
      if ( playing && millis()-t1 > ms_per_tick ) {
        do_tick(ticks);
        ticks++;
        t1 = millis();
      }
  #endif

  //Serial.println(F("."));
  /*if (!playing && single_step) {
    do_tick(ticks);
  }*/
  /*if (loop_counter%1000==0) Serial.println(F("main loop() - 1000 loops passed"));
  loop_counter++;*/
}

// called inside interrupt
void do_tick(volatile uint32_t in_ticks) {  
  /*#ifdef DEBUG_TICKS
      unsigned int delta = millis()-t1;

      Serial.print(ticks);
      Serial.print(F(":\tTicked with delta\t"));
      Serial.print(delta);
      Serial.print(F("!\t(ms_per_tick is "));
      Serial.print(ms_per_tick);
      Serial.print(F(") sending clock for [ "));
  #endif*/

  ticks = in_ticks;
  
  if (restart_on_next_bar && is_bpm_on_bar(in_ticks)) {
    //in_ticks = ticks = 0;
    on_restart();
    //ATOMIC(
      //midi_apcmini->sendNoteOn(7, APCMINI_OFF, 1);
    //)
    restart_on_next_bar = false;
  }

  update_cv_outs(in_ticks);

  #ifdef ENABLE_BEATSTEP
      beatstep_on_tick(in_ticks);
  #endif

  #ifdef ENABLE_BAMBLE
      bamble_on_tick(in_ticks);
  #endif

  #ifdef ENABLE_APCMINI
      apcmini_on_tick(in_ticks);
  #endif

  #ifdef DEBUG_TICKS
      Serial.println(F(" ]"));
  #endif 

  //ticks++;
  //t1 = millis();
  //single_step = false;
}
