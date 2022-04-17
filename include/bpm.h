#ifndef BPM__INCLUDED
#define BPM__INCLUDED

#include <Arduino.h>

#define PPQN  24
#define BEATS_PER_BAR 4

#define BPM_MINIMUM   60.0
#define BPM_MAXIMUM   140.0

extern bool playing;
extern bool single_step;

extern bool restart_on_next_bar;

#ifdef USE_UCLOCK
volatile 
#endif
extern uint32_t ticks; // = 0;

// tracking which beat we're on
extern float bpm_current; //BPM_MINIMUM; //60.0f;
#ifndef USE_UCLOCK
extern double ms_per_tick; // = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
#endif

inline bool is_bpm_on_phrase(uint32_t ticks,      unsigned long offset = 0) { return ticks==offset || ticks%(PPQN*4*4) == offset; }
inline bool is_bpm_on_bar(uint32_t    ticks,      unsigned long offset = 0) { return ticks==offset || ticks%(PPQN*4)   == offset; }
inline bool is_bpm_on_half_bar(uint32_t  ticks,   unsigned long offset = 0) { return ticks==offset || ticks%(PPQN*2)   == offset; }
inline bool is_bpm_on_beat(uint32_t  ticks,       unsigned long offset = 0) { return ticks==offset || ticks%(PPQN)     == offset; }
inline bool is_bpm_on_eighth(uint32_t  ticks,     unsigned long offset = 0) { return ticks==offset || ticks%(PPQN/2)   == offset; }
inline bool is_bpm_on_sixteenth(uint32_t  ticks,  unsigned long offset = 0) { return ticks==offset || ticks%(PPQN/4)   == offset; }

inline bool is_bpm_on_multiplier(unsigned long ticks, float multiplier, unsigned long offset = 0) {
  unsigned long p = ((float)PPQN*multiplier);
#ifdef DEBUG_BPM
  Serial.print(F("is_bpm_on_multiplier(ticks="));
  Serial.print(ticks);
  Serial.print(F(", multiplier="));
  Serial.print(multiplier);
  Serial.print(F(", offset="));
  Serial.print(offset);
  Serial.print(F(") checking ticks "));
  Serial.print(ticks);
  Serial.print(F(" with PPQN*multiplier "));
  Serial.print(p);
  Serial.print(F(" against offset "));
  Serial.print(offset);
  Serial.print(F(" == ticks%p = "));
  Serial.print(ticks%p);
  Serial.print(F(" ? ="));
#endif

  bool v = (ticks==offset || ticks%p == offset);  

#ifdef DEBUG_BPM
  Serial.print(v ? F("true!") : F("false!"));
  Serial.println();
#endif
  return v;
}

void set_bpm(float new_bpm);

#endif
