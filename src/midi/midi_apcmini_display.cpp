#include "Config.h"
#if defined(ENABLE_APCMINI) && defined(ENABLE_APCMINI_DISPLAY)

//#include "behaviours/behaviour_apcmini->device.h"
#include "midi/midi_apcmini_display.h"

#include "midi_helpers.h"

#include "Config.h"
#include "cv_outs.h"
#include "sequencer.h"
#include "storage.h"
#include "project.h"

#include "behaviours/behaviour_apcmini.h"

int8_t apc_note_last_sent[MIDI_MAX_NOTE+1] = {};

// cached send note on
void apcdisplay_sendNoteOn(int8_t pitch, int8_t value, int8_t channel, bool force) {
  if (!behaviour_apcmini->is_connected()) return;

  if (force || value != apc_note_last_sent[pitch]) {
    behaviour_apcmini->device->sendNoteOn(pitch, value, channel);

    //if (!force) 
    apc_note_last_sent[pitch] = value;
  }
}
//cached send note off
void apcdisplay_sendNoteOff(int8_t pitch, int8_t value, int8_t channel, bool force) {
  apcdisplay_sendNoteOn(pitch, 0 /*value*/, channel, force);
}
void apcdisplay_initialise_last_sent() {
  for(int i = 0 ; i < MIDI_MAX_NOTE ; i++)
    apc_note_last_sent[i] = -1;
}

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
    apcdisplay_sendNoteOn(START_BEAT_INDICATOR + beat_counter, APCMINI_GREEN);
  } else if (is_bpm_on_beat(ticks,duration)) {
    //Serial.printf("Off %i (ticks %i with duration %i)\n", beat_counter, ticks, duration);
    apcdisplay_sendNoteOn(START_BEAT_INDICATOR + beat_counter, APCMINI_OFF);
    //apcdisplay_sendNoteOff(START_BEAT_INDICATOR + beat_counter, APCMINI_OFF, 1);
      //behaviour_apcmini->device->send_now();
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

int get_sequencer_cell_apc_colour(byte row, byte column) {
  if (row < NUM_CLOCKS) {
    // clock
    int io = (column - current_state.clock_delay[row]) % APCMINI_DISPLAY_WIDTH;
    float cm = get_clock_multiplier(row);
    if (is_clock_off(row)) {
      return APCMINI_OFF;
    } else if (cm<=1.0 || io%(byte)cm==0) {
      return get_colour_for_clock_multiplier(cm);
    } else { // shouldn't reach this
      return APCMINI_OFF;
    }
  } else {
    // sequencer
    int v = read_sequence(row-NUM_CLOCKS, column);
    return v ? get_colour(v-1) : APCMINI_OFF;
  }
}
int get_sequencer_cell_565_colour(byte row, byte column) {
  int colour = get_sequencer_cell_apc_colour(row, column);
  return get_565_colour_for_apc_colour(colour);
}
int16_t get_565_colour_for_apc_colour(int colour) {
  switch(colour) {
    case APCMINI_GREEN: return GREEN;
    case APCMINI_RED:   return RED;
    case APCMINI_YELLOW:return YELLOW;
    case APCMINI_GREEN_BLINK: return millis()%500 > 250 ? GREEN : BLUE;
    case APCMINI_RED_BLINK:   return millis()%500 > 250 ? RED : BLUE;
    case APCMINI_YELLOW_BLINK:return millis()%500 > 250 ? YELLOW : BLUE;
    default:
      return colour>0 ? C_WHITE : BLACK;
  }
}

void redraw_clock_row(byte clock_number, bool force) {
    if (behaviour_apcmini->device==nullptr) return;

    int start_row = 64-((clock_number+1)*APCMINI_DISPLAY_WIDTH);
    for (unsigned int x = 0 ; x < APCMINI_DISPLAY_WIDTH ; x++) {
      apcdisplay_sendNoteOn(start_row+x, get_sequencer_cell_apc_colour(clock_number, x));
    }
}

void redraw_clock_selected(byte old_clock_selected, byte clock_selected, bool force) {
  if (behaviour_apcmini->device==nullptr) return;

  apcdisplay_sendNoteOn(APCMINI_BUTTON_CLIP_STOP + old_clock_selected, APCMINI_OFF, 1, force);
  apcdisplay_sendNoteOn(APCMINI_BUTTON_CLIP_STOP + clock_selected,     APCMINI_ON,  1, force);
}
#endif

#ifdef ENABLE_SEQUENCER
void redraw_sequence_row(byte sequence_number, bool force) {
  if (behaviour_apcmini->device==nullptr) return;

  int start_row = 32-((sequence_number+1)*APCMINI_DISPLAY_WIDTH);
  for (unsigned int i = 0 ; i < APCMINI_DISPLAY_WIDTH ; i++) {
    apcdisplay_sendNoteOn(start_row+i, get_sequencer_cell_apc_colour(NUM_CLOCKS+sequence_number,i));
  }
}
#endif

#ifdef ENABLE_APCMINI_DISPLAY
  void apcmini_clear_display() {
    if (behaviour_apcmini->device==nullptr) return;
    
    Serial_println(F("Clearing APC display..")); Serial_flush();
    for (int i = 0 ; i < 4 ; i++) {
      redraw_clock_row(i,true);
      redraw_sequence_row(i,true);
    }
    for (int y = 0 ; y < APCMINI_NUM_ROWS ; y++) {
      for (int x = 0 ; x < APCMINI_DISPLAY_WIDTH ; x++) {
          //apcdisplay_initialise_last_sent();
          apcdisplay_sendNoteOn (x+(y*APCMINI_DISPLAY_WIDTH), APCMINI_OFF, 1, true);
          //apcdisplay_sendNoteOff(x+(y*APCMINI_DISPLAY_WIDTH), APCMINI_OFF, 1, true);
      }
    }
    for (int x = START_BEAT_INDICATOR ; x < START_BEAT_INDICATOR + (BEATS_PER_BAR*2) ; x++) {
      apcdisplay_sendNoteOn(x, APCMINI_OFF, 1, true);
    }
    // clear the 'selected clock row' indicator lights
    apcdisplay_sendNoteOn(APCMINI_BUTTON_CLIP_STOP, APCMINI_OFF, 1, true);
    apcdisplay_sendNoteOn(APCMINI_BUTTON_SOLO, APCMINI_OFF, 1, true);
    apcdisplay_sendNoteOn(APCMINI_BUTTON_REC_ARM, APCMINI_OFF, 1, true);
    apcdisplay_sendNoteOn(APCMINI_BUTTON_MUTE, APCMINI_OFF, 1, true);
    //delay(1000);
    apcdisplay_initialise_last_sent();
    Serial_println(F("Leaving APC display")); Serial_flush();
    // 
  }

  void apcmini_update_clock_display() {
    //Serial.println(F("starting apcmini_update_clock_display().."));
    // draw the clock divisions
    #ifdef ENABLE_CLOCKS
      for (int c = 0 ; c < NUM_CLOCKS ; c++) {
        //byte start_row = (8-NUM_CLOCKS) * 8;
        redraw_clock_row(c);
      }
    #endif
    #ifdef ENABLE_SEQUENCER
      for (int c = 0 ; c < NUM_SEQUENCES ; c++) {
        redraw_sequence_row(c);
      }
    #endif
    behaviour_apcmini->last_updated_display = millis();
    //Serial.println(F("returning from apcmini_update_clock_display()"));
  }
#endif
#endif