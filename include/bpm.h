#ifndef BPM__INCLUDED
#define BPM__INCLUDED

#include <Arduino.h>

#define PPQN  24
#define BEATS_PER_BAR   4
#define BARS_PER_PHRASE 4

#define BPM_MINIMUM   60.0
#define BPM_MAXIMUM   140.0

extern bool playing;
extern bool single_step;

extern bool restart_on_next_bar;

volatile extern uint32_t ticks; // = 0;
extern long last_processed_tick;

// tracking which beat we're on
extern float bpm_current; //BPM_MINIMUM; //60.0f;
#ifndef USE_UCLOCK
extern double ms_per_tick; // = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
#endif

#define BPM_CURRENT_PHRASE          (ticks / (PPQN*4*4))
#define BPM_CURRENT_BAR_OF_PHRASE   (ticks % (PPQN*4*4) / (PPQN*4))
#define BPM_CURRENT_BEAT_OF_BAR     (ticks % (PPQN*4) / PPQN)

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

/* // TODO: figure out why this isnt work properly! 
class TapBPMTracker {
  public:
    #define beat_sample_size 4
    unsigned long last_beat_stamp[beat_sample_size];
    int ph = 0;
    unsigned long long last_beat_at = 0;
    double estimated_ms_per_tick;
    double last_average;
  
    void reset() {
      memset(last_beat_stamp,0,sizeof(last_beat_stamp)/sizeof(int));
    }

    void push_beat() {
      if (last_beat_at!=0) {
        last_beat_stamp[ph] = millis() - last_beat_at;
        Serial.printf("push_beat setting slot %i to %u\n", ph, last_beat_stamp[ph]);
        ph++;
      }
      last_beat_at = millis();
      if (ph>=beat_sample_size) ph = 0;
    }

    // pinched from interwebs and modified
    double average_step_length (int len)  // assuming array is int.
    {
      unsigned long sum = 0L ;  // sum will be larger than an item, long for safety.
      int counted = 0;
      for (int i = 0 ; i < beat_sample_size ; i++) {
        if (last_beat_stamp[i]==0l) continue;
        sum += last_beat_stamp [i] ;
        Serial.printf("%i got duration %u [sum is %u],\t", i, last_beat_stamp[i], sum);
        counted++;  
      }
      
      if (counted >= len) {
        last_average = (((double)sum) / (double)counted);
        Serial.printf(": counted %i with sum %u, returning %3.3f \r\n", counted, sum, last_average);
        return last_average;  // average will be fractional, so float may be appropriate.
      } else {
        Serial.printf(": returning last value %3.3f\r\n", last_average);
        return last_average;
      }
    }

    double bpm_calculate_current () {
      // use this block to calculate BPM from the start of receiving ticks -- this will not track well when BPM is changed!! 
      //double beats = (double)received_ticks/(double)PPQN;
      //double elapsed_ms = millis() - first_tick_received_at;

      // this version uses average of the last 4 received beats (actually steps) to calculate BPM so it tracks to changes better 
      double beats = 1.0f / (double)beat_sample_size; //0.25; //1;
      
      double elapsed_ms = average_step_length(4); // / 1000.0;

      double elapsed_minutes = (elapsed_ms / 60000.0f);
      double bpm = (beats / elapsed_minutes);
      // speed = distance/time
      // bpm = beats/time

      estimated_ms_per_tick = 60000.0f/(bpm*PPQN);

      //Serial.printf("bpm calculated as %3.3f (from elapsed_ms %3.3f and beats %3.3f)\r\n", bpm, elapsed_ms,  beats);

      return bpm;
    }
};

extern TapBPMTracker tap_tempo_tracker;
*/

#endif
