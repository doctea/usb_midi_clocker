#ifdef ENABLE_APCMINI_DISPLAY


// button colours from https://remotify.io/community/question/led-feedback-values
#define APCMINI_OFF           0
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

#ifdef ENABLE_CLOCKS
void redraw_clock_row(byte c) {
    byte start_row = 64-((c+1)*APCMINI_DISPLAY_WIDTH);
    for (byte i = 0 ; i < APCMINI_DISPLAY_WIDTH ; i++) {
      byte io = (i - current_state.clock_delay[c]) % APCMINI_DISPLAY_WIDTH; //) % APCMINI_DISPLAY_WIDTH;   // TODO: this doesn't display properly?
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

void apcmini_update_position_display(int ticks) {
  //ATOMIC(
    if (is_bpm_on_beat(ticks)) {
  #ifdef DEBUG_TICKS
      Serial.print(F("apcmini w/"));
      /*Serial.print(ticks);
      Serial.print(F("\tCounter is "));*/
      Serial.print(beat_counter);
      Serial.print(F(" "));
  #endif
      beat_counter = (byte)((ticks/PPQN) % APCMINI_DISPLAY_WIDTH);
      ATOMIC(
        midi_apcmini->sendNoteOn(START_BEAT_INDICATOR + beat_counter, APCMINI_GREEN, 1);
      );
    } else if (is_bpm_on_beat(ticks,duration)) {
      ATOMIC(
        midi_apcmini->sendNoteOn(START_BEAT_INDICATOR + beat_counter, APCMINI_OFF, 1);
      )
    }
  //)
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

#endif
