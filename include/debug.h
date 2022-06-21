#include <Arduino.h>

#if defined(__arm__) && defined(CORE_TEENSY)
  void debug_free_ram() {
    Serial.println(F("debug_free_ram() not implemented on Teensy"));
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
