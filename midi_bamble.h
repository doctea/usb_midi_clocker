#include "bpm.h"

MIDI_NAMESPACE::MidiInterface<UHS2MIDI_NAMESPACE::uhs2MidiTransport> *midi_bamble;
uint8_t ixBamble   = 0xff;

bool bamble_started = false;

inline void bamble_loop() {
  if ( ixBamble != 0xff) {
    do {
      Midi[ixBamble]->read();
    } while ( MidiTransports[ixBamble]->available() > 0);
  }
}

void bamble_on_tick(unsigned long ticks) {
  if (ixBamble != 0xff && midi_bamble) {
    if (DEBUG_TICKS) Serial.print(F(" bamble "));
    if (ticks%(PPQN*4)==0 && !bamble_started) {
      Serial.println(F("First beat of bar and BEATSTEP not started -- starting!"));
      midi_bamble->sendStart();
      bamble_started = true;
    }
    
    midi_bamble->sendClock();
  }
}

void bamble_init() {
    bamble_started = false;

    //midi_bamble->setHandleControlChange(bamble_control_change);
    //midi_bamble->setHandleStart(bamble_handle_start);    
}
