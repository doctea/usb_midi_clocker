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
    Serial.print("Beatstep");
    //handleNoteOn(inChannel, inNumber, inValue);
  }
}

void beatstep_handle_start() {
  midi_beatstep->sendStart();
  beatstep_started = true;
}

void beatstep_on_tick(unsigned long ticks) {
  if (midi_beatstep) {
    if (DEBUG_TICKS) Serial.print(F(" beatstep "));
    if (ticks%(PPQN*4)==0 && !beatstep_started) {
      Serial.println("First beat of bar and BEATSTEP not started -- starting!");
      midi_beatstep->sendStart();
      beatstep_started = true;
    }
    
    midi_beatstep->sendClock();
  }
}

void beatstep_on_restart() {
  if (midi_beatstep) {
    midi_beatstep->sendStop();
    midi_beatstep->sendStart();
  }
}

void beatstep_init() {
    beatstep_started = false;

    midi_beatstep->setHandleControlChange(beatstep_control_change);
    midi_beatstep->setHandleStart(beatstep_handle_start);    
}
