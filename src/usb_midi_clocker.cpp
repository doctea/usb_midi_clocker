// note: as of 2022-02-17, requires https://github.com/felis/USB_Host_Shield_2.0/pull/438 to be applied to the USB_Host_Shield_2.0 library if using Arturia Beatstep, otherwise it won't receive MIDI data or clock!
// proof of concept for syncing multiple USB Midi devices wit

#include "Config.h"

#include "debug.h"
#include "storage.h"

//#define USE_UCLOCK  // experimental: crashes a lot when receiving CC messages from APCMini

void on_restart();
void do_tick(uint32_t ticks);

#ifdef USE_UCLOCK
  #include <uClock.h>
#else
  #define ATOMIC(X) X
#endif

int duration = 2;

#ifdef ENABLE_USB_HOST
  #include "usb.h"
#endif

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

#include "midi_input.h"

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  if (!TRUE_MIDI_SERIAL) {
    Serial.begin(115200);
    #ifdef WAIT_FOR_SERIAL
      while (!Serial) {
        digitalWrite(LED_BUILTIN, 1);
        delay(500);
        digitalWrite(LED_BUILTIN, 0);
      };
    #endif
  }

  if (!TRUE_MIDI_SERIAL) {
    Serial.println(F("BOOTED!")); Serial.flush();
  }

  #if defined(ENABLE_CLOCKS) || defined(ENABLE_SEQUENCER)
    pinMode(PIN_CLOCK_1, OUTPUT);
    pinMode(PIN_CLOCK_2, OUTPUT);
    pinMode(PIN_CLOCK_3, OUTPUT);
    pinMode(PIN_CLOCK_4, OUTPUT);
  #endif

  setup_serial_midi_input();

  #ifdef ENABLE_USB_HOST
    setup_multi_usb();
    delay( 1000 );
    if (!TRUE_MIDI_SERIAL) {
      Serial.println(F("Arduino finished USB setup."));
    }
  #endif
  delay( 1000 );
  
  #ifdef ENABLE_SEQUENCER
    init_sequence();
  #endif

  #ifdef USE_UCLOCK
    Serial.println(F("Initialising uClock.."));
    setup_uclock();
  #else
    setup_cheapclock();
  #endif

  if (!TRUE_MIDI_SERIAL) 
    Serial.println(F("Arduino finished setup()."));
}

//long loop_counter = 0;

// -----------------------------------------------------------------------------`
//
// -----------------------------------------------------------------------------
void loop()
{
  //Serial.println(F("started loop()")); Serial.flush();
  //static long loop_count = 0;
  if ((millis()%2000)<1000) 
    digitalWrite(LED_BUILTIN, HIGH);
  else
    digitalWrite(LED_BUILTIN, LOW);
  //return;
  //if (loop_counter%100==0) Serial.println(F("100th loop()"));
  #ifdef ENABLE_USB_HOST
    //Serial.println("about to usb.task()"); Serial.flush();
    ATOMIC(
      Usb.Task();
    )
    //Serial.println("just did usb.task()"); Serial.flush();
  #endif

  //Serial.println(F("about to loop_serial_midi()")); Serial.flush();
  loop_serial_midi();
  //Serial.println(F("just did loop_serial_midi()")); Serial.flush();

  #ifdef ENABLE_USB_HOST
    #ifdef ENABLE_BEATSTEP
      //Serial.println(F("about to beatstep_loop()")); Serial.flush();
      beatstep_loop();
      //Serial.println(F("just did beatstep_loop()")); Serial.flush();
    #endif

    #ifdef ENABLE_APCMINI
        apcmini_loop();
    #endif

    #ifdef ENABLE_BAMBLE
        bamble_loop();
    #endif
  #endif

  #ifndef USE_UCLOCK
    //Serial.println(F("Starting clock ticks..."));
    bool should_tick = false;
    if (clock_mode==CLOCK_EXTERNAL_USB_HOST && check_and_unset_usb_midi_clock_ticked())
      should_tick = playing && true;
    else if (clock_mode==CLOCK_INTERNAL) 
      should_tick = playing && millis()-t1 > ms_per_tick;

    if ( should_tick ) {
      /*Serial.print(F("ticked "));
      Serial.print(ticks);
      Serial.println();
      Serial.flush();*/
      #if defined(ENABLE_USB_HOST) && defined(ENABLE_BEATSTEP)
        #ifdef TEST_START_EVERY_FOUR_BEATS
          if (ticks==0 || ticks % PPQN == 0 ) {
            if (!TRUE_MIDI_SERIAL) {
              Serial.println(F("about to beatstep_on_restart..")); Serial.flush();
            }
            beatstep_on_restart();
          }
        #endif
        if(!TRUE_MIDI_SERIAL && is_bpm_on_bar(ticks)) {
          Serial.println(F("Tick on bar!"));
        }
      #endif
      //Serial.println(F("about to do_ticks()"));
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

  #ifdef ENABLE_USB_HOST
    #ifdef ENABLE_BEATSTEP
        beatstep_on_tick(in_ticks);
    #endif

    #ifdef ENABLE_BAMBLE
        bamble_on_tick(in_ticks);
    #endif

    #ifdef ENABLE_APCMINI
        apcmini_on_tick(in_ticks);
    #endif
  #endif

  #ifdef DEBUG_TICKS
      Serial.println(F(" ]"));
  #endif 

  //ticks++;
  //t1 = millis();
  //single_step = false;
}
