#ifndef MPK49__INCLUDED
#define MPK49__INCLUDED

#ifdef ENABLE_MPK49

#include "USBHost_t36.h"

#include "bpm.h"
#include "Config.h"
#include "ConfigMidi.h"
#include "midi_outs.h"

extern MIDIDevice_BigBuffer *midi_MPK49;  
extern uint8_t ixMPK49; //  = 0xff;

extern bool MPK49_started; // = false;
extern bool mpk49_recording;
extern bool mpk49_playing;

void MPK49_loop(unsigned long ticks);
// called inside interrupt
void MPK49_on_tick(uint32_t ticks);

// called inside interrupt
void MPK49_on_restart();

void mpk49_handle_note_on(byte channel, byte note, byte velocity);
void mpk49_handle_note_off(byte channel, byte note, byte velocity);

void MPK49_init();

#ifdef ENABLE_SCREEN
  void mpk49_display_looper_status(ST7789_t3 *tft);
#endif

#endif
#endif