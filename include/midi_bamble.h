#ifndef BAMBLE__INCLUDED
#define BAMBLE__INCLUDED

#include "Config.h"

#ifdef ENABLE_BAMBLE

//#include <MIDI.h>
#include <USBHost_t36.h>

//#include "bpm.h"
//#include "Config.h"
//#include "ConfigMidi.h"
//#include "midi_outs.h"

extern MIDIDevice *midi_bamble;  
extern uint8_t ixBamble;

extern bool bamble_started;

void bamble_loop(unsigned long ticks);
void bamble_on_tick(uint32_t ticks);
void bamble_on_restart();
void bamble_init();

#endif


#endif