#ifndef BPM__INCLUDED
#define BPM__INCLUDED

#define PPQN  24

/*
#define IS_BPM_ON_BEAT (ticks==0 || (ticks%PPQN)==0)
#define IS_BPM_ON_TWO_BEAT (ticks==0 || (ticks%(2*PPQN))==0)
#define IS_BPM_ON_BAR  (ticks==0 || (ticks%(4*PPQN))==0)
#define IS_BPM_ON_HALF_BEAT (ticks==0 || (ticks%(PPQN/2))==0)
*/

unsigned long t1 = millis();
unsigned long ticks = 0;

// tracking which beat we're on
float bpm_current = 60.0f;
//double ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
unsigned long ms_per_tick = 40;

unsigned int started_at = 0;
#endif
