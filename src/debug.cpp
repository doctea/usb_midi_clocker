#include <Arduino.h>

#include "debug.h"

#if defined(__arm__) && defined(CORE_TEENSY)
  extern unsigned long _heap_start;
  extern unsigned long _heap_end;
  extern char *__brkval;

  int freeRam() {
    return (char *)&_heap_end - __brkval;
  }
  #ifdef ARDUINO_TEENSY41
    // cribbed from https://forum.pjrc.com/index.php?threads/how-to-display-free-ram.33443/post-312421
    // but it doesn't seem to work? always reports all the RAM as free
    extern char _extram_start[], _extram_end[];
    extern uint8_t external_psram_size;
    int freeExtRam() {
      auto psram = _extram_start + (external_psram_size << 20) - _extram_end;
      return psram;
    }
  #endif

  void debug_free_ram() {
    Serial_printf(F("debug_free_ram: %i RAM2"), freeRam());
    #ifdef ARDUINO_TEENSY41
      Serial_printf(F(", %i EXTRAM"), freeExtRam());
    #endif
    Serial_println();
  }

  FLASHMEM void reset_teensy() {
    // https://forum.pjrc.com/threads/57810-Soft-reboot-on-Teensy4-0
    #define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
    #define CPU_RESTART_VAL 0x5FA0004
    #define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);
    Serial.println(F("Restarting!\n")); Serial_flush();
    CPU_RESTART;
    //Serial.println(F("Restarted?!"); Serial_flush();
  }

#else
  int freeRam () {  
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
  }
  void debug_free_ram() {
    Serial_print(F("Free RAM2 is "));
    Serial_print(freeRam());
    #ifdef ARDUINO_TEENSY41
      Serial_print(F(", free EXTRAM is "));
      Serial_print(freeExtRam());
    #endif
    Serial_println();
  }
#endif