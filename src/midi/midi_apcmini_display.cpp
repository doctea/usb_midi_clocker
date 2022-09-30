#include "Config.h"
//#include "behaviours/behaviour_apcmini->device.h"
#include "midi/midi_apcmini_display.h"

//#define ATOMIC(X) noInterrupts(); X; interrupts();
#define ATOMIC(X) X

#include "Config.h"
#include "cv_outs.h"
#include "sequencer.h"
#include "storage.h"
#include "project.h"

#include "behaviours/behaviour_apcmini.h"

/*const byte colour_intensity[] = {
  APCMINI_GREEN,
  APCMINI_YELLOW,
  APCMINI_RED,
};*/
byte get_colour(byte lev) {
  if (lev>=sizeof(colour_intensity))
    lev = sizeof(colour_intensity)-1;
  return colour_intensity[lev];
}

void apcmini_update_position_display(int ticks) {
  if (behaviour_apcmini==nullptr) return;
  //Serial.println("apcmini_update_position_display behaviour_apcmini isn't nullptr");

  if (behaviour_apcmini->device==nullptr) return;

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
      behaviour_apcmini->device->sendNoteOn(START_BEAT_INDICATOR + beat_counter, APCMINI_GREEN, 1);
    );
  } else if (is_bpm_on_beat(ticks,duration)) {
    //Serial.printf("Off %i (ticks %i with duration %i)\n", beat_counter, ticks, duration);
    ATOMIC(
      behaviour_apcmini->device->sendNoteOn(START_BEAT_INDICATOR + beat_counter, APCMINI_OFF, 1);
      behaviour_apcmini->device->sendNoteOff(START_BEAT_INDICATOR + beat_counter, APCMINI_OFF, 1);
      //behaviour_apcmini->device->send_now();
    )
  }
}


#ifdef ENABLE_CLOCKS

byte get_colour_for_clock_multiplier(float cm) {
  if (cm<=0.25) {
    return get_colour(LEVEL_3);
  } else if (cm<=0.5) {
    return get_colour(LEVEL_2);
  } else if (cm<=1.0) {
    return get_colour(LEVEL_1);
  } else if (cm>16.0) {
    return get_colour(LEVEL_3);
  } else if (cm>8.0) {
    return get_colour(LEVEL_2);
  } else if (cm>4.0) {
    return get_colour(LEVEL_1);
  }
  return get_colour(LEVEL_1);
}

void redraw_clock_row(byte c) {
    if (behaviour_apcmini->device==nullptr) return;

    byte start_row = 64-((c+1)*APCMINI_DISPLAY_WIDTH);
    for (byte i = 0 ; i < APCMINI_DISPLAY_WIDTH ; i++) {
      byte io = (i - current_state.clock_delay[c]) % APCMINI_DISPLAY_WIDTH;
      float cm = get_clock_multiplier(c);
      if (is_clock_off(c)) {
        behaviour_apcmini->device->sendNoteOn(start_row+i, APCMINI_OFF, 1);
      } else if (cm<=1.0) {
        behaviour_apcmini->device->sendNoteOn(start_row+i, get_colour_for_clock_multiplier(cm), 1);
      } else if (io%(byte)cm==0) {
        behaviour_apcmini->device->sendNoteOn(start_row+i, get_colour_for_clock_multiplier(cm), 1);
      } else { // shouldn't reach this
        //Serial.printf("WARNING: reached unexpected branch for clock %i: cm is %0.2f\n", c, cm);
        behaviour_apcmini->device->sendNoteOn(start_row+i, APCMINI_OFF, 1);  // turn the led off
      }
    }
}

void redraw_clock_selected(byte old_clock_selected, byte clock_selected) {
  if (behaviour_apcmini->device==nullptr) return;

  ATOMIC(behaviour_apcmini->device->sendNoteOn(APCMINI_BUTTON_CLIP_STOP + old_clock_selected, APCMINI_OFF, 1);)
  ATOMIC(behaviour_apcmini->device->sendNoteOn(APCMINI_BUTTON_CLIP_STOP + clock_selected,     APCMINI_ON,  1);)
}
#endif

#ifdef ENABLE_SEQUENCER
void redraw_sequence_row(byte c) {
  if (behaviour_apcmini->device==nullptr) return;

  byte start_row = 32-((c+1)*APCMINI_DISPLAY_WIDTH);
  for (byte i = 0 ; i < APCMINI_DISPLAY_WIDTH ; i++) {
    byte v = read_sequence(c,i);
    if (v) { //should_trigger_sequence(i*PPQN,c)) {
      ATOMIC(behaviour_apcmini->device->sendNoteOn(start_row+i, get_colour(v-1)/*(2*(v-1)) + APCMINI_ON*/, 1);)
    } else {
      ATOMIC(behaviour_apcmini->device->sendNoteOn(start_row+i, APCMINI_OFF, 1);)
    }
  }
}
#endif


#ifdef ENABLE_APCMINI_DISPLAY
  void apcmini_clear_display() {
    if (behaviour_apcmini->device==nullptr) return;
    
    Serial.println(F("Clearing APC display.."));
    for (uint8_t x = 0 ; x < APCMINI_NUM_ROWS ; x++) {
      for (uint8_t y = 0 ; y < APCMINI_DISPLAY_WIDTH ; y++) {
        ATOMIC(
          behaviour_apcmini->device->sendNoteOn(x+(y*APCMINI_DISPLAY_WIDTH), APCMINI_OFF, 1);
        )
      }
    }
    for (byte x = START_BEAT_INDICATOR ; x < START_BEAT_INDICATOR + (BEATS_PER_BAR*2) ; x++) {
      ATOMIC(
        behaviour_apcmini->device->sendNoteOn(x, APCMINI_OFF, 1);
      )
    }
    //delay(1000);
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
      behaviour_apcmini->last_updated_display = millis();
    )
    //Serial.println(F("returning from apcmini_update_clock_display()"));
  }
#endif
