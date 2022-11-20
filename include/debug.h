#ifndef DEBUG__INCLUDED
#define DEBUG__INCLUDED

int freeRam();
void debug_free_ram();

void reset_teensy();

#ifndef Serial_flush
    #ifdef SERIAL_FLUSH_REALLY
        #define Serial_flush() Serial.flush()
    #else
        #define Serial_flush() {}
    #endif
#endif

#endif
