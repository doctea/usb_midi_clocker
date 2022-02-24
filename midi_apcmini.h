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

    if (is_bpm_on_eighth(ticks)) {
      if (DEBUG_TICKS) {
        Serial.print(F("apcmini w/"));
        /*Serial.print(ticks);
        Serial.print(F("\tCounter is "));*/
        Serial.print(beat_counter);
        Serial.print(F(" "));
      }
      beat_counter = (byte)((ticks/(PPQN)) % NUM_STEPS); //(beat_counter+1)%8;
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


unsigned long last_updated_display = 0;

void apcmini_clear_display() {
  Serial.println("Clearing APC display..");
  for (byte x = 0 ; x < 8 ; x++) {
    for (byte y = 0 ; y < 8 ;y++) {
      midi_apcmini->sendNoteOn(x+(y*8), 0, 1);
    }
  }
}

void apcmini_update_clock() {
  // draw the clock divisions
  /*for (byte c = 0 ; c < NUM_CLOCKS ; c++) {
    //byte start_row = (8-NUM_CLOCKS) * 8;
    byte start_row = 64-((c+1)*8);
    for (byte i = 0 ; i < 8 ; i++) {
      float cm = clock_multiplier[c] / 2;
      if (cm<0.5) {
        midi_apcmini->sendNoteOn(start_row+i, 2, 1);
      } else if (cm<1.0) {
        midi_apcmini->sendNoteOn(start_row+i, 3, 1);
      } else if (i%(byte)cm==0) {
        midi_apcmini->sendNoteOn(start_row+i, 1, 1);
      } else {
        midi_apcmini->sendNoteOn(start_row+i, 0, 1);
      }
    }
  }*/

  for (byte c = 0 ; c < NUM_SEQUENCES ; c++) {
    //byte start_row = (8-NUM_CLOCKS) * 8;
    byte start_row = 64-((c+1)*8);
    for (byte i = 0 ; i < NUM_STEPS ; i++) {
      byte level = 0;
      /*if (sequence[c][i*2]>1 && sequence[c][(i*2)+1]>1) {
        level = 3;
      } else if (sequence[c][i]>1) {
        level = 1;
      } else {
        level = 0;
      }*/
      if (sequence[c][i]>0) {
        midi_apcmini->sendNoteOn(start_row+i, 1, 1);
      } else {
        midi_apcmini->sendNoteOn(start_row+i, 0, 1);
      }
    }
  }
  last_updated_display = ticks;
}


void apcmini_init() {
    midi_apcmini->setHandleControlChange(apcmini_control_change);
    midi_apcmini->setHandleNoteOn(apcmini_note_on);
    apcmini_clear_display();
}
