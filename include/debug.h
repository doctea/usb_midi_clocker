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
#ifndef Debug_printf
    #ifdef ENABLE_DEBUG_SERIAL
        #define Debug_println(X)    Serial.println(X)
        #define Debug_printf(...)     Serial.printf(__VA_ARGS__)
        #define Debug_print(X)      Serial.print(X)
    #else
        #define Debug_println(X)    {}
        #define Debug_printf(...)     {}
        #define Debug_print(X)      {}
    #endif
#endif

#endif
