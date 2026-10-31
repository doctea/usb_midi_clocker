#include <Arduino.h>

#include "debug.h"
#if defined(ARDUINO_TEENSY41)
  #include <smalloc.h>
#endif

#if defined(__arm__) && defined(CORE_TEENSY)
  extern unsigned long _heap_start;
  extern unsigned long _heap_end;
  extern char *__brkval;

  int freeRam() {
    return (char *)&_heap_end - __brkval;
  }
  #ifdef ARDUINO_TEENSY41
    int freeExtRam() {
      static uint32_t last_sample_ms = 0;
      static int cached_free_bytes = 0;
      static bool has_sample = false;
      const uint32_t now_ms = millis();
      if (!has_sample || now_ms - last_sample_ms >= 1000) {
        size_t total_bytes = 0;
        size_t free_bytes = 0;
        if (sm_malloc_stats_pool(&extmem_smalloc_pool, &total_bytes, nullptr,
                                 &free_bytes, nullptr) < 0) {
          cached_free_bytes = 0;
        } else {
          cached_free_bytes = static_cast<int>(free_bytes);
        }
        last_sample_ms = now_ms;
        has_sample = true;
      }
      return cached_free_bytes;
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