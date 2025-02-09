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

#ifdef ENABLE_PARAMETERS
  // todo: probably rename cv_input.h to something else?
  #include "cv_input.h"
#endif
#include "ParameterManager.h"

#ifdef ENABLE_CV_OUTPUT
    #include "cv_output.h"
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
#include "interfaces/interfaces.h"
#include "cv_outs.h"

#ifdef ENABLE_USB
  #include "usb/multi_usb_handlers.h"
#endif
#ifdef ENABLE_USBSERIAL
  #include "usb/multi_usbserial_handlers.h"
#endif

#ifdef ENABLE_EUCLIDIAN
  #include "sequencer/Euclidian.h"
  #include "outputs/output.h"
  #include "outputs/output_processor.h"
  #include "sequencer/sequencing.h"
#endif

#include "ParameterManager.h"

#include "behaviours/behaviour_manager.h"

#include "input_keyboard.h"
#include "input_ali_controller.h"

#include "profiler.h"

#include "__version.h"

#ifdef ENABLE_PROFILER
  #define NUMBER_AVERAGES 1024
  uint32_t *main_loop_length_averages; //[NUMBER_AVERAGES];
  int count = 0;
#endif

//#define DEBUG_MAIN_LOOP
#ifdef DEBUG_MAIN_LOOP
  #define DEBUG_MAIN_PRINTLN(X) Serial_printf(X)
#else
  #define DEBUG_MAIN_PRINTLN(X) {}
#endif

#ifndef GDB_DEBUG
//FLASHMEM 
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
    Serial_println(F("Connected serial!")); Serial_flush();
  #endif
  if (CrashReport) {
    /*while (!Serial);
    Serial_println("CRASHREPORT!");
    Serial_print(CrashReport);
    while(Serial.available()==0);
    Serial.clear();*/
    setup_storage();
    log_crashreport();
  }
  delay(1);
  #ifdef DUMP_CRASHLOG_AT_STARTUP
    while (!Serial);
    delay(500);
    setup_storage();
    dump_crashreport_log();
  #endif

  /*while (1) {
    Serial_printf(".");
  }*/
  Serial_printf(F("At start of setup(), free RAM is %u\n"), freeRam()); Serial_flush();

  // this crashes when run from here...?  but kinda need it activated before setup_behaviour_manager() so that we can load cvoutputparameter calibration?
  // TODO: so: we need to be able to run something like "post-setup initialisation" on parameters at the end of setup, after everything else is set 
  //    this would load calibration for cvoutputparameters; and maybe also set up the default connections for parameters; and maybe also send the initial values to the cv outputs
  //    ..maybe we can do this at the end of setup_parameters()?
  //    1:30am WAIT WUT its actually loading the calibration values for the cvoutputparameters successfully now?!
  //    next day: wasn't working again so implemented load_all_parameters() in parameter_manager
  /*tft_print((char*)"..storage..\n");
  storage::setup_storage();
  Serial_printf(F("after setup_storage(), free RAM is %u\n"), freeRam());
  */

  //tft_print((char*)"..USB device handler..");
  // do this first, because need to have the behaviour classes instantiated before menu, as menu wants to call back to the behaviour_subclocker behaviours..
  Serial_println(F("..USB device handler.."));
  setup_behaviour_manager();
  Serial_printf(F("after setup_behaviour_manager(), free RAM is %u\n"), freeRam());

  Serial_println(F("..setup_saveable_parameters.."));
  behaviour_manager->setup_saveable_parameters();
  Serial_printf(F("after setup_saveable_parameters(), free RAM is %u\n"), freeRam());

  //Serial_println("..MIDIOutputWrapper manager..");
  //setup_midi_output_wrapper_manager();
  Serial_println(F("..MIDI matrix manager.."));
  //setup_midi_output_wrapper_manager();
  setup_midi_mapper_matrix_manager();
  Serial_printf(F("after setup_midi_mapper_matrix_manager(), free RAM is %u\n"), freeRam());

  #ifdef ENABLE_SCREEN
    //setup_tft();
    void setup_menu(bool button_pressed_state = LOW);
    setup_menu(LOW);
  #endif

  tft_print("PIO Env: " ENV_NAME "\n");
  tft_print("Git info: " COMMIT_INFO "\n");
  tft_print("Built at " __TIME__ " on " __DATE__ "\n");

  #ifdef ENABLE_CV_GATE_OUTPUT
    tft_print((char*)"Setting up CV gates..\n");
    //setup_cv_output();
    setup_gate_manager();
    setup_gate_manager_menus();
  #endif
  Debug_printf(F("after setup_gate_manager(), free RAM is %u\n"), freeRam());

  delay( 100 );

  /*#ifdef ENABLE_EUCLIDIAN
      tft_print("setting up EUCLIDIAN..!");
      delay(1000);
      setup_sequencer();
      output_processor->configure_sequencer(sequencer);
      #ifdef ENABLE_SCREEN
          setup_menu_euclidian(sequencer);
          setup_sequencer_menu();
          Debug_printf("after setup_sequencer_menu, free RAM is %u\n", freeRam());
      #endif
  #endif*/

  tft_print((char*)"..serial MIDI..\n");
  setup_midi_serial_devices();
  Serial_println(F("Serial ready."));   
  Debug_printf(F("after setup_midi_serial_devices(), free RAM is %u\n"), freeRam());

  tft_print((char*)"..storage..\n");
  storage::setup_storage();
  Debug_printf(F("after setup_storage(), free RAM is %u\n"), freeRam());

  tft_print((char*)"..setup project..\n");
  project->setup_project();
  Debug_printf(F("after setup_project(), free RAM is %u\n"), freeRam());

  #ifdef ENABLE_CV_INPUT
    setup_cv_input();
    Debug_printf(F("after setup_cv_input(), free RAM is %u\n"), freeRam());
  #endif
  #ifdef ENABLE_PARAMETERS
    setup_parameters();
    Debug_printf(F("after setup_parameters(), free RAM is %u\n"), freeRam());
  #endif
  /*#ifdef ENABLE_CV_OUTPUT
      setup_cv_output();
  #endif*/
  #ifdef ENABLE_CV_OUTPUT
    setup_cv_output_parameter_inputs();
    Debug_printf(F("after setup_cv_output_parameter_inputs(), free RAM is %u\n"), freeRam());
  #endif
  #ifdef ENABLE_SCREEN
    setup_parameter_menu();
    Debug_printf(F("after setup_parameter_menu(), free RAM is %u\n"), freeRam());
  #endif

  #ifdef ENABLE_SCREEN
    Serial_println(F("...starting behaviour_manager#make_menu_items...")); Serial_flush();
    behaviour_manager->create_all_behaviour_menu_items(menu);
    Serial_println(F("...finished behaviour_manager#make_menu_items...")); Serial_flush();
  #endif

  /*
  #ifdef ENABLE_SEQUENCER
    tft_print((char*)"..Sequencer..\n");
    init_sequence();
    Debug_printf(F("after init_sequence(), free RAM is %u\n"), freeRam());
  #endif
  */

  #ifdef USE_UCLOCK
    tft_print((char*)"Initialising uClock..\n");
    setup_uclock(&do_tick);
  #else
    tft_print((char*)"..Cheap clock..\n");
    setup_cheapclock();
  #endif

  tft_print((char*)"..PC USB..\n");
  setup_pc_usb();
  Debug_printf(F("after setup_pc_usb(), free RAM is %u\n"), freeRam());
  
  #ifdef ENABLE_USB
    tft_print((char*)"..USB..");
    setup_multi_usb();
    Serial_println(F("USB ready.")); Serial_flush();
    Debug_printf(F("after setup_multi_usb(), free RAM is %u\n"), freeRam());
  #endif

  #if defined(ENABLE_TYPING_KEYBOARD) or defined(ENABLE_CONTROLLER_KEYBOARD)
    tft_print((char*)"Setting up typing keyboard..\n"); Serial_flush();
    setup_typing_keyboard();
  #endif

  #ifdef ENABLE_USBSERIAL
    tft_print((char*)"..USBSerial..");
    setup_multi_usbserial();
    Serial_println(F("USBSerial ready.")); Serial_flush();
    Debug_printf(F("after setup_multi_usbserial(), free RAM is %u\n"), freeRam());
  #endif

  Serial_println(F("Arduino ready.")); Serial_flush();
  #ifdef ENABLE_SCREEN
    tft_print((char*)"Ready!"); 
    //tft_clear();

    Serial_println(F("About to init menu..")); Serial_flush();
    menu->start();
    Debug_printf(F("after menu->start(), free RAM is %u\n"), freeRam());
    //tft_start();

    menu->setup_quickjump();

    setup_debug_menu();
    Debug_printf(F("after setup_debug_menu(), free RAM is %u\n"), freeRam());

    menu->select_page(0);
  #endif

  #ifdef ENABLE_PROFILER
    Serial_printf("Allocating array for profiler");
    main_loop_length_averages = malloc(sizeof(uint32_t)*NUMBER_AVERAGES);
  #endif

  Serial_println(F("Finished setup()!"));
  Serial_printf(F("at end of setup(), free RAM is %u\n"), freeRam());

  #ifdef ENABLE_SCREEN
    snprintf(menu->last_message, MENU_C_MAX, "...started up, %u bytes free...", freeRam());
  #endif

  #ifdef LOAD_CALIBRATION_ON_BOOT
    parameter_manager->load_all_calibrations();
  #endif

  #ifdef USE_UCLOCK
    Serial_println("Starting uClock..."); Serial_flush();
    clock_start();
    Serial_println("Started uClock!"); Serial_flush();
  #endif

  debug_free_ram();

  pushButtonA.resetStateChange();
  pushButtonB.resetStateChange();
  pushButtonC.resetStateChange();
  
  Serial_println("Finished setup()!");
}

//long loop_counter = 0;


// -----------------------------------------------------------------------------
// 
// -----------------------------------------------------------------------------
void loop() {
  //ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    if (Serial.getReadError() || Serial.getWriteError()) {
        Serial.end();
        Serial.clearReadError(); Serial.clearWriteError();
        Serial.begin(115200);
        Serial.setTimeout(0);
    }
  }

  #if defined(ENABLE_TYPING_KEYBOARD) or defined(ENABLE_CONTROLLER_KEYBOARD)
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
  #endif

  //#ifdef ENABLE_PROFILER
    uint32_t start_loop_micros_stamp = micros();
  //#endif
  //bool debug = true;
  if (debug_flag) { Serial_println(F("start of loop!")); Serial_flush(); }

  #ifdef DEBUG_LED
    static int loop_counter;
    static bool lit = false;
    loop_counter++;
    if (loop_counter%1000000==0) {
      digitalWrite(LED_BUILTIN, lit);
      lit = !lit;
      //Serial_println(F("100000th loop()"));
    }
  #endif

  #ifdef ENABLE_USB
    if (debug_flag) { Serial_println(F("about to Usb.Task()")); Serial_flush(); }
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) 
    {
      Usb.Task();
    }
    if (debug_flag) { Serial_println(F("just did Usb.Task()")); Serial_flush(); }
  #endif
  //static unsigned long last_ticked_at_micros = 0;

  bool ticked = false;
  //ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    if (debug_flag) { Serial_println(F("about to update_clock_ticks")); Serial_flush(); }
    ticked = update_clock_ticks();
    if (debug_flag) { Serial_println(F("just did update_clock_ticks")); Serial_flush(); }
  
    #ifdef USE_UCLOCK
      // do_tick is called from interrupt via uClock, so we don't need to do it manually here
      // do, however, tell the menu to update stuff if a new tick has happend
    #else
      if ( playing && ticked ) {
        if (debug_flag) { Serial_println(F("about to do_tick")); Serial_flush(); }
        do_tick(ticks);
        if (debug_flag) { Serial_println(F("just did do_tick")); Serial_flush(); }

        last_ticked_at_micros = micros();
      }
    #endif
    #ifdef ENABLE_SCREEN
      if (playing && ticked) {
          if (debug_flag) { Serial_println(F("about to menu->update_ticks")); Serial_flush(); }
          menu->update_ticks(ticks);
          if (debug_flag) { Serial_println(F("just did menu->update_ticks")); Serial_flush(); }
      }
    #endif
  }
  
  if (!playing || (clock_mode!=CLOCK_INTERNAL || ticked) || (micros() + average_loop_micros) < (last_ticked_at_micros + micros_per_tick)) {
    // hmm actually if we just ticked then we potentially have MORE time to work with than if we havent just ticked..!
    #ifdef ENABLE_SCREEN
      ///Serial_println("going into menu->display and then pausing 1000ms: "); Serial_flush();
      static unsigned long last_drawn;
      bool screen_was_drawn = false;      //ATOMIC_BLOCK(ATOMIC_RESTORESTATE) 
      {
        if (debug_flag) { Serial_println(F("about to menu->update_inputs")); Serial_flush(); }
        static bool first_run = true;
        if (!first_run) {
          menu->update_inputs();
        }
        first_run = false;
        if (debug_flag) { Serial_println(F("just did menu->update_inputs")); Serial_flush(); }
      }
      if (/*(ticked && is_bpm_on_beat(ticks)) || */millis() - last_drawn > MENU_MS_BETWEEN_REDRAW) {
        //long before_display = millis();
        if (debug_flag) { Serial_println(F("about to menu->display")); Serial_flush(); }
        if (debug_flag) menu->debug = true;
        //ATOMIC_BLOCK(ATOMIC_RESTORESTATE) 
        {
          menu->display(); //update(ticks);
          if (debug_flag) { Serial_println(F("just did menu->display")); Serial_flush(); }
          //Serial_printf("display() took %ums..", millis()-before_display);
          last_drawn = millis();
          screen_was_drawn = true;
        }
      }
      //delay(1000); Serial_println("exiting sleep after menu->display"); Serial_flush();
    #endif

    //#ifdef ENABLE_CV_INPUT
      //ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
        //if (!screen_was_drawn) {
          //__disable_irq();
          //parameter_manager->throttled_update_cv_inputs(TIME_BETWEEN_CV_INPUT_UPDATES);
          if (debug_flag) { Serial_println(F("about to parameter_manager->throttled_update_cv_input__all")); Serial_flush(); }
          parameter_manager->throttled_update_cv_input__all(TIME_BETWEEN_CV_INPUT_UPDATES, false, false);
          if (debug_flag) { Serial_println(F("just did parameter_manager->throttled_update_cv_input__all")); Serial_flush(); }
          //__enable_irq();                              
        //}
        if (!playing) {
          //parameter_manager->update_mixers();
        }
      }
    //#endif
  }

  // only update from main loop if we're paused, so that we can still see effect of manual updating of gates etc
  //ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    if (!playing) {
      if (debug_flag) { Serial_println(F("about to gate_manager->update();")); Serial_flush(); }
      gate_manager->update(); 
      if (debug_flag) { Serial_println(F("just did gate_manager->update();")); Serial_flush(); }
    }
  }

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) 
  {
    if (debug_flag) { Serial_println(F("about to behaviour_manager->do_reads()..")); Serial_flush(); }
    behaviour_manager->do_reads();
    if (debug_flag) { Serial_println(F("just did behaviour_manager->do_reads()")); Serial_flush(); }
  }

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  { 
    if (debug_flag) { Serial_println(F("about to behaviour_manager->do_loops()..")); Serial_flush(); }
    behaviour_manager->do_loops();
    if (debug_flag) { Serial_println(F("just did behaviour_manager->do_loops()")); Serial_flush(); }
  }

  #ifdef ENABLE_USB
    //ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
      if (debug_flag) { Serial_println(F("about to update_usb_midi_device_connections();..")); Serial_flush(); }
      update_usb_midi_device_connections();
      if (debug_flag) { Serial_println(F("just did update_usb_midi_device_connections();..")); Serial_flush(); }
      #ifdef ENABLE_USBSERIAL
        if (debug_flag) { Serial_println(F("about to update_usbserial_device_connections();..")); Serial_flush(); }
        update_usbserial_device_connections();
        if (debug_flag) { Serial_println(F("just did update_usbserial_device_connections();..")); Serial_flush(); }
      #endif 
      //Serial_println("finished calling update_usb_xxx_device_connections.."); Serial_flush();
      //read_midi_usb_devices();
    }

    //ATOMIC_BLOCK(ATOMIC_RESTORESTATE) 
    {      
      if (debug_flag) { Serial_println(F("about to read_usb_from_computer();..")); Serial_flush(); }
      read_usb_from_computer();   // this is what sets should tick flag so should do this as early as possible before main loop start (or as late as possible in previous loop)
      if (debug_flag) { Serial_println(F("just did read_usb_from_computer();..")); Serial_flush(); }
    }
  #endif

  #if defined(ENABLE_TYPING_KEYBOARD) or defined(ENABLE_CONTROLLER_KEYBOARD)
    // process any events that are waiting from the usb keyboard handler
    if (debug_flag) { Serial_println(F("about to process_key_buffer();..")); Serial_flush(); }
    process_key_buffer();
    if (debug_flag) { Serial_println(F("just did process_key_buffer();..")); Serial_flush(); }
  #endif

  #ifdef ENABLE_PROFILER
    main_loop_length_averages[count++] = micros() - start_loop_micros_stamp;
    uint32_t accumulator = 0;
    for (unsigned int i = 0 ; i < NUMBER_AVERAGES ; i++) {
      accumulator += main_loop_length_averages[i];
    }
    average_loop_micros = accumulator / NUMBER_AVERAGES;
    //Serial_printf("average_loop_micros got %u from accumulator %u divided by %i", average_loop_micros,  accumulator, NUMBER_AVERAGES);
    if (count>=NUMBER_AVERAGES) count = 0;
  #else
    average_loop_micros = micros() - start_loop_micros_stamp;
  #endif
  if(debug_flag) { Serial_println(F("reached end of loop()!")); Serial_flush(); }
}

// (should be) called inside interrupt
void do_tick(uint32_t in_ticks) {
  uint32_t start_time = micros();
  //ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {

  bool debug = debug_flag;
  /*#ifdef DEBUG_TICKS
      unsigned int delta = millis()-last_ticked_at_micros;

      Serial_print(ticks);
      Serial_print(F(":\tTicked with delta\t"));
      Serial_print(delta);
      Serial_print(F("!\t(ms_per_tick is "));
      Serial_print(ms_per_tick);
      Serial_print(F(") sending clock for [ "));
  #endif*/
  //Serial_println("ticked");

  ::ticks = in_ticks;
  
  // original restart check+code went here? -- seems like better timing with bamble etc when call this here
  if (is_restart_on_next_bar() && is_bpm_on_bar(ticks)) {
    DEBUG_MAIN_PRINTLN(F("do_tick(): about to global_on_restart"));
    global_on_restart();
    set_restart_on_next_bar(false);
  }

  if (is_bpm_on_phrase(ticks)) {
    DEBUG_MAIN_PRINTLN(F("do_tick(): about to project.on_phrase()"));
    project->on_phrase(BPM_CURRENT_PHRASE);
    #ifdef ENABLE_USB
      behaviour_manager->do_phrase(BPM_CURRENT_PHRASE);   //TODO: which of these is actually doing the work??
    #endif
  }
  if (is_bpm_on_bar(ticks)) {
    //project.on_bar(BPM_CURRENT_BAR_OF_PHRASE);
    DEBUG_MAIN_PRINTLN(F("do_tick(): about to behaviour_manager->do_bar()"));
    behaviour_manager->do_bar(BPM_CURRENT_BAR_OF_PHRASE);
    DEBUG_MAIN_PRINTLN(F("do_tick(): just did behaviour_manager->do_bar()"));
  } else if (is_bpm_on_phrase(ticks+1)) {
    behaviour_manager->do_end_phrase_pre_clock(BPM_CURRENT_PHRASE);
  }

  DEBUG_MAIN_PRINTLN(F("do_tick(): about to behaviour_manager->do_pre_clock()"));
  behaviour_manager->do_pre_clock(in_ticks);

  #ifdef ENABLE_LOOPER
    midi_loop_track.process_tick(ticks);
  #endif

  // drone / machinegun works when do_end_bar here !
  // do this after everything else because of problems with machinegun mode..?
  if (is_bpm_on_beat(ticks+1)) {
    behaviour_manager->do_end_beat(BPM_CURRENT_BEAT);
    if (is_bpm_on_bar(ticks+1)) {
      behaviour_manager->do_end_bar(BPM_CURRENT_BAR_OF_PHRASE);
      if (is_bpm_on_phrase(ticks+1)) {
        behaviour_manager->do_end_phrase(BPM_CURRENT_PHRASE);
      }
    } 
  }

  #ifdef ENABLE_DRUM_LOOPER
    drums_loop_track.process_tick(ticks);
  #endif

  if (debug) { DEBUG_MAIN_PRINTLN(F("in do_tick() about to behaviour_manager->send_clocks()")); Serial_flush(); }
  behaviour_manager->send_clocks();
  if (debug) { DEBUG_MAIN_PRINTLN(F("in do_tick() just did behaviour_manager->send_clocks()")); Serial_flush(); }

  gate_manager->update(); 

  // done doesn't end properly for usb behaviours if do_end_bar here!

  /*#ifdef ENABLE_CV_OUTPUT
    if (debug) {DEBUG_MAIN_PRINTLN(F("in do_tick() about to update_cv_outs()")); Serial_flush(); }
    //ATOMIC_BLOCK(ATOMIC_RESTORESTATE) 
    {
      update_cv_outs(in_ticks);
    //}
    if (debug) { DEBUG_MAIN_PRINTLN(F("in do_tick() just did update_cv_outs()")); Serial_flush(); }
    //ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      gate_manager->update(); 
    }
  #endif*/

  if (debug) { DEBUG_MAIN_PRINTLN(F("in do_tick() about to behaviour_manager->do_ticks()")); Serial_flush(); }
  behaviour_manager->do_ticks(in_ticks);
  if (debug) { DEBUG_MAIN_PRINTLN(F("in do_tick() just did behaviour_manager->do_ticks()")); Serial_flush(); }

  //parameter_manager->update_mixers();

  /*
  // done doesn't end properly for usb behaviours if do_end_bar here!
  // do this after everything else because of problems with machinegun mode..?
  if (is_bpm_on_bar(ticks+1)) {
    behaviour_manager->do_end_bar(BPM_CURRENT_BAR_OF_PHRASE);
    if (is_bpm_on_phrase(ticks+1)) {
      behaviour_manager->do_end_phrase(BPM_CURRENT_PHRASE);
    }
  }*/

  /*#ifdef ENABLE_USB2
    if (is_bpm_on_phrase(ticks+1)) {
      behaviour_manager->do_phrase(BPM_CURRENT_PHRASE+1);   //TODO: which of these is actually doing the work??
    }
  #endif*/

  #ifdef DEBUG_TICKS
    Serial_println(F(" ]"));
  #endif 

  if (parameter_manager->pending_calibration()) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      parameter_manager->process_calibration();
    }
  }

  //last_processed_tick = ticks;
  //ticks++;
  //last_ticked_at_micros = millis();
  //single_step = false;
  //}
  uint32_t time_to_tick = micros() - start_time;
  if (time_to_tick>=micros_per_tick)
    Serial_printf("WARNING: Took %ius to tick, needs to be <%ius!\n", time_to_tick, micros_per_tick);
}
