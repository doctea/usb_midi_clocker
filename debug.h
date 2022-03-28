
#ifdef DEBUG_OUTPUT
#define debug_print(X)    Serial.print(X)
#define debug_println(X)  Serial.println(X)
#else
#define debug_print(X)    if (false) { X; }
#define debug_println(X)  if (false) { X; }
#endif

int freeRam () {  
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void debug_free_ram() {
  debug_print(F("apcmini_control_change: Free RAM is "));
  debug_println(freeRam());
}
