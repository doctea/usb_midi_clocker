#include <Arduino.h>

#include "debug.h"

LinkedList<String> *messages_log = new LinkedList<String>();

void messages_log_add(String msg) {
  messages_log->add(msg);
  if (messages_log->size() >= MAX_MESSAGES_LOG) {
    messages_log->unlink(0);
  }
}

#if defined(__arm__) && defined(CORE_TEENSY)
  extern unsigned long _heap_start;
  extern unsigned long _heap_end;
  extern char *__brkval;

  int freeRam() {
    return (char *)&_heap_end - __brkval;
  }

  void debug_free_ram() {
    Serial.printf(F("debug_free_ram: %i\n"), freeRam());
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
    Serial.print(F("Free RAM is "));
    Serial.println(freeRam());
  }
#endif