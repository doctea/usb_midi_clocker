#include "bpm.h"

MIDI_NAMESPACE::MidiInterface<UHS2MIDI_NAMESPACE::uhs2MidiTransport> *midi_apcmini;
uint8_t ixAPCmini  = 0xff;

bool apcmini_started = false;

inline void apcmini_loop() {
  if ( ixAPCmini != 0xff) {
    do {
      Midi[ixAPCmini]->read();
    } while ( MidiTransports[ixAPCmini]->available() > 0);
  }  
}


void apcmini_control_change (byte inChannel, byte inNumber, byte inValue) {
  Serial.print("APCMINI CC ch");
  Serial.print(inChannel);
  Serial.print("\tnum ");
  Serial.print(inNumber);
  Serial.print("\tvalue: ");
  Serial.println(inValue);

  if (inNumber==51) {
    bpm_current = map(inValue, 0, 127, 60, 140);
    Serial.print("set bpm to ");
    Serial.println(bpm_current);
    ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
  }
}

void apcmini_on_tick(unsigned long ticks) {
  static byte beat_counter;
  
  if (ixAPCmini != 0xff && midi_apcmini) {
    midi_apcmini->sendClock();

    if (ticks%PPQN==0) {
      if (DEBUG_TICKS) {
        Serial.print(F("apcmini w/"));
        /*Serial.print(ticks);
        Serial.print(F("\tCounter is "));*/
        Serial.print(beat_counter);
        Serial.print(F(" "));
      }
      beat_counter = (byte)((ticks/PPQN) % 8); //(beat_counter+1)%8;
      midi_apcmini->sendNoteOn(beat_counter, 1, 1);
      //midi_apcmini->sendNoteOn(counter, 1, 1);
    } else if (ticks%PPQN==duration) {
      midi_apcmini->sendNoteOn(beat_counter, 0, 1);
    }
  }
}

void apcmini_init() {
    midi_apcmini->setHandleControlChange(apcmini_control_change);
}
