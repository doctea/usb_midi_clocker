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
  Serial.print(buf);

#ifdef ENABLE_BEATSTEP
  if ( vid == 0x1c75 && pid == 0x0206 ) {         //is Arturia BeatStep?
    ixBeatStep = idx;
    Serial.print(F("BeatStep connected..."));
    midi_beatstep = Midi[idx];
    beatstep_init();
    Serial.println(F("completed Beatstep init"));
    return;
  } 
#endif
#ifdef ENABLE_APCMINI
  if ( vid == 0x09e8 && pid == 0x0028 ) {   //is AKAI APCmini?
    ixAPCmini = idx;
    Serial.println(F("AKAI APCmini connected."));
    midi_apcmini = Midi[idx];
    apcmini_init();
    return;
  }
#endif
#ifdef ENABLE_BAMBLE
  if ( vid == 0x2886 && pid == 0x800B ) {            //is BAMBLE?
    ixBamble = idx;
    Serial.println(F("BAMBLEWEENY connected."));
    midi_bamble = Midi[idx];
    bamble_init();
    return;
  }
#endif

  Serial.print(F("Detected unknown (or disabled) device vid="));
  Serial.print(vid);
  Serial.print(F(", pid="));
  Serial.println(pid);
}

// call this when global clock should be reset
void on_restart() {
    restart_on_next_bar = false;

    Serial.println(F("on_restart()==>"));
    //ATOMIC(ticks = 0;)
    Serial.println(F("reset ticks"));
    // TODO: cheapclock version
    uClock.setTempo(bpm_current); // todo: probably not needed?
    Serial.println(F("reset tempo"));
    uClock.resetCounters();
    Serial.println(F("reset counters"));
    //uClock.getTick(&ticks);
#ifdef ENABLE_BEATSTEP
    Serial.print(F("restart beatstep..."));
    beatstep_on_restart();
    Serial.println(F("restarted"));
#endif
#ifdef ENABLE_BAMBLE
    Serial.print(F("restart bamble..."));
    bamble_on_restart();
    Serial.println(F("restarted"));
#endif
#ifdef ENABLE_APCMINI
    Serial.print(F("restart apcmini..."));
    apcmini_on_restart();
    Serial.println(F("restarted"));
    redraw_immediately = true;
#endif
    Serial.println(F("<==on_restart()"));
}

/*
void start_clocks_if_stopped() {
  if (!beatstep_started && midi_beatstep) {
    Serial.println("BEATSTEP not started -- starting!");
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
