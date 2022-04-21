#ifndef APCMINI__INCLUDED
#define APCMINI__INCLUDED
//#ifdef ENABLE_APCMINI

#include "bpm.h"
#include "sequencer.h"
#include "multi_usb_handlers.h"

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
#define APCMINI_BUTTON_VOLUME    68
#define APCMINI_BUTTON_PAN       69
#define APCMINI_BUTTON_SEND      70
#define APCMINI_BUTTON_DEVICE    71

//#define START_BEAT_INDICATOR 0
#define START_BEAT_INDICATOR  APCMINI_BUTTON_UP

// restart buttons mapping - these are shifted
#define BUTTON_RESTART_IMMEDIATELY    APCMINI_BUTTON_UP
#define BUTTON_RESTART_AT_END_OF_BAR  APCMINI_BUTTON_DEVICE

//MIDI_NAMESPACE::MidiInterface<UHS2MIDI_NAMESPACE::uhs2MidiTransport> *midi_apcmini;
extern MIDIDevice_BigBuffer *midi_apcmini;
extern volatile uint8_t ixAPCmini;  //= 0xff;

extern volatile bool apcmini_started;// = false;
extern bool apcmini_shift_held;// = false;

#ifdef ENABLE_CLOCKS
extern byte clock_selected;// = 0;
#endif

extern bool redraw_immediately;// = false;
extern unsigned long last_updated_display;// = 0;

#ifdef ENABLE_APCMINI_DISPLAY
void apcmini_update_clock_display();
void apcmini_update_position_display(int ticks);
void apcmini_clear_display();
#endif

void apcmini_loop(unsigned long ticks); 

#ifdef ENABLE_APCMINI_DISPLAY
#include "midi_apcmini_display.h"
#endif


// called from loop, already inside ATOMIC, so don't use ATOMIC here
void apcmini_note_on(byte inChannel, byte inNumber, byte inVelocity);
// called from loop, already inside ATOMIC, so don't use ATOMIC here
void apcmini_note_off(byte inChannel, byte inNumber, byte inVelocity);
// called from loop, already inside ATOMIC, so don't use ATOMIC here
void apcmini_control_change (byte inChannel, byte inNumber, byte inValue);
// called inside interrupt
void apcmini_on_tick(volatile uint32_t ticks);
// called inside interrupt
void apcmini_on_restart();
void apcmini_init();
//#endif

#endif