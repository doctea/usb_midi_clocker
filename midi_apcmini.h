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

void apcmini_note_on(byte inChannel, byte inNumber, byte inVelocity) {
  if (inNumber==0 && inVelocity==127) { // lower-left pad pressed
    Serial.println(F("APCmini pressed, restarting downbeat"));
    on_restart();
  }
}

void apcmini_control_change (byte inChannel, byte inNumber, byte inValue) {
  Serial.print(F("APCMINI CC ch"));
  Serial.print(inChannel);
  Serial.print(F("\tnum "));
  Serial.print(inNumber);
  Serial.print(F("\tvalue: "));
  Serial.println(inValue);

  if (inNumber==51) {
    bpm_current = map(inValue, 0, 127, 60, 140);
    Serial.print(F("set bpm to "));
    Serial.println(bpm_current);
    ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
  }
}

void apcmini_on_tick(unsigned long ticks) {
  static byte beat_counter;
  
  if (midi_apcmini) {
    midi_apcmini->sendClock();

    if (is_bpm_on_beat(ticks)) {
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
    } else if (is_bpm_on_beat(ticks,duration)) {
      midi_apcmini->sendNoteOn(beat_counter, 0, 1);
    }
  }
}

void apcmini_on_restart() {
  if (midi_apcmini) {
    midi_apcmini->sendStop();
    midi_apcmini->sendStart();
  }
}

void apcmini_init() {
    midi_apcmini->setHandleControlChange(apcmini_control_change);
    midi_apcmini->setHandleNoteOn(apcmini_note_on);
}
