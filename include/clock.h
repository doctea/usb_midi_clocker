#ifndef CLOCK__INCLUDED
#define CLOCK__INCLUDED

#include "bpm.h"

#include <Arduino.h>

#define CLOCK_INTERNAL 0
#define CLOCK_EXTERNAL_USB_HOST 1
#define NUM_CLOCK_SOURCES 2

#ifdef USE_UCLOCK
  
  // The callback function wich will be called by Clock each Pulse of 96PPQN clock resolution.
  void ClockOut96PPQN(volatile uint32_t *tick) {
    do_tick(*tick);
  }
  
  // The callback function wich will be called when clock starts by using Clock.start() method.
  void onClockStart() {
    Serial.println(F("onClockStart()!"));
    //Serial.write(MIDI_START);
    ATOMIC(
      ticks = 0;
    )
    Serial.println(F("Clock started!"));
  }
  
  void onClockStop() {
    Serial.print(F("Clock stopped!"));
  }
  
  void setup_uclock() {
    // Inits the clock
    uClock.init();
    // Set the callback function for the clock output to send MIDI Sync message.
    uClock.setClock96PPQNOutput(ClockOut96PPQN);
    // Set the callback function for MIDI Start and Stop messages.
    uClock.setOnClockStartOutput(onClockStart);
    uClock.setOnClockStopOutput(onClockStop);
    // Set the clock BPM to 126 BPM
    uClock.setTempo(bpm_current);
  
    uClock.start();
  }

#else
  
  /// use cheapclock clock
  unsigned long t1 = millis();
  void setup_cheapclock() {
    ticks = 0;
    set_bpm(bpm_current);
  }

  int clock_mode = DEFAULT_CLOCK_MODE;

  // for handling external midi clock from host's usb
  bool serial_midi_clock_ticked = false;
  unsigned long last_serial_midi_clock_ticked_at;
  void serial_midi_handle_clock() {
    if (clock_mode==CLOCK_EXTERNAL_USB_HOST && serial_midi_clock_ticked) {
      Serial.print("WARNING: received a usb midi clock tick at ");
      Serial.print(millis());
      Serial.print("%u, but last one from ");
      Serial.print(last_serial_midi_clock_ticked_at);
      Serial.print(" was not yet processed (didn't process within gap of ");
      Serial.print(millis()-last_serial_midi_clock_ticked_at);
      Serial.print(")!\n");
    }
    /*if (CLOCK_EXTERNAL_USB_HOST) {  // TODO: figure out why this isn't working and fix
      tap_tempo_tracker.push_beat();
    }*/
    last_serial_midi_clock_ticked_at = millis();
    serial_midi_clock_ticked = true;
  }
  bool check_and_unset_usb_midi_clock_ticked() {
    bool v = serial_midi_clock_ticked;
    serial_midi_clock_ticked = false;
    /*if(clock_mode==CLOCK_EXTERNAL_USB_HOST && ticks%PPQN==0) {  // TODO: figure out why this isn't working and fix
      set_bpm(tap_tempo_tracker.bpm_calculate_current());
    }*/
    return v;
  }
  void serial_midi_handle_start() {
    if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
      //tap_tempo_tracker.reset();
      playing = true;
      on_restart();
    }
  }
  void serial_midi_handle_continue() {
    if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
      playing = true;
    }
  }
  void serial_midi_handle_stop() {
    if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
      playing = false;
    }
  }

#endif


#endif