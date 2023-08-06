#ifndef APCMINI__INCLUDED
#define APCMINI__INCLUDED
//#ifdef ENABLE_APCMINI

#include "bpm.h"
#include "sequencer.h"
//#include "multi_usb_handlers.h"
//#include <MIDI.h>
//#include "USBHost_t36.h"

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

#define APCMINI_FADER_CC_1      48
#define APCMINI_FADER_CC_2      49
#define APCMINI_FADER_CC_3      50
#define APCMINI_FADER_CC_4      51
#define APCMINI_FADER_CC_5      52
#define APCMINI_FADER_CC_6      53
#define APCMINI_FADER_CC_7      54
#define APCMINI_FADER_CC_8      55
#define APCMINI_FADER_CC_MASTER 56

//#define START_BEAT_INDICATOR 0
#define START_BEAT_INDICATOR  APCMINI_BUTTON_UP

// restart buttons mapping - these are shifted
#define BUTTON_RESTART_IMMEDIATELY    APCMINI_BUTTON_UP
#define BUTTON_RESTART_AT_END_OF_BAR  APCMINI_BUTTON_DEVICE

#ifdef ENABLE_APCMINI_DISPLAY
    void apcmini_update_clock_display();
    void apcmini_update_position_display(int ticks);
    void apcmini_clear_display();
#endif

void apcmini_loop(unsigned long ticks); 

#ifdef ENABLE_APCMINI_DISPLAY
    #include "midi/midi_apcmini_display.h"
#endif

void apcmini_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void apcmini_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void apcmini_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);

#endif