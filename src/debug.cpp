#include <Arduino.h>

#if defined(__arm__) && defined(CORE_TEENSY)
  extern unsigned long _heap_start;
  extern unsigned long _heap_end;
  extern char *__brkval;

  int freeRam() {
    return (char *)&_heap_end - __brkval;
  }

  void debug_free_ram() {
    //Serial.println(F("debug_free_ram() not implemented on Teensy"));
    Serial.printf("debug_free_ram: %i\n", freeRam());
  }
#else
  int freeRam () {  
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
  }
  void debug_free_ram() {
    Serial.print(F("apcmini_control_change: Free RAM is "));
    Serial.println(freeRam());
  }
#endif