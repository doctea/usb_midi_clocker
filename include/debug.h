#pragma once

//#define ENABLE_DEBUG_SERIAL
#define SERIAL_FLUSH_REALLY

#include <Arduino.h>

extern bool debug_flag;

int freeRam();
void debug_free_ram();

#ifdef ARDUINO_TEENSY41
    int freeExtRam();
#endif

void reset_teensy();

// need these because we get stuck in a race condition if printing to Serial when Serial isn't connected?
#define Serial_println(X)   if(Serial)Serial.println(X)
#define Serial_printf(...)  if(Serial)Serial.printf(__VA_ARGS__)
#define Serial_print(X)     if(Serial)Serial.print(X)

#ifndef Serial_flush
    #ifdef SERIAL_FLUSH_REALLY
        #define Serial_flush() if(Serial)Serial.flush()
    #else
        #define Serial_flush() {}
    #endif
#endif
#ifndef Debug_printf
    #ifdef ENABLE_DEBUG_SERIAL
        #define Debug_println(X)    if(Serial)Serial.println(X)
        #define Debug_printf(...)   if(Serial)Serial.printf(__VA_ARGS__)
        #define Debug_print(X)      if(Serial)Serial.print(X)
    #else
        #define Debug_println(X)    {}
        #define Debug_printf(...)   {}
        #define Debug_print(X)      {}
    #endif
#endif

#include "menu_messages.h"

