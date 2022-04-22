#ifndef BEATSTEP__INCLUDED
#define BEATSTEP__INCLUDED
#ifdef ENABLE_BEATSTEP

//#define ATOMIC(X) noInterrupts(); X; interrupts();
  #define ATOMIC(X) X

#include "USBHost_t36.h"
#include <Arduino.h>
#include "bpm.h"

extern MIDIDevice_BigBuffer *midi_beatstep;
extern volatile uint8_t ixBeatStep; //= 0xff;

extern volatile bool beatstep_started; //= false;

void beatstep_loop(unsigned long ticks);
void beatstep_control_change (byte inChannel, byte inNumber, byte inValue);
void beatstep_handle_start();
void beatstep_on_tick(volatile uint32_t ticks);
void beatstep_on_restart(); 
void beatstep_init();
#endif

#endif