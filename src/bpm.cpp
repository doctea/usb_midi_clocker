#include "Config.h"
#include "bpm.h"
#include "clock.h"

volatile uint32_t ticks = 0;
float bpm_current = 120.0f; //BPM_MINIMUM; //60.0f;
double ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));

long last_processed_tick = -1;

bool playing = true;
bool single_step = false;
bool restart_on_next_bar = false;

void set_bpm(float new_bpm) {
  if (bpm_current!=new_bpm) {
    bpm_current = new_bpm;
    #ifdef USE_UCLOCK
      //ATOMIC(
          //uClock.setTempo(new_bpm); //bpm_current * 24);
      //)
      set_new_bpm(bpm_current);
    #else
      ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
      Serial.printf("set ms_per_tick to %f\n", ms_per_tick);
    #endif
    Serial.print(F("set bpm to "));
    Serial.println(bpm_current);
  }
}
