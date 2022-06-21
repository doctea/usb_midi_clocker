#include <Arduino.h>
#include "Config.h"

#include "clock.h"

#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

void setup_serial_midi_input() {
  MIDI.setHandleClock(serial_midi_handle_clock);
  MIDI.setHandleStart(serial_midi_handle_start);
  MIDI.setHandleStop(serial_midi_handle_stop);
  MIDI.setHandleContinue(serial_midi_handle_continue);

  MIDI.begin(MIDI_CHANNEL_OMNI);
  #ifdef TRUE_MIDI_SERIAL
    Serial.begin(32150);
  #else
    Serial.begin(BAUDRATE_HAIRLESS);
  #endif
}

void loop_serial_midi() {
    MIDI.read();
}

