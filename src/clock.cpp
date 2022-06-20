#include "Config.h"
#include "clock.h"
#include "midi_outs.h"

#ifdef USE_UCLOCK

  void timer_tick() {
    ticks++;
    //Serial.printf("ticked %i\n", ticks); 
    //send_midi_serial_clocks();
    //send_midi_usb_clocks();
  }
  
  void setup_uclock() {
    // Inits the clock
    //uClock.init();
    // Set the callback function for the clock output to send MIDI Sync message.
    Timer3.attachInterrupt(timer_tick);
    Timer3.initialize(1000 * 1000 * 60 / bpm_current / PPQN);
    /*uClock.setClock96PPQNOutput(ClockOut96PPQN);
    // Set the callback function for MIDI Start and Stop messages.
    uClock.setOnClockStartOutput(onClockStart);
    uClock.setOnClockStopOutput(onClockStop);
    // Set the clock BPM to 126 BPM
    uClock.setTempo(bpm_current);
  
    uClock.start();*/
  }

  void set_new_bpm(float bpm_current) {
    Timer3.setPeriod(1000 * 1000 * 60 / bpm_current / PPQN);
  }

#else
  
  int clock_mode = DEFAULT_CLOCK_MODE;

  /// use cheapclock clock
  unsigned long t1 = millis();
  void setup_cheapclock() {
    ticks = 0;
    set_bpm(bpm_current);
  }

#endif
