#include "bpm.h"

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

#endif
