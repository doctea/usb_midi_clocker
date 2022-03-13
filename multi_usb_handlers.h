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

  if ( vid == 0x1c75 && pid == 0x0206 ) {         //is Arturia BeatStep?
    ixBeatStep = idx;
    Serial.println(F("BeatStep connected."));
    midi_beatstep = Midi[idx];
    beatstep_init();
    return;
  } else if ( vid == 0x09e8 && pid == 0x0028 ) {   //is AKAI APCmini?
    ixAPCmini = idx;
    Serial.println(F("AKAI APCmini connected."));
    midi_apcmini = Midi[idx];
    apcmini_init();
    return;
  } else if ( vid == 0x2886 && pid == 0x800B ) {            //is BAMBLE?
    ixBamble = idx;
    Serial.println(F("BAMBLEWEENY connected."));
    midi_bamble = Midi[idx];
    bamble_init();
    return;
  }
}

// call this when global clock should be reset
void on_restart() {
    ticks = 0;
    beatstep_on_restart();
    bamble_on_restart();
    apcmini_on_restart();
    redraw_immediately = true;
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
