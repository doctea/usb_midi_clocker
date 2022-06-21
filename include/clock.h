#include "bpm.h"

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
  bool usb_midi_clock_ticked = false;
  unsigned long last_usb_midi_clock_ticked_at;
  void usb_midi_handle_clock() {
    if (clock_mode==CLOCK_EXTERNAL_USB_HOST && usb_midi_clock_ticked) {
      Serial.printf("WARNING: received a usb midi clock tick at %u, but last one from %u was not yet processed (didn't process within gap of %u)!\n", millis(), last_usb_midi_clock_ticked_at, millis()-last_usb_midi_clock_ticked_at);
    }
    /*if (CLOCK_EXTERNAL_USB_HOST) {  // TODO: figure out why this isn't working and fix
      tap_tempo_tracker.push_beat();
    }*/
    last_usb_midi_clock_ticked_at = millis();
    usb_midi_clock_ticked = true;
  }
  bool check_and_unset_usb_midi_clock_ticked() {
    bool v = usb_midi_clock_ticked;
    usb_midi_clock_ticked = false;
    /*if(clock_mode==CLOCK_EXTERNAL_USB_HOST && ticks%PPQN==0) {  // TODO: figure out why this isn't working and fix
      set_bpm(tap_tempo_tracker.bpm_calculate_current());
    }*/
    return v;
  }
  void usb_midi_handle_start() {
    if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
      //tap_tempo_tracker.reset();
      playing = true;
      on_restart();
    }
  }
  void usb_midi_handle_continue() {
    if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
      playing = true;
    }
  }
  void usb_midi_handle_stop() {
    if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
      playing = false;
    }
  }

#endif
