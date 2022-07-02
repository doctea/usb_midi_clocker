#ifndef SUBCLOCKER__INCLUDED
#define SUBCLOCKER__INCLUDED
#ifdef ENABLE_SUBCLOCKER

#define ATOMIC(X) X

#include "USBHost_t36.h"
#include <Arduino.h>
#include "bpm.h"


extern int subclocker_divisor;
extern int subclocker_delay_ticks;

extern MIDIDevice_BigBuffer *midi_subclocker;
extern volatile uint8_t ixSubclocker; //= 0xff;
extern volatile bool subclocker_started; //= false;

extern int current_subclocker_note;
extern int last_subclocker_note;

/*extern MIDIOutputWrapper *subclocker_output;
void subclocker_setOutputWrapper(MIDIOutputWrapper *);*/

void subclocker_loop(unsigned long ticks);
//void subclocker_control_change (byte inChannel, byte inNumber, byte inValue);
void subclocker_handle_start();
void subclocker_on_tick(volatile uint32_t ticks);
void subclocker_on_restart(); 
void subclocker_init();

void subclocker_send_clock(unsigned long ticks);

/*#ifdef ENABLE_SCREEN
  #include "tft.h"
  void Subclocker_display_key_status(ST7789_t3 *tft);
#endif*/

#endif

#endif