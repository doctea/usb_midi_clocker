#ifndef BPM__INCLUDED
#define BPM__INCLUDED

#define PPQN  24

/*
#define IS_BPM_ON_BEAT (ticks==0 || (ticks%PPQN)==0)
#define IS_BPM_ON_TWO_BEAT (ticks==0 || (ticks%(2*PPQN))==0)
#define IS_BPM_ON_BAR  (ticks==0 || (ticks%(4*PPQN))==0)
#define IS_BPM_ON_HALF_BEAT (ticks==0 || (ticks%(PPQN/2))==0)
*/

inline bool is_bpm_on_phrase(unsigned long ticks,     unsigned long offset = 0) { return ticks==offset || ticks%(PPQN*4*4) == offset; }
inline bool is_bpm_on_bar(unsigned long ticks,        unsigned long offset = 0) { return ticks==offset || ticks%(PPQN*4)   == offset; }
inline bool is_bpm_on_half_bar(unsigned long ticks,   unsigned long offset = 0) { return ticks==offset || ticks%(PPQN*2)   == offset; }
inline bool is_bpm_on_beat(unsigned long ticks,       unsigned long offset = 0) { return ticks==offset || ticks%(PPQN)     == offset; }
inline bool is_bpm_on_eighth(unsigned long ticks,     unsigned long offset = 0) { return ticks==offset || ticks%(PPQN/2)   == offset; }
inline bool is_bpm_on_sixteenth(unsigned long ticks,  unsigned long offset = 0) { return ticks==offset || ticks%(PPQN/4)   == offset; }

unsigned long t1 = millis();
unsigned long ticks = 0;

// tracking which beat we're on
float bpm_current = 60.0f;
//double ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
unsigned long ms_per_tick = 40;

unsigned int started_at = 0;
#endif
