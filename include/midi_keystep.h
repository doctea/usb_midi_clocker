#ifndef keystep__INCLUDED
#define keystep__INCLUDED

#ifdef ENABLE_KEYSTEP

#include "bpm.h"
#include "Config.h"
#include "ConfigMidi.h"
#include "midi_outs.h"

extern MIDIDevice *midi_keystep;  
extern uint8_t ixKeystep;

extern bool keystep_started;

void keystep_loop();
// called inside interrupt
void keystep_on_tick(uint32_t ticks) ;
// called inside interrupt
void keystep_on_restart();

void keystep_handle_note_on(byte channel, byte note, byte velocity);
void keystep_handle_note_off(byte channel, byte note, byte velocity);

void keystep_init();
#endif


#endif