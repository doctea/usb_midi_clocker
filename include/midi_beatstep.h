#ifdef ENABLE_BEATSTEP

#include "bpm.h"

MIDI_NAMESPACE::MidiInterface<UHS2MIDI_NAMESPACE::uhs2MidiTransport> *midi_beatstep;
volatile uint8_t ixBeatStep = 0xff;

volatile bool beatstep_started = false;

inline void beatstep_loop() {
  ATOMIC(
    if ( ixBeatStep != 0xff) {
      do {
        Midi[ixBeatStep]->read();
      } while ( MidiTransports[ixBeatStep]->available() > 0);
    }
  )
}

void beatstep_control_change (byte inChannel, byte inNumber, byte inValue) {
  if (inNumber==54 && inValue==127) {
    if (!TRUE_MIDI_SERIAL) Serial.println(F("BEATSTEP START PRESSED!"));
    beatstep_started = false;
  } else if (!TRUE_MIDI_SERIAL) {
    Serial.print(F("Received Beatstep CC "));
    Serial.print(inNumber);
    Serial.print(F(" with value "));
    Serial.println(inValue);
    //handleNoteOn(inChannel, inNumber, inValue);
  }
}

void beatstep_handle_start() {
  if (!TRUE_MIDI_SERIAL) Serial.println(F("beatstep_handle_start()"));
  //ATOMIC(
    midi_beatstep->sendStart();
  //)
  beatstep_started = true;
  if (!TRUE_MIDI_SERIAL) Serial.println(F("beatstep_handle_start() finished"));
}

void beatstep_on_tick(volatile uint32_t ticks) {
  //Serial.flush();
  
  if (midi_beatstep) {
    //Serial.print(F("beatstep_on_tick:"));
#ifdef DEBUG_TICKS
    if (DEBUG_TICKS) Serial.print(F(" beatstep "));
#endif
    //Serial.println("about to test bpm on bar..");
    //Serial.flush();
    if (is_bpm_on_bar(ticks) && !beatstep_started) {
      if (!TRUE_MIDI_SERIAL) Serial.println(F("First beat of bar and BEATSTEP not started -- starting!"));
      //Serial.println("First beat of bar and BEATSTEP not started -- starting!");
      //Serial.flush();
      //ATOMIC(
        midi_beatstep->sendStart();
      //);
      if (!TRUE_MIDI_SERIAL) Serial.println("sent start");
      //Serial.flush();
      beatstep_started = true;
    }

    //Serial.println("about to send clock message");
    //Serial.flush();
    //ATOMIC(
      midi_beatstep->sendClock();
    //);
    //Serial.println(F("sent clock"));
    
    //Serial.flush();
  } else {
    //Serial.println("..no midi_beatstep detected!");
    //Serial.flush();
  }
}

// called inside interrupt
void beatstep_on_restart() {
  if (midi_beatstep) {
    if (!TRUE_MIDI_SERIAL) Serial.println(F("beatstep_on_restart()"));
    //ATOMIC(
      midi_beatstep->sendStop();
      midi_beatstep->sendStart();
    //)
    if (!TRUE_MIDI_SERIAL) Serial.println(F("beatstep_on_restart done"));
  }
}

void beatstep_init() {
    if (!TRUE_MIDI_SERIAL) Serial.println(F("beatstep_init()"));
    ATOMIC(
      beatstep_started = false;
    )

    midi_beatstep->turnThruOff();
    midi_beatstep->setHandleControlChange(beatstep_control_change);
    midi_beatstep->setHandleStart(beatstep_handle_start);    
    if (!TRUE_MIDI_SERIAL) Serial.println(F("beatstep_init() finished"));
    //)
}

#endif
