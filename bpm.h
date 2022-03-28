#ifndef BPM__INCLUDED
#define BPM__INCLUDED

#define PPQN  24
#define BEATS_PER_BAR 4

#define BPM_MINIMUM   60.0
#define BPM_MAXIMUM   140.0

bool playing = true;
bool single_step = false;

volatile bool restart_on_next_bar = false;

#ifdef USE_UCLOCK
volatile 
#endif
uint32_t ticks = 0;

// tracking which beat we're on
volatile static float bpm_current = 120.0f; //BPM_MINIMUM; //60.0f;
#ifndef USE_UCLOCK
double ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
#endif

inline bool is_bpm_on_phrase(uint32_t ticks,      signed long offset = 0) { return ticks==offset || ticks%(PPQN*4*4) == offset; }
inline bool is_bpm_on_bar(uint32_t    ticks,      signed long offset = 0) { return ticks==offset || ticks%(PPQN*4)   == offset; }
inline bool is_bpm_on_half_bar(uint32_t  ticks,   signed long offset = 0) { return ticks==offset || ticks%(PPQN*2)   == offset; }
inline bool is_bpm_on_beat(uint32_t  ticks,       signed long offset = 0) { return ticks==offset || ticks%(PPQN)     == offset; }
inline bool is_bpm_on_eighth(uint32_t  ticks,     signed long offset = 0) { return ticks==offset || ticks%(PPQN/2)   == offset; }
inline bool is_bpm_on_sixteenth(uint32_t  ticks,  signed long offset = 0) { return ticks==offset || ticks%(PPQN/4)   == offset; }

inline bool is_bpm_on_multiplier(signed long ticks, float multiplier, signed long offset = 0) {
  unsigned long p = ((float)PPQN*multiplier);
#ifdef DEBUG_BPM
  debug_print(F("is_bpm_on_multiplier(ticks="));
  debug_print(ticks);
  debug_print(F(", multiplier="));
  debug_print(multiplier);
  debug_print(F(", offset="));
  debug_print(offset);
  debug_print(F(") checking ticks "));
  debug_print(ticks);
  debug_print(F(" with PPQN*multiplier "));
  debug_print(p);
  debug_print(F(" against offset "));
  debug_print(offset);
  debug_print(F(" == ticks%p = "));
  debug_print(ticks%p);
  debug_print(F(" ? ="));
#endif
ATOMIC(
  bool v = (ticks==offset || ticks%p == offset);  
)
#ifdef DEBUG_BPM
  debug_print(v ? F("true!") : F("false!"));
  debug_println();
#endif
  return v;
}

volatile bool set_tempo = false;

void set_bpm(float new_bpm) {
  if (bpm_current!=new_bpm) {
    bpm_current = new_bpm;
#ifdef USE_UCLOCK
//ATOMIC(
    set_tempo = true;
    //uClock.setTempo(new_bpm); //bpm_current * 24);
//)
#else
    ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
#endif
    debug_print(F("set bpm to "));
    debug_println(bpm_current);
  }
}

#endif
