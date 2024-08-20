#include "Config.h"
#if defined(ENABLE_APCMINI) && defined(ENABLE_APCMINI_DISPLAY)

// enable workaround to prevent some LED updates being somehow missed
// this only started happening recently (i believe in the month before 2024-12-01)
// and i hope that it can be disabled at some point in the future!
#define ENABLE_APCMINI_PARTIAL_UPDATE_WORKAROUND

//#include "behaviours/behaviour_apcmini->device.h"
#include "midi/midi_apcmini_display.h"

#include "midi_helpers.h"

#include "Config.h"
#include "cv_outs.h"
#include "sequencer.h"
#include "storage.h"
#include "project.h"

#include "behaviours/behaviour_apcmini.h"

apc_gate_page_t apc_gate_page = CLOCKS;

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
  } else if (is_bpm_on_beat(ticks,behaviour_clock_gates->duration)) {
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
    float cm = behaviour_clock_gates->get_clock_multiplier(row);
    if (behaviour_clock_gates->is_clock_off(row)) {
      return APCMINI_OFF;
    } else if (cm<=1.0 || io%(byte)cm==0) {
      return get_colour_for_clock_multiplier(cm);
    } else { // shouldn't reach this
      return APCMINI_OFF;
    }
  } else {
    // sequencer
    int v =behaviour_sequencer_gates-> read_sequence(row-NUM_CLOCKS, column);
    return v ? get_colour(v-1) : APCMINI_OFF;
  }
}
int get_sequencer_cell_565_colour(byte row, byte column) {
  int colour = get_sequencer_cell_apc_colour(row, column);
  return get_565_colour_for_apc_note(colour);
}
int16_t get_565_colour_for_apc_note(int colour) {
  switch(colour) {
    case APCMINI_GREEN: return GREEN;
    case APCMINI_RED:   return RED;
    case APCMINI_YELLOW:return YELLOW;
    case APCMINI_GREEN_BLINK: return millis()%500 > 250 ? GREEN  : DARK_GREEN;
    case APCMINI_RED_BLINK:   return millis()%500 > 250 ? RED    : DARK_RED;
    case APCMINI_YELLOW_BLINK:return millis()%500 > 250 ? YELLOW : DARK_YELLOW;
    default:
      return colour>0 ? C_WHITE : BLACK;
  }
}

void redraw_clock_row(byte clock_number, bool force) {
    if (behaviour_apcmini->device==nullptr) return;

    int start_row = (NUM_CLOCKS*APCMINI_DISPLAY_WIDTH)-((clock_number+1)*APCMINI_DISPLAY_WIDTH);
    for (unsigned int x = 0 ; x < APCMINI_DISPLAY_WIDTH ; x++) {
      //apcdisplay_sendNoteOn(start_row+x, APCMINI_OFF);
      apcdisplay_sendNoteOn(start_row+x, get_sequencer_cell_apc_colour(clock_number, x));
    }
    // draw the selector
    
    if (clock_number==behaviour_apcmini->clock_selected) {
      Serial.printf("redraw_clock_row(%i) while clock_selected=%i\n", clock_number,behaviour_apcmini->clock_selected);
      apcdisplay_sendNoteOn(APCMINI_BUTTON_CLIP_STOP+clock_number, APCMINI_GREEN);
    } else {
      Serial.printf("redraw_clock_row(%i) while clock_selected=%i\n", clock_number,behaviour_apcmini->clock_selected);
      apcdisplay_sendNoteOn(APCMINI_BUTTON_CLIP_STOP+clock_number, APCMINI_OFF);
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

  int start_row = (NUM_SEQUENCES*APCMINI_DISPLAY_WIDTH)-((sequence_number+1)*APCMINI_DISPLAY_WIDTH);
  for (unsigned int x = 0 ; x < APCMINI_DISPLAY_WIDTH ; x++) {
    //apcdisplay_sendNoteOn(start_row+x, APCMINI_OFF);
    apcdisplay_sendNoteOn(start_row+x, get_sequencer_cell_apc_colour(NUM_CLOCKS+sequence_number,x));
  }
}
#endif

#ifdef ENABLE_APCMINI_DISPLAY
  void apcmini_clear_display() {
    if (behaviour_apcmini->device==nullptr) return;
    
    Serial_println(F("Clearing APC display..")); Serial_flush();

    /*for (int y = 0 ; y < APCMINI_NUM_ROWS ; y++) {
      for (int x = 0 ; x < APCMINI_DISPLAY_WIDTH ; x++) {
          //apcdisplay_initialise_last_sent();
          apcdisplay_sendNoteOn (x+(y*APCMINI_DISPLAY_WIDTH), APCMINI_OFF, 1, true);
          //apcdisplay_sendNoteOff(x+(y*APCMINI_DISPLAY_WIDTH), APCMINI_OFF, 1, true);
      }
    }*/

    if (get_apc_gate_page()==CLOCKS)
      for (int i = 0 ; i < NUM_CLOCKS ; i++)
        redraw_clock_row(i,true);

    if (get_apc_gate_page()==SEQUENCES) 
      for (int i = 0 ; i < NUM_SEQUENCES ; i++)
        redraw_sequence_row(i,true);

    // clear the beat position indicator
    for (int x = START_BEAT_INDICATOR ; x < START_BEAT_INDICATOR + (BEATS_PER_BAR*2) ; x++) {
      apcdisplay_sendNoteOn(x, APCMINI_OFF, 1, true);
    }
    // clear the 'selected clock row' indicator lights
    // todo: can replace this with a loop around 0 -> APCMINI_BUTTON_CLIP_STOP+NUM_CLOCKS
    apcdisplay_sendNoteOn(APCMINI_BUTTON_CLIP_STOP, APCMINI_OFF, 1, true);
    apcdisplay_sendNoteOn(APCMINI_BUTTON_SOLO, APCMINI_OFF, 1, true);
    apcdisplay_sendNoteOn(APCMINI_BUTTON_REC_ARM, APCMINI_OFF, 1, true);
    apcdisplay_sendNoteOn(APCMINI_BUTTON_MUTE, APCMINI_OFF, 1, true);
    apcdisplay_sendNoteOn(APCMINI_BUTTON_SELECT, APCMINI_OFF, 1, true);
    apcdisplay_sendNoteOn(APCMINI_BUTTON_UNLABELED_1, APCMINI_OFF, 1, true);
    apcdisplay_sendNoteOn(APCMINI_BUTTON_UNLABELED_2, APCMINI_OFF, 1, true);
    apcdisplay_sendNoteOn(APCMINI_BUTTON_STOP_ALL_CLIPS, APCMINI_OFF, 1, true);

    //delay(1000);
    apcdisplay_initialise_last_sent();
    Serial_println(F("Leaving APC display")); Serial_flush();
    // 
  }

  // update the APCMini display
  void apcmini_update_clock_display() {
    //Serial.println(F("starting apcmini_update_clock_display().."));

    #ifdef ENABLE_APCMINI_PARTIAL_UPDATE_WORKAROUND
      // don't redraw all of the rows in the same call, to avoid weird problem with some messages apparently not being received
      // TODO: make this support more than 4 clocks/sequencers (ie not rely on NUM_CLOCKS like it does)
      static int row_to_draw = 0;
      if (get_apc_gate_page()==CLOCKS)         redraw_clock_row(row_to_draw);
      else if (get_apc_gate_page()==SEQUENCES) redraw_sequence_row(row_to_draw);
      row_to_draw++;
      if (row_to_draw >= NUM_CLOCKS) {
        row_to_draw = 0;
        behaviour_apcmini->last_updated_display = millis();
      }
    #else
      #ifdef ENABLE_CLOCKS
        if (get_apc_gate_page()==CLOCKS) {
          for (int c = 0 ; c < NUM_CLOCKS ; c++) {
            //byte start_row = (8-NUM_CLOCKS) * 8;
            redraw_clock_row(c);
          }
        }
      #endif
      #ifdef ENABLE_SEQUENCER
        if (get_apc_gate_page()==SEQUENCES) {
          for (int c = 0 ; c < NUM_SEQUENCES ; c++) {
            redraw_sequence_row(c);
          }
        }
      #endif
      behaviour_apcmini->last_updated_display = millis();
      //Serial.println(F("returning from apcmini_update_clock_display()"));
    #endif
  }
#endif

void set_apc_gate_page(apc_gate_page_t page) {
  Serial.printf("Switched to page %i\n", page);
  apc_gate_page = page;
}
apc_gate_page_t get_apc_gate_page() {
  return apc_gate_page;
}

#endif