#include "bpm.h"

MIDI_NAMESPACE::MidiInterface<UHS2MIDI_NAMESPACE::uhs2MidiTransport> *midi_beatstep;
uint8_t ixBeatStep = 0xff;

bool beatstep_started = false;

inline void beatstep_loop() {
  if ( ixBeatStep != 0xff) {
    do {
      Midi[ixBeatStep]->read();
    } while ( MidiTransports[ixBeatStep]->available() > 0);
  }
}

void beatstep_control_change (byte inChannel, byte inNumber, byte inValue) {
  if (inNumber==54 && inValue==127) {
    Serial.println(F("BEATSTEP START PRESSED!"));
    beatstep_started = false;
  } else {
    Serial.print(F("Received Beatstep CC "));
    Serial.print(inNumber);
    Serial.print(F(" with value "));
    Serial.println(inValue);
    //handleNoteOn(inChannel, inNumber, inValue);
  }
}

void beatstep_handle_start() {
  midi_beatstep->sendStart();
  beatstep_started = true;
}

void beatstep_on_tick(unsigned long ticks) {
  //Serial.print("beatstep_on_tick:");
  //Serial.flush();
  if (midi_beatstep) {
    //Serial.println("..midi_beatstep set..");
#ifdef DEBUG_TICKS
    if (DEBUG_TICKS) Serial.print(F(" beatstep "));
#endif
    //Serial.println("about to test bpm on bar..");
    //Serial.flush();
    if (is_bpm_on_bar(ticks) && !beatstep_started) {
      //Serial.println(F("First beat of bar and BEATSTEP not started -- starting!"));
      //Serial.println("First beat of bar and BEATSTEP not started -- starting!");
      //Serial.flush();
      ATOMIC(midi_beatstep->sendStart());
      //Serial.println("sent start");
      //Serial.flush();
      beatstep_started = true;
    }

    //Serial.println("about to send clock message");
    //Serial.flush();
    ATOMIC(midi_beatstep->sendClock());
    //Serial.println("sent clock");
    //Serial.flush();
  } else {
    //Serial.println("..no midi_beatstep detected!");
    //Serial.flush();
  }
}

void beatstep_on_restart() {
  if (midi_beatstep) {
    ATOMIC(
      midi_beatstep->sendStop();
      midi_beatstep->sendStart();
    )
  }
}

void beatstep_init() {
    //ATOMIC(
      beatstep_started = false;

      midi_beatstep->turnThruOff();
      midi_beatstep->setHandleControlChange(beatstep_control_change);
      midi_beatstep->setHandleStart(beatstep_handle_start);    
    //)
}
