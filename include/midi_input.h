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
  
  pinMode(PIN_A0, INPUT_PULLUP);
  pinMode(PIN_A1, INPUT_PULLUP);

  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
  digitalWrite(LED_BUILTIN, HIGH);

  if (TRUE_MIDI_SERIAL || (digitalRead(PIN_A0) && !digitalRead(PIN_A1))) {
    Serial.begin(31250);
    #if defined(ENABLE_CLOCKS) || defined(ENABLE_SEQUENCER)
      digitalWrite(PIN_CLOCK_1, HIGH);
      digitalWrite(PIN_CLOCK_2, HIGH);
      digitalWrite(PIN_CLOCK_3, HIGH);
      digitalWrite(PIN_CLOCK_4, HIGH);
      delay(1000);
      digitalWrite(PIN_CLOCK_1, LOW);
      digitalWrite(PIN_CLOCK_2, LOW);
      digitalWrite(PIN_CLOCK_3, LOW);
      digitalWrite(PIN_CLOCK_4, LOW);
    #endif
  } else {
    Serial.begin(BAUDRATE_HAIRLESS);
    /*while (!Serial);
    Serial.println("Started..");*/
  }

  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);

}

void loop_serial_midi() {
    MIDI.read();
}

