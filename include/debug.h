#ifndef DEBUG__INCLUDED
#define DEBUG__INCLUDED

//#define ENABLE_DEBUG_SERIAL

#include <Arduino.h>

int freeRam();
void debug_free_ram();

void reset_teensy();

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
        #define Debug_printf(...)     {}
        #define Debug_print(X)      {}
    #endif
#endif

#include "LinkedList.h"
#define MAX_MESSAGES_LOG 20
extern LinkedList<String> *messages_log;
void messages_log_add(String msg);

#endif
