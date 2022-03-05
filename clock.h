#include "bpm.h"


// The callback function wich will be called by Clock each Pulse of 96PPQN clock resolution.
void ClockOut96PPQN(uint32_t * tick) {
  // Send MIDI_CLOCK to external gears
  //Serial.write(MIDI_CLOCK);
  //Serial.print(F("ClockOut96PPQN ticked "));
  //Serial.println(*tick);
  //if (*tick % 6 == 0) 
  //  Serial.println("Ticked on 6th?"); //a quarter note?");
  do_tick(*tick);
}

// The callback function wich will be called when clock starts by using Clock.start() method.
void onClockStart() {
  //Serial.write(MIDI_START);
  Serial.println("uClock started!");
}

void onClockStop() {
  Serial.print("uClock stopped!");
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
