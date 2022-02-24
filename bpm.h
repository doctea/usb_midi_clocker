#ifndef BPM__INCLUDED
#define BPM__INCLUDED

#define PPQN  24

// tracking which beat we're on
float bpm_current = 60.0f;
//double ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
unsigned long ms_per_tick = 40;

unsigned int started_at = 0;
#endif
