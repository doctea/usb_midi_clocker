#ifndef APCMINI_DISPLAY__INCLUDED
#define APCMINI_DISPLAY__INCLUDED
#ifdef ENABLE_APCMINI_DISPLAY

#include "cv_outs.h"
#include "sequencer.h"

// button colours from https://remotify.io/community/question/led-feedback-values
#define APCMINI_OFF           (uint8_t)0
#define APCMINI_ON            1
#define APCMINI_GREEN         1
#define APCMINI_GREEN_BLINK   2
#define APCMINI_RED           3
#define APCMINI_RED_BLINK     4
#define APCMINI_YELLOW        5
#define APCMINI_YELLOW_BLINK  6


// gradation of colours to use
#define LEVEL_1 0
#define LEVEL_2 1
#define LEVEL_3 2
const byte colour_intensity[] = {
  APCMINI_GREEN,
  APCMINI_YELLOW,
  APCMINI_RED,
};
inline byte get_colour(byte lev) {
  if (lev>=sizeof(colour_intensity))
    lev = sizeof(colour_intensity)-1;
  return colour_intensity[lev];
}


void apcmini_update_position_display(int ticks) {
  byte beat_counter = (byte)((ticks/PPQN) % APCMINI_DISPLAY_WIDTH);
  if (is_bpm_on_beat(ticks)) {
    //#define DEBUG_TICKS yes
    #ifdef DEBUG_TICKS
        Serial.print(F("apcmini w/"));
        /*Serial.print(ticks);
        Serial.print(F("\tCounter is "));*/
        Serial.print(beat_counter);
        Serial.print(F(" "));
    #endif
    //Serial.printf("On %i\n", beat_counter);
    ATOMIC(
      midi_apcmini->sendNoteOn(START_BEAT_INDICATOR + beat_counter, APCMINI_GREEN, 1);
    );
  } else if (is_bpm_on_beat(ticks,duration)) {
    //Serial.printf("Off %i (ticks %i with duration %i)\n", beat_counter, ticks, duration);
    ATOMIC(
      midi_apcmini->sendNoteOn(START_BEAT_INDICATOR + beat_counter, APCMINI_OFF, 1);
      midi_apcmini->sendNoteOff(START_BEAT_INDICATOR + beat_counter, APCMINI_OFF, 1);
      midi_apcmini->send_now();
    )
  }
}


#ifdef ENABLE_CLOCKS
void redraw_clock_row(byte c) {
    byte start_row = 64-((c+1)*APCMINI_DISPLAY_WIDTH);
    for (byte i = 0 ; i < APCMINI_DISPLAY_WIDTH ; i++) {
      byte io = (i - current_state.clock_delay[c]) % APCMINI_DISPLAY_WIDTH;
      float cm = get_clock_multiplier(c);
      if (cm<=0.25) {
        ATOMIC(midi_apcmini->sendNoteOn(start_row+i, get_colour(LEVEL_3), 1));
      } else if (cm<=0.5) {
        ATOMIC(midi_apcmini->sendNoteOn(start_row+i, get_colour(LEVEL_2), 1));
      } else if (cm<=1.0) {
        ATOMIC(midi_apcmini->sendNoteOn(start_row+i, get_colour(LEVEL_1), 1));
      } else if (!is_clock_off(c) && io%(byte)cm==0) {
        byte colour =  cm  > 8.0 ? get_colour(LEVEL_2) : 
                      (cm  > 4.0 ? get_colour(LEVEL_1) : get_colour(LEVEL_1));
        ATOMIC(midi_apcmini->sendNoteOn(start_row+i, colour, 1));
      } else {
        ATOMIC(midi_apcmini->sendNoteOn(start_row+i, APCMINI_OFF, 1));  // turn the led off
      }
    }
}

void redraw_clock_selected(byte old_clock_selected, byte clock_selected) {
  ATOMIC(midi_apcmini->sendNoteOn(APCMINI_BUTTON_CLIP_STOP + old_clock_selected, APCMINI_OFF, 1);)
  ATOMIC(midi_apcmini->sendNoteOn(APCMINI_BUTTON_CLIP_STOP + clock_selected,     APCMINI_ON,  1);)
}
#endif

#ifdef ENABLE_SEQUENCER
void redraw_sequence_row(byte c) {
    byte start_row = 32-((c+1)*APCMINI_DISPLAY_WIDTH);
    for (byte i = 0 ; i < APCMINI_DISPLAY_WIDTH ; i++) {
      byte v = read_sequence(c,i);
      if (v) { //should_trigger_sequence(i*PPQN,c)) {
        ATOMIC(midi_apcmini->sendNoteOn(start_row+i, get_colour(v-1)/*(2*(v-1)) + APCMINI_ON*/, 1);)
      } else {
        ATOMIC(midi_apcmini->sendNoteOn(start_row+i, APCMINI_OFF, 1);)
      }
    }
}
#endif


#ifdef ENABLE_APCMINI_DISPLAY
void apcmini_clear_display() {
  Serial.println(F("Clearing APC display.."));
  for (uint8_t x = 0 ; x < APCMINI_NUM_ROWS ; x++) {
    for (uint8_t y = 0 ; y < APCMINI_DISPLAY_WIDTH ; y++) {
      ATOMIC(
        midi_apcmini->sendNoteOn(x+(y*APCMINI_DISPLAY_WIDTH), APCMINI_OFF, 1);
      )
    }
  }
  for (byte x = START_BEAT_INDICATOR ; x < START_BEAT_INDICATOR + (BEATS_PER_BAR*2) ; x++) {
    ATOMIC(
      midi_apcmini->sendNoteOn(x, APCMINI_OFF, 1);
    )
  }
  delay(1000);
  Serial.println("Leaving APC display");
}

void apcmini_update_clock_display() {
  //Serial.println(F("starting apcmini_update_clock_display().."));
  // draw the clock divisions
#ifdef ENABLE_CLOCKS
  for (byte c = 0 ; c < NUM_CLOCKS ; c++) {
    //byte start_row = (8-NUM_CLOCKS) * 8;
    redraw_clock_row(c);
  }
#endif
#ifdef ENABLE_SEQUENCER
  for (byte c = 0 ; c < NUM_SEQUENCES ; c++) {
    redraw_sequence_row(c);
  }
#endif
  ATOMIC(
    last_updated_display = millis();
  )
  //Serial.println(F("returning from apcmini_update_clock_display()"));
}
#endif


#endif

#endif