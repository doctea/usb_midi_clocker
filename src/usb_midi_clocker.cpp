// note: as of 2022-02-17, requires https://github.com/felis/USB_Host_Shield_2.0/pull/438 to be applied to the USB_Host_Shield_2.0 library if using Arturia Beatstep, otherwise it won't receive MIDI data or clock!
// proof of concept for syncing multiple USB Midi devices wit

#if defined(__arm__) && defined(CORE_TEENSY)
  //#define byte uint8_t
  #define F(X) X
#endif

#include <Arduino.h>

#include "Config.h"

#include "debug.h"
#include "storage.h"

#ifdef ENABLE_SCREEN
  //#include "tft.h"
  #include "menu.h"
#endif
#include "project.h"

//#define DEBUG_TICKS
//#define DEBUG_SEQUENCER

//void on_restart();
void do_tick(uint32_t ticks);

#ifdef USE_UCLOCK
  //#include <uClock.h>
#else
  //#define ATOMIC(X) noInterrupts(); X; interrupts();
  #define ATOMIC(X) X
#endif

//#include "usb.h"
#include "midi_pc_usb.h"

#include "Config.h"

#include "MidiMappings.h"
#include "midi_outs.h"

#include "bpm.h"
#include "clock.h"

#ifdef ENABLE_SEQUENCER
  #include "sequencer.h"
#endif
#include "cv_outs.h"

#include "multi_usb_handlers.h"

#include "behaviour_manager.h"

void setup() {
  Serial.begin(115200);
  #ifdef WAIT_FOR_SERIAL
    while (!Serial);
  #endif

  //tft_print((char*)"..USB device handler..");
  // do this first, because need to have the behaviour classes instantiated before menu, as menu wants to call back to the behaviour_subclocker behaviours..
  // TODO: have the behaviours add their menu items
  Serial.println("..USB device handler..");
  setup_behaviour_manager();

  #ifdef ENABLE_SCREEN
    //setup_tft();
    setup_menu();
  #endif

  #ifdef ENABLE_CV
    tft_print((char*)"Setting up CV..\n");
    setup_cv();
  #endif

  delay( 100 );

  tft_print((char*)"..serial MIDI..\n");
  setup_midi_serial_devices();
  Serial.println(F("Serial ready."));   

  tft_print((char*)"..storage..\n");
  storage::setup_storage();

  tft_print((char*)"..setup project..\n");
  project.setup_project();

  #ifdef ENABLE_SEQUENCER
    tft_print((char*)"..Sequencer..\n");
    init_sequence();
  #endif

  #ifdef USE_UCLOCK
    Serial.println(F("Initialising uClock.."));
    setup_uclock();
  #else
    tft_print((char*)"..clock..\n");
    setup_cheapclock();
  #endif

  tft_print((char*)"..PC USB..\n");
  setup_pc_usb();
  
  tft_print((char*)"..USB..");
  setup_multi_usb();
  Serial.println(F("USB ready."));

  Serial.println(F("Arduino ready."));
  #ifdef ENABLE_SCREEN
    tft_print((char*)"Ready!");
    tft_clear();

    menu->start();
    //tft_start();
  #endif
}

//long loop_counter = 0;

// -----------------------------------------------------------------------------`
//
// -----------------------------------------------------------------------------
void loop() {
  //Serial.println("start of loop!"); Serial.flush();
  bool debug = false;

  #ifdef DEBUG_LED
    static int loop_counter;
    static bool lit = false;
    loop_counter++;
    if (loop_counter%1000000==0) {
      digitalWrite(LED_BUILTIN, lit);
      lit = !lit;
      //Serial.println(F("100000th loop()"));
    }
  #endif
  if (debug) { Serial.println("about to Usb.Task()"); Serial.flush(); }
  Usb.Task();
  if (debug) { Serial.println("just did Usb.Task()"); Serial.flush(); }
  //while (usbMIDI.read());

  //static unsigned long last_ticked_at_micros = 0;

  bool ticked = false;
  if (clock_mode==CLOCK_EXTERNAL_USB_HOST && /*playing && */check_and_unset_pc_usb_midi_clock_ticked())
    ticked = true;
  else if (clock_mode==CLOCK_INTERNAL && playing && micros()-last_ticked_at_micros >= micros_per_tick)
    ticked = true;
  else if (clock_mode==CLOCK_NONE)
    ticked = false;
  
  if ( playing && ticked ) {
    if (micros()-last_ticked_at_micros > micros_per_tick+1000) { //((unsigned long)micros_per_tick)+1) {
      //Serial.printf("WARNING: tick took %ius, more than micros_per_tick of %ius!\n", micros()-last_ticked_at_micros, (unsigned long)micros_per_tick);
      #ifdef DEBUG
        Serial.printf("WARNING: tick %u took %uus, more than 1ms longer than required micros_per_tick, which is %fus\n", ticks, micros()-last_ticked_at_micros, micros_per_tick);
      #endif
    }
    if (debug) { Serial.println("about to do_tick"); Serial.flush(); }
    do_tick(ticks);
    if (debug) { Serial.println("just did do_tick"); Serial.flush(); }
    //last_ticked_at_micros = micros();
    last_ticked_at_micros = micros();
    ticks++;

    /*  // but thing maths works out better if this is called here?
    if (restart_on_next_bar && is_bpm_on_bar(ticks)) {
      //in_ticks = ticks = 0;
      on_restart();
      //ATOMIC(
        //midi_apcmini->sendNoteOn(7, APCMINI_OFF, 1);
      //)
      restart_on_next_bar = false;
    }
    */

  } else {
    #ifdef ENABLE_SCREEN
      //tft_update(ticks);
      ///Serial.println("going into menu->display and then pausing 1000ms: "); Serial.flush();
      static unsigned long last_drawn;
      menu->update_inputs();
      if (millis() - last_drawn > MENU_MS_BETWEEN_REDRAW) {
        menu->update_ticks(ticks);
        //long before_display = millis();
        Serial.println("about to menu->display"); Serial.flush();
        menu->display(); //update(ticks);
        Serial.println("just did menu->display"); Serial.flush();
        //Serial.printf("display() took %ums..", millis()-before_display);
        last_drawn = millis();
      }
      //delay(1000); Serial.println("exiting sleep after menu->display"); Serial.flush();
    #endif
  }

  read_midi_serial_devices();
  loop_midi_serial_devices();

  #ifdef ENABLE_USB
    update_usb_device_connections();
    read_midi_usb_devices();
    behaviour_manager->do_loops();
    read_usb_from_computer();   // this is what sets should tick flag so should do this as early as possible before main loop start (or as late as possible in previous loop)
  #endif

  //Serial.println("end of loop!"); Serial.flush();

  //Serial.println(F("."));
  /*if (!playing && single_step) {
    do_tick(ticks);
  }*/
  /*if (loop_counter%1000==0) Serial.println(F("main loop() - 1000 loops passed"));
  loop_counter++;*/

  //storage::load_state_update();  // read next bit of file

        //Serial.println("sleeping before end of loop 1000ms: "); Serial.flush();
        //delay(1000);
        //Serial.println("exiting sleep before end of loop"); Serial.flush();

}

// called inside interrupt
void do_tick(uint32_t in_ticks) {
  /*#ifdef DEBUG_TICKS
      unsigned int delta = millis()-last_ticked_at_micros;

      Serial.print(ticks);
      Serial.print(F(":\tTicked with delta\t"));
      Serial.print(delta);
      Serial.print(F("!\t(ms_per_tick is "));
      Serial.print(ms_per_tick);
      Serial.print(F(") sending clock for [ "));
  #endif*/
  //Serial.println("ticked");

  #ifndef USE_UCLOCK
    ticks = in_ticks;
  #endif
  
  // original restart check+code went here? -- seems like better timing with bamble etc when call this here
  if (restart_on_next_bar && is_bpm_on_bar(ticks)) {
    //in_ticks = ticks = 0;
    global_on_restart();
    //ATOMIC(
      //midi_apcmini->sendNoteOn(7, APCMINI_OFF, 1);
    //)
    restart_on_next_bar = false;
  }

  if (is_bpm_on_phrase(ticks))
    project.on_phrase(BPM_CURRENT_PHRASE);

  send_midi_serial_clocks();

  #ifdef ENABLE_USB
    //Serial.println("in do_tick() about to behaviour_manager->send_clocks()"); Serial.flush();
    behaviour_manager->send_clocks();
    //Serial.println("in do_tick() just did behaviour_manager->send_clocks()"); Serial.flush();
  #endif

  #ifdef ENABLE_CV
    update_cv_outs(in_ticks);
  #endif

  #ifdef ENABLE_USB
    //Serial.println("in do_tick() about to behaviour_manager->do_ticks()"); Serial.flush();
    behaviour_manager->do_ticks(in_ticks);
    //Serial.println("in do_tick() just did behaviour_manager->do_ticks()"); Serial.flush();
  #endif

  #ifdef DEBUG_TICKS
    Serial.println(F(" ]"));
  #endif 

  //last_processed_tick = ticks;
  //ticks++;
  //last_ticked_at_micros = millis();
  //single_step = false;
}
