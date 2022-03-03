#ifndef BPM__INCLUDED
#define BPM__INCLUDED

#define PPQN  24

#define BPM_MINIMUM   60
#define BPM_MAXIMUM   140

bool playing = true;
bool single_step = false;

unsigned long t1 = millis();
unsigned long ticks = 0;

// tracking which beat we're on
float bpm_current = 60.0f;
double ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
//unsigned long ms_per_tick = 40;

unsigned int started_at = 0;

inline bool is_bpm_on_phrase(signed long ticks,     signed long offset = 0) { return ticks==offset || ticks%(PPQN*4*4) == offset; }
inline bool is_bpm_on_bar(signed long ticks,        signed long offset = 0) { return ticks==offset || ticks%(PPQN*4)   == offset; }
inline bool is_bpm_on_half_bar(signed long ticks,   signed long offset = 0) { return ticks==offset || ticks%(PPQN*2)   == offset; }
inline bool is_bpm_on_beat(signed long ticks,       signed long offset = 0) { return ticks==offset || ticks%(PPQN)     == offset; }
inline bool is_bpm_on_eighth(signed long ticks,     signed long offset = 0) { return ticks==offset || ticks%(PPQN/2)   == offset; }
inline bool is_bpm_on_sixteenth(signed long ticks,  signed long offset = 0) { return ticks==offset || ticks%(PPQN/4)   == offset; }

inline bool is_bpm_on_multiplier(signed long ticks, float multiplier, signed long offset = 0) {
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
  bool v = (ticks==offset || ticks%p == offset);  // TODO: test whether this ticks==offset business is causing the little stutter on restart?
#ifdef DEBUG_BPM
  Serial.print(v ? F("true!") : F("false!"));
  Serial.println();
#endif
  return v;
}

void set_bpm(unsigned int new_bpm) {
    bpm_current = new_bpm;
    Serial.print(F("set bpm to "));
    Serial.println(bpm_current);
    ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
}

#endif
