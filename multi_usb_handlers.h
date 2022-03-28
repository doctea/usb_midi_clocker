#include "bpm.h"
/*
BEATSTEP:   Initialised device vendorid: 7285   productid: 518
BAMBLEWEENY:  Initialised device vendorid: 10374    productid: 32779
AKAI APCMINI: Initialised device vendorid: 2536   productid: 40
*/

void setupmidi(uint8_t idx)
{
  uint16_t vid = MidiTransports[idx]->idVendor();
  uint16_t pid = MidiTransports[idx]->idProduct();
  char buf[16];
  sprintf(buf, "%d:%04X,%04X - ",  idx, vid, pid);
  debug_print(buf);

#ifdef ENABLE_BEATSTEP
  if ( vid == 0x1c75 && pid == 0x0206 ) {         //is Arturia BeatStep?
    ixBeatStep = idx;
    debug_print(F("BeatStep connected..."));
    midi_beatstep = Midi[idx];
    beatstep_init();
    debug_println(F("completed Beatstep init"));
    return;
  } 
#endif
#ifdef ENABLE_APCMINI
  if ( vid == 0x09e8 && pid == 0x0028 ) {   //is AKAI APCmini?
    ixAPCmini = idx;
    debug_println(F("AKAI APCmini connected."));
    midi_apcmini = Midi[idx];
    apcmini_init();
    return;
  }
#endif
#ifdef ENABLE_BAMBLE
  if ( vid == 0x2886 && pid == 0x800B ) {            //is BAMBLE?
    ixBamble = idx;
    debug_println(F("BAMBLEWEENY connected."));
    midi_bamble = Midi[idx];
    bamble_init();
    return;
  }
#endif

  debug_print(F("Detected unknown (or disabled) device vid="));
  debug_print(vid);
  debug_print(F(", pid="));
  debug_println(pid);
}

// call this when global clock should be reset
void on_restart() {
  restart_on_next_bar = false;

  debug_println(F("on_restart()==>"));
  
  debug_println(F("reset ticks"));
  // TODO: cheapclock version
#ifdef USE_UCLOCK
  uClock.setTempo(bpm_current); // todo: probably not needed?
  debug_println(F("reset tempo"));
  uClock.resetCounters();
  debug_println(F("reset counters"));
#else
  ticks = 0;
#endif
  
#ifdef ENABLE_BEATSTEP
  debug_print(F("restart beatstep..."));
  beatstep_on_restart();
  debug_println(F("restarted"));
#endif
#ifdef ENABLE_BAMBLE
  debug_print(F("restart bamble..."));
  bamble_on_restart();
  debug_println(F("restarted"));
#endif
#ifdef ENABLE_APCMINI
  debug_print(F("restart apcmini..."));
  apcmini_on_restart();
  debug_println(F("restarted"));
  redraw_immediately = true;
#endif
  debug_println(F("<==on_restart()"));
}

/*
void start_clocks_if_stopped() {
  if (!beatstep_started && midi_beatstep) {
    debug_println("BEATSTEP not started -- starting!");
    midi_beatstep->sendStart();
    beatstep_started = true;
  }
  if (!bamble_started && midi_bamble) {
    midi_bamble->sendStart();
    bamble_started = true;
  }
  if (!apcmini_started && midi_apcmini) {
    apcmini_started = true;
  }
}
*/
