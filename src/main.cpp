// note: as of 2022-02-17, requires https://github.com/felis/USB_Host_Shield_2.0/pull/438 to be applied to the USB_Host_Shield_2.0 library if using Arturia Beatstep, otherwise it won't receive MIDI data or clock!
// proof of concept for syncing multiple USB Midi devices wit

#if defined(__arm__) && defined(CORE_TEENSY)
  //#define byte uint8_t
  #define F(X) X
#endif

#include <Arduino.h>

#include "BootConfig.h"
#include "Config.h"

#if defined(GDB_DEBUG) or defined(USB_MIDI16_DUAL_SERIAL)
  #include "TeensyDebug.h"
  #pragma GCC optimize ("O0")
#endif

#include "debug.h"
#include "storage.h"

#ifdef ENABLE_SCREEN
  //#include "tft.h"
  #include "menu.h"
  #include "mymenu/menu_debug.h"
#endif
#include "project.h"

#ifdef ENABLE_CV_INPUT
  #include "cv_input.h"
#endif

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
#include "midi/midi_pc_usb.h"

#include "midi/MidiMappings.h"
#include "midi/midi_outs.h"

#include "bpm.h"
#include "clock.h"

#ifdef ENABLE_SEQUENCER
  #include "sequencer.h"
#endif
#include "cv_outs.h"

#ifdef ENABLE_USB
  #include "multi_usb_handlers.h"
#endif
#ifdef ENABLE_USBSERIAL
  #include "multi_usbserial_handlers.h"
#endif

#include "behaviours/behaviour_manager.h"

#include "input_keyboard.h"

#include "profiler.h"

#ifdef ENABLE_PROFILER
  #define NUMBER_AVERAGES 1024
  uint32_t *main_loop_length_averages; //[NUMBER_AVERAGES];
  int count = 0;
#endif

#ifndef GDB_DEBUG
FLASHMEM 
#endif
void setup() {
  #if defined(GDB_DEBUG) or defined(USB_MIDI16_DUAL_SERIAL)
    debug.begin(SerialUSB1);
  #endif

  Serial.begin(115200);
  Serial.setTimeout(0);
  #ifdef WAIT_FOR_SERIAL
    //tft_print("\nWaiting for serial connection..");
    while (!Serial);
    //tft_print("Connected serial!\n");
    Serial.println(F("Connected serial!")); Serial_flush();
  #endif

  /*while (1) {
    Serial.printf(".");
  }*/

  Serial.printf(F("At start of setup(), free RAM is %u\n"), freeRam());

  //tft_print((char*)"..USB device handler..");
  // do this first, because need to have the behaviour classes instantiated before menu, as menu wants to call back to the behaviour_subclocker behaviours..
  // TODO: have the behaviours add their menu items
  Serial.println(F("..USB device handler.."));
  setup_behaviour_manager();
  Serial.printf(F("after setup_behaviour_manager(), free RAM is %u\n"), freeRam());

  //Serial.println("..MIDIOutputWrapper manager..");
  //setup_midi_output_wrapper_manager();
  Serial.println(F("..MIDI matrix manager.."));
  //setup_midi_output_wrapper_manager();
  setup_midi_mapper_matrix_manager();
  Serial.printf(F("after setup_midi_mapper_matrix_manager(), free RAM is %u\n"), freeRam());

  #ifdef ENABLE_SCREEN
    //setup_tft();
    setup_menu();
  #endif

  #ifdef ENABLE_TYPING_KEYBOARD
    tft_print((char*)"Setting up typing keyboard..\n");
    setup_typing_keyboard();
  #endif

  #ifdef ENABLE_CV_OUTPUT
    tft_print((char*)"Setting up CV..\n");
    setup_cv_output();
  #endif
  Serial.printf(F("after setup_cv_output(), free RAM is %u\n"), freeRam());

  delay( 100 );

  tft_print((char*)"..serial MIDI..\n");
  setup_midi_serial_devices();
  Serial.println(F("Serial ready."));   
  Serial.printf(F("after setup_midi_serial_devices(), free RAM is %u\n"), freeRam());

  tft_print((char*)"..storage..\n");
  storage::setup_storage();
  Serial.printf(F("after setup_storage(), free RAM is %u\n"), freeRam());


  tft_print((char*)"..setup project..\n");
  project->setup_project();
  Serial.printf(F("after setup_project(), free RAM is %u\n"), freeRam());

  #ifdef ENABLE_CV_INPUT
    setup_cv_input();
    Serial.printf(F("after setup_cv_input(), free RAM is %u\n"), freeRam());
    setup_parameters();
    Serial.printf(F("after setup_parameters(), free RAM is %u\n"), freeRam());
    #ifdef ENABLE_SCREEN
      menu->add_page("Parameter Inputs");
      setup_parameter_menu();
      Serial.printf(F("after setup_parameter_menu(), free RAM is %u\n"), freeRam());
    #endif
  #endif

  Serial.println(F("...starting behaviour_manager#make_menu_items..."));
  behaviour_manager->create_all_behaviour_menu_items(menu);
  Serial.println(F("...finished behaviour_manager#make_menu_items..."));

  behaviour_manager->setup_saveable_parameters();

  #ifdef ENABLE_SEQUENCER
    tft_print((char*)"..Sequencer..\n");
    init_sequence();
    Serial.printf(F("after init_sequence(), free RAM is %u\n"), freeRam());
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
  Serial.printf(F("after setup_pc_usb(), free RAM is %u\n"), freeRam());
  
  #ifdef ENABLE_USB
    tft_print((char*)"..USB..");
    setup_multi_usb();
    Serial.println(F("USB ready.")); Serial_flush();
    Serial.printf(F("after setup_multi_usb(), free RAM is %u\n"), freeRam());
  #endif

  #ifdef ENABLE_USBSERIAL
    tft_print((char*)"..USBSerial..");
    setup_multi_usbserial();
    Serial.println(F("USBSerial ready.")); Serial_flush();
    Serial.printf(F("after setup_multi_usbserial(), free RAM is %u\n"), freeRam());
  #endif

  Serial.println(F("Arduino ready.")); Serial_flush();
  #ifdef ENABLE_SCREEN
    tft_print((char*)"Ready!"); 
    tft_clear();

    Serial.println(F("About to init menu..")); Serial_flush();
    menu->start();
    Serial.printf(F("after menu->start(), free RAM is %u\n"), freeRam());
    //tft_start();

    setup_debug_menu();
    Serial.printf(F("after setup_debug_menu(), free RAM is %u\n"), freeRam());

    menu->select_page(0);
  #endif

  #ifdef ENABLE_PROFILER
    Serial.printf("Allocating array for profiler");
    main_loop_length_averages = malloc(sizeof(uint32_t)*NUMBER_AVERAGES);
  #endif

  Serial.println(F("Finished setup()!"));
  Serial.printf(F("at end of setup(), free RAM is %u\n"), freeRam());

  snprintf(menu->last_message, MENU_C_MAX, "...started up, %u bytes free...", freeRam());
}

//long loop_counter = 0;

bool debug = false;

// -----------------------------------------------------------------------------
// 
// -----------------------------------------------------------------------------
void loop() {
  if (Serial.getReadError() || Serial.getWriteError()) {
      Serial.end();
      Serial.clearReadError(); Serial.clearWriteError();
      Serial.begin(115200);
      Serial.setTimeout(0);
  }

  if (debug_stress_sequencer_load && ticks % 6 == 1)  {
    OnPress(':');
    OnPress('L');
    OnPress('J');
    OnPress('K');
    OnPress('L');
    OnPress('J');
    OnPress('K');
    OnPress(':');
  }

  //#ifdef ENABLE_PROFILER
    uint32_t start_loop_micros_stamp = micros();
  //#endif
  //bool debug = true;
  if (debug) { Serial.println(F("start of loop!")); Serial_flush(); }

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

  if (debug) { Serial.println(F("about to Usb.Task()")); Serial_flush(); }
  Usb.Task();
  if (debug) { Serial.println(F("just did Usb.Task()")); Serial_flush(); }
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
    if (debug) { Serial.println(F("about to do_tick")); Serial_flush(); }
    do_tick(ticks);
    if (debug) { Serial.println(F("just did do_tick")); Serial_flush(); }

    if (debug) { Serial.println(F("about to do menu->update_ticks(ticks)")); Serial_flush(); }
    menu->update_ticks(ticks);
    if (debug) { Serial.println(F("just did menu->update_ticks(ticks)")); Serial_flush(); }

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
  }
  //} else {
  if ((clock_mode!=CLOCK_INTERNAL || ticked) || (micros() + average_loop_micros) < (last_ticked_at_micros + micros_per_tick)) {
      // hmm actually if we just ticked then we potentially have MORE time to work with than if we havent just ticked..!
    #ifdef ENABLE_SCREEN
      //tft_update(ticks);
      ///Serial.println("going into menu->display and then pausing 1000ms: "); Serial_flush();
      static unsigned long last_drawn;
      bool screen_was_drawn = false;
      menu->update_inputs();
      if (millis() - last_drawn > MENU_MS_BETWEEN_REDRAW) {
        //long before_display = millis();
        if (debug) { Serial.println(F("about to menu->display")); Serial_flush(); }
        if (debug) menu->debug = true;
        menu->display(); //update(ticks);
        if (debug) { Serial.println(F("just did menu->display")); Serial_flush(); }
        //Serial.printf("display() took %ums..", millis()-before_display);
        last_drawn = millis();
        screen_was_drawn = true;
      }
      //delay(1000); Serial.println("exiting sleep after menu->display"); Serial_flush();
    #endif

    #ifdef ENABLE_CV_INPUT
      static unsigned long time_of_last_param_update = 0;
      if (!screen_was_drawn && millis() - time_of_last_param_update > TIME_BETWEEN_CV_INPUT_UPDATES) {
        if(debug) parameter_manager->debug = true;
        if(debug) Serial.println(F("about to do parameter_manager->update_voltage_sources()..")); Serial_flush();
        parameter_manager->update_voltage_sources();
        //if(debug) Serial.println("just did parameter_manager->update_voltage_sources().."); Serial_flush();
        //if(debug) Serial.println("about to do parameter_manager->update_inputs().."); Serial_flush();
        parameter_manager->update_inputs();
        //if(debug) Serial.println("about to do parameter_manager->update_mixers().."); Serial_flush();
        parameter_manager->update_mixers();
        if(debug) Serial.println(F("just did parameter_manager->update_inputs()..")); Serial_flush();
        time_of_last_param_update = millis();
      }
    #endif

  }

  //read_midi_serial_devices();
  //loop_midi_serial_devices();
  if (debug) Serial.println(F("about to behaviour_manager->do_reads().."));
  behaviour_manager->do_reads();
  if (debug) Serial.println(F("just did behaviour_manager->do_reads()"));

  if (debug) Serial.println(F("about to behaviour_manager->do_loops().."));
  behaviour_manager->do_loops();
  if (debug) Serial.println(F("just did behaviour_manager->do_loops()"));

  #ifdef ENABLE_USB
    update_usb_midi_device_connections();
    #ifdef ENABLE_USBSERIAL
      update_usbserial_device_connections();
    #endif 
    //read_midi_usb_devices();
    
    read_usb_from_computer();   // this is what sets should tick flag so should do this as early as possible before main loop start (or as late as possible in previous loop)
  #endif

  // process any events that are waiting from the usb keyboard handler
  process_key_buffer();

  #ifdef ENABLE_PROFILER
    main_loop_length_averages[count++] = micros() - start_loop_micros_stamp;
    uint32_t accumulator = 0;
    for (unsigned int i = 0 ; i < NUMBER_AVERAGES ; i++) {
      accumulator += main_loop_length_averages[i];
    }
    average_loop_micros = accumulator / NUMBER_AVERAGES;
    //Serial.printf("average_loop_micros got %u from accumulator %u divided by %i", average_loop_micros,  accumulator, NUMBER_AVERAGES);
    if (count>=NUMBER_AVERAGES) count = 0;
  #else
    average_loop_micros = micros() - start_loop_micros_stamp;
  #endif
  if(debug) { Serial.println(F("reached end of loop()!")); Serial_flush(); }
}

// called inside interrupt
void do_tick(uint32_t in_ticks) {
  bool debug = false;
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

  ticks = in_ticks;
  
  // original restart check+code went here? -- seems like better timing with bamble etc when call this here
  if (restart_on_next_bar && is_bpm_on_bar(ticks)) {
    if (debug) Serial.println(F("do_tick(): about to global_on_restart"));
    //in_ticks = ticks = 0;
    global_on_restart();
    //ATOMIC(
      //midi_apcmini->sendNoteOn(7, APCMINI_OFF, 1);
    //)
    restart_on_next_bar = false;
  }

  if (is_bpm_on_phrase(ticks)) {
    if (debug) Serial.println(F("do_tick(): about to project.on_phrase()"));
    project->on_phrase(BPM_CURRENT_PHRASE);
    #ifdef ENABLE_USB
      behaviour_manager->do_phrase(BPM_CURRENT_PHRASE);   //TODO: which of these is actually doing the work??
    #endif
  }
  if (is_bpm_on_bar(ticks)) {
    //project.on_bar(BPM_CURRENT_BAR_OF_PHRASE);
    if (debug) Serial.println(F("do_tick(): about to behaviour_manager->do_bar()"));
    behaviour_manager->do_bar(BPM_CURRENT_BAR_OF_PHRASE);
    if (debug) Serial.println(F("do_tick(): just did behaviour_manager->do_bar()"));
  } /*else if (is_bpm_on_bar(ticks+1)) {
    behaviour_manager->do_end_bar(BPM_CURRENT_BAR_OF_PHRASE);
    if (is_bpm_on_phrase(ticks+1)) {
      behaviour_manager->do_end_phrase(BPM_CURRENT_PHRASE);
    }
  }*/ else if (is_bpm_on_phrase(ticks+1)) {
    behaviour_manager->do_end_phrase_pre_clock(BPM_CURRENT_PHRASE);
  }

  if (debug) Serial.println(F("do_tick(): about to behaviour_manager->do_pre_clock()"));
  behaviour_manager->do_pre_clock(in_ticks);

  #ifdef ENABLE_DRUM_LOOPER
    drums_loop_track.process_tick(ticks);
  #endif

  //send_midi_serial_clocks();

  if (debug) { Serial.println(F("in do_tick() about to behaviour_manager->send_clocks()")); Serial_flush(); }
  behaviour_manager->send_clocks();
  if (debug) { Serial.println(F("in do_tick() just did behaviour_manager->send_clocks()")); Serial_flush(); }

  #ifdef ENABLE_CV_OUTPUT
    if (debug) { Serial.println(F("in do_tick() about to update_cv_outs()")); Serial_flush(); }
    update_cv_outs(in_ticks);
    if (debug) { Serial.println(F("in do_tick() just did update_cv_outs()")); Serial_flush(); }
  #endif

  if (debug) { Serial.println(F("in do_tick() about to behaviour_manager->do_ticks()")); Serial_flush(); }
  behaviour_manager->do_ticks(in_ticks);
  if (debug) { Serial.println(F("in do_tick() just did behaviour_manager->do_ticks()")); Serial_flush(); }

  // do this after everything else because of problems with machinegun mode..?
  if (is_bpm_on_bar(ticks+1)) {
    behaviour_manager->do_end_bar(BPM_CURRENT_BAR_OF_PHRASE);
    if (is_bpm_on_phrase(ticks+1)) {
      behaviour_manager->do_end_phrase(BPM_CURRENT_PHRASE);
    }
  }

  /*#ifdef ENABLE_USB2
    if (is_bpm_on_phrase(ticks+1)) {
      behaviour_manager->do_phrase(BPM_CURRENT_PHRASE+1);   //TODO: which of these is actually doing the work??
    }
  #endif*/

  #ifdef DEBUG_TICKS
    Serial.println(F(" ]"));
  #endif 

  //last_processed_tick = ticks;
  //ticks++;
  //last_ticked_at_micros = millis();
  //single_step = false;
}
