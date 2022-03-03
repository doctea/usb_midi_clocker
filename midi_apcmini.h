#include "bpm.h"

MIDI_NAMESPACE::MidiInterface<UHS2MIDI_NAMESPACE::uhs2MidiTransport> *midi_apcmini;
uint8_t ixAPCmini  = 0xff;

bool apcmini_started = false;
bool apcmini_shift_held = false;

bool restart_on_next_bar = false;

int clock_selected = 0;

bool redraw_immediately = false;
unsigned long last_updated_display = 0;

void apcmini_update_clock_display();

inline void apcmini_loop() {
  if ( ixAPCmini != 0xff) {
    do {
      Midi[ixAPCmini]->read();
    } while ( MidiTransports[ixAPCmini]->available() > 0);
  }  
}

#define APCMINI_NUM_ROWS      8
#define APCMINI_DISPLAY_WIDTH 8

#define APCMINI_BUTTON_CLIP_STOP 82
#define APCMINI_BUTTON_SOLO      83
#define APCMINI_BUTTON_REC_ARM   84
#define APCMINI_BUTTON_MUTE      85
#define APCMINI_BUTTON_SELECT    86
#define APCMINI_BUTTON_UNLABELED_1    87
#define APCMINI_BUTTON_UNLABELED_2    88
#define APCMINI_BUTTON_STOP_ALL_CLIPS 89
#define APCMINI_BUTTON_SHIFT     98
#define APCMINI_BUTTON_UP        64
#define APCMINI_BUTTON_DOWN      65
#define APCMINI_BUTTON_LEFT      66
#define APCMINI_BUTTON_RIGHT     67

// button colours from https://remotify.io/community/question/led-feedback-values
#define APCMINI_OFF           0
#define APCMINI_GREEN         1
#define APCMINI_GREEN_BLINK   2
#define APCMINI_RED           3
#define APCMINI_RED_BLINK     4
#define APCMINI_YELLOW        5
#define APCMINI_YELLOW_BLINK  6

void apcmini_note_on(byte inChannel, byte inNumber, byte inVelocity) {
  if (inNumber==APCMINI_BUTTON_STOP_ALL_CLIPS && inVelocity==127) {
    playing = !playing;
  } else if (inNumber==0 && inVelocity==127) { // lower-left pad pressed
    Serial.println(F("APCmini pressed, restarting downbeat"));
    on_restart();
  } else if (inNumber==7 && inVelocity==127) {
    Serial.println(F("APCmini pressed, restarting downbeat on next bar"));
    midi_apcmini->sendNoteOn(7, APCMINI_GREEN_BLINK, 1);
    restart_on_next_bar = true;
  } else if (inNumber==APCMINI_BUTTON_UP) {
    redraw_immediately = true;
    clock_selected--;
    if (clock_selected<0)
      clock_selected = NUM_CLOCKS-1;
  } else if (inNumber==APCMINI_BUTTON_DOWN) {
    redraw_immediately = true;
    clock_selected++;
    if (clock_selected>=NUM_CLOCKS) 
      clock_selected = 0;
  } else if (inNumber==APCMINI_BUTTON_LEFT) {
    redraw_immediately = true;
    clock_delay[clock_selected] -= 1;
    if (clock_delay[clock_selected]<0)
      clock_delay[clock_selected] = 7;
    Serial.print(F("Set selected clock delay to "));
    Serial.println(clock_delay[clock_selected]);
  } else if (inNumber==APCMINI_BUTTON_RIGHT) {
    redraw_immediately = true;
    clock_delay[clock_selected] += 1;
    if (clock_delay[clock_selected]>7)
      clock_delay[clock_selected] = 0;
    Serial.print(F("Set selected clock delay to "));
    Serial.println(clock_delay[clock_selected]);
  } else if (inNumber==APCMINI_BUTTON_UNLABELED_1 && inVelocity==127) {
    single_step = true;
    //ticks += 1;
    Serial.print(F("Single-stepped to tick "));
    Serial.println(ticks);
  } else if ((inNumber>=APCMINI_BUTTON_CLIP_STOP && inNumber<= APCMINI_BUTTON_MUTE) && inVelocity==127) {  // Clip Stop -> Solo -> Rec arm -> Mute buttons
    byte clock_number = inNumber - APCMINI_BUTTON_CLIP_STOP;  
    if (apcmini_shift_held) {
      clock_multiplier[clock_number] /= 2;    // halve the selected clock multiplier
    } else {
      clock_multiplier[clock_number] *= 2;   // double the selected clock multiplier
    }
    
    if (clock_multiplier[clock_number]>CLOCK_MULTIPLIER_MAX)
      clock_multiplier[clock_number] = CLOCK_MULTIPLIER_MIN;
    if (clock_multiplier[clock_number]<CLOCK_MULTIPLIER_MIN) 
      clock_multiplier[clock_number] = CLOCK_MULTIPLIER_MAX;
      
    apcmini_update_clock_display();
  } else if (inNumber==APCMINI_BUTTON_SHIFT && inVelocity==127) {
    apcmini_shift_held = true;
  } else {
    Serial.println(inNumber);//if (inNumber<(8*8) && inNumber>=(8*5)) {
  }

  if (redraw_immediately) 
    last_updated_display = 0;
}

void apcmini_note_off(byte inChannel, byte inNumber, byte inVelocity) {
  if (inNumber==APCMINI_BUTTON_SHIFT) {
    apcmini_shift_held = false;
  }
}

void apcmini_control_change (byte inChannel, byte inNumber, byte inValue) {
  Serial.print(F("APCMINI CC ch"));
  Serial.print(inChannel);
  Serial.print(F("\tnum "));
  Serial.print(inNumber);
  Serial.print(F("\tvalue: "));
  Serial.println(inValue);

  if (inNumber==56) {   // 56 == "master" fader set bpm 
    set_bpm(map(inValue, 0, 127, BPM_MINIMUM, BPM_MAXIMUM)); // scale CC value
  }
}

void apcmini_on_tick(unsigned long ticks) {
  static byte beat_counter;
  
  if (midi_apcmini) {
    midi_apcmini->sendClock();

    if (restart_on_next_bar && is_bpm_on_bar(ticks)) {
      on_restart();
      midi_apcmini->sendNoteOn(7, APCMINI_OFF, 1);
      restart_on_next_bar = false;
    }

    if (is_bpm_on_beat(ticks)) {
      if (DEBUG_TICKS) {
        Serial.print(F("apcmini w/"));
        /*Serial.print(ticks);
        Serial.print(F("\tCounter is "));*/
        Serial.print(beat_counter);
        Serial.print(F(" "));
      }
      beat_counter = (byte)((ticks/PPQN) % APCMINI_DISPLAY_WIDTH); //(beat_counter+1)%8;
      midi_apcmini->sendNoteOn(beat_counter, APCMINI_GREEN, 1);
      //midi_apcmini->sendNoteOn(counter, 1, 1);
    } else if (is_bpm_on_beat(ticks,duration)) {
      midi_apcmini->sendNoteOn(beat_counter, APCMINI_OFF, 1);
    }

    if (redraw_immediately || ticks - last_updated_display > PPQN) {
      apcmini_update_clock_display();
      redraw_immediately = false;
    }
  }
}

void apcmini_on_restart() {
  if (midi_apcmini) {
    midi_apcmini->sendStop();
    midi_apcmini->sendStart();
  }
}


void apcmini_clear_display() {
  Serial.println("Clearing APC display..");
  for (byte x = 0 ; x < APCMINI_NUM_ROWS ; x++) {
    for (byte y = 0 ; y < APCMINI_DISPLAY_WIDTH ;y++) {
      midi_apcmini->sendNoteOn(x+(y*8), APCMINI_OFF, 1);
    }
  }
}

void apcmini_update_clock_display() {
  // draw the clock divisions
  for (byte c = 0 ; c < NUM_CLOCKS ; c++) {
    //byte start_row = (8-NUM_CLOCKS) * 8;
    byte start_row = 64-((c+1)*APCMINI_DISPLAY_WIDTH);
    for (byte i = 0 ; i < APCMINI_DISPLAY_WIDTH ; i++) {
      byte io = (i - clock_delay[c]) % APCMINI_DISPLAY_WIDTH; 
      if (clock_multiplier[c]<=0.5) {
        midi_apcmini->sendNoteOn(start_row+i, APCMINI_RED, 1);
      } else if (clock_multiplier[c]<=1.0) {
        midi_apcmini->sendNoteOn(start_row+i, APCMINI_GREEN, 1);
      } else if (io%(byte)clock_multiplier[c]==0) {
        byte colour = clock_multiplier[c]   > 8.0 ? APCMINI_RED : 
                      (clock_multiplier[c]  > 4.0 ? APCMINI_YELLOW : APCMINI_GREEN);
        midi_apcmini->sendNoteOn(start_row+i, colour, 1);
      } else {
        midi_apcmini->sendNoteOn(start_row+i, APCMINI_OFF, 1);  // turn the led off
      }
    }
    if (c==clock_selected)
      midi_apcmini->sendNoteOn(APCMINI_BUTTON_CLIP_STOP + c, APCMINI_GREEN, 1);
    else
      midi_apcmini->sendNoteOn(APCMINI_BUTTON_CLIP_STOP + c, APCMINI_OFF, 1);
  }
  last_updated_display = ticks;
}


void apcmini_init() {
    midi_apcmini->turnThruOff();
    midi_apcmini->setHandleControlChange(apcmini_control_change);
    midi_apcmini->setHandleNoteOn(apcmini_note_on);
    midi_apcmini->setHandleNoteOff(apcmini_note_off);
    apcmini_clear_display();
}
