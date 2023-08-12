#ifndef APCMINI_DISPLAY__INCLUDED
#define APCMINI_DISPLAY__INCLUDED

//#define ATOMIC(X) noInterrupts(); X; interrupts();
#define ATOMIC(X) X

#include <MIDI.h>
#include "Config.h"
#include "cv_outs.h"
#include "sequencer.h"
#include "storage.h"

// button colours from https://remotify.io/community/question/led-feedback-values
#define APCMINI_OFF           0
#define APCMINI_ON            1
#define APCMINI_GREEN         1
#define APCMINI_GREEN_BLINK   2
#define APCMINI_RED           3
#define APCMINI_RED_BLINK     4
#define APCMINI_YELLOW        5
#define APCMINI_YELLOW_BLINK  6

extern byte apc_note_last_sent[127];

// gradation of colours to use
#define LEVEL_1 0
#define LEVEL_2 1
#define LEVEL_3 2
const byte colour_intensity[] = {
  APCMINI_GREEN,
  APCMINI_YELLOW,
  APCMINI_RED,
  APCMINI_GREEN_BLINK,
  APCMINI_YELLOW_BLINK,
  APCMINI_RED_BLINK
};
inline byte get_colour(byte lev);


void apcmini_update_position_display(int ticks);


void apcdisplay_sendNoteOn(byte pitch, byte value, byte channel = 1, bool force = false);
void apcdisplay_sendNoteOff(byte pitch, byte value, byte channel = 1, bool force = false);
void apcdisplay_initialise_last_sent();

#ifdef ENABLE_CLOCKS
void redraw_clock_row(byte c);

void redraw_clock_selected(byte old_clock_selected, byte clock_selected);
#endif

#ifdef ENABLE_SEQUENCER
void redraw_sequence_row(byte c);
#endif


#ifdef ENABLE_APCMINI_DISPLAY
void apcmini_clear_display();
void apcmini_update_position_display(int ticks);
void apcmini_update_clock_display(); 
#endif


#endif
