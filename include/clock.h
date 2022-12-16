#include "Config.h"
#include "bpm.h"
//#include "midi/midi_outs.h"

#define CLOCK_INTERNAL          0
#define CLOCK_EXTERNAL_USB_HOST 1
#define CLOCK_NONE              2
#define NUM_CLOCK_SOURCES       3

#ifdef USE_UCLOCK

  //void send_midi_serial_clocks();

  #include <TimerThree.h>
  
  //#define ATOMIC(X) noInterrupts(); X; interrupts();
  #define ATOMIC(X) X

  // The callback function wich will be called by Clock each Pulse of 96PPQN clock resolution.
  /*void ClockOut96PPQN(volatile uint32_t *tick) {
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
  }*/

  void timer_tick();
  void setup_uclock();
  void set_new_bpm(float bpm_current);

#else
  
  extern int clock_mode;// = DEFAULT_CLOCK_MODE;

  /// use cheapclock clock
  extern uint32_t last_ticked_at_micros;
  void setup_cheapclock();

#endif
