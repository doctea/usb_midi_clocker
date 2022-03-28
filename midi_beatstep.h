#ifdef ENABLE_BEATSTEP

#include "bpm.h"

MIDI_NAMESPACE::MidiInterface<UHS2MIDI_NAMESPACE::uhs2MidiTransport> *midi_beatstep;
volatile uint8_t ixBeatStep = 0xff;

volatile bool beatstep_started = false;

inline void beatstep_loop() {
  if ( ixBeatStep != 0xff) {
    ATOMIC(
      do {
        Midi[ixBeatStep]->read();
      } while ( MidiTransports[ixBeatStep]->available() > 0);
    )
  }
}

void beatstep_control_change (byte inChannel, byte inNumber, byte inValue) {
  if (inNumber==54 && inValue==127) {
    debug_println(F("BEATSTEP START PRESSED!"));
    beatstep_started = false;
  } else {
    debug_print(F("Received Beatstep CC "));
    debug_print(inNumber);
    debug_print(F(" with value "));
    debug_println(inValue);
    //handleNoteOn(inChannel, inNumber, inValue);
  }
}

void beatstep_handle_start() {
  debug_println(F("beatstep_handle_start()"));
  //ATOMIC(
    midi_beatstep->sendStart();
  //)
  beatstep_started = true;
  debug_println(F("beatstep_handle_start() finished"));
}

void beatstep_on_tick(uint32_t ticks) {
  //Serial.flush();
  
  if (midi_beatstep) {
    //debug_print(F("beatstep_on_tick:"));
#ifdef DEBUG_TICKS
    if (DEBUG_TICKS) debug_print(F(" beatstep "));
#endif
    //debug_println("about to test bpm on bar..");
    //Serial.flush();
    if (is_bpm_on_bar(ticks) && !beatstep_started) {
      debug_println(F("First beat of bar and BEATSTEP not started -- starting!"));
      //debug_println("First beat of bar and BEATSTEP not started -- starting!");
      //Serial.flush();
      //ATOMIC(
        midi_beatstep->sendStart();
      //);
      debug_println("sent start");
      //Serial.flush();
      beatstep_started = true;
    }

    //debug_println("about to send clock message");
    //Serial.flush();
    //ATOMIC(
      midi_beatstep->sendClock();
    //);
    //debug_println(F("sent clock"));
    
    //Serial.flush();
  } else {
    //debug_println("..no midi_beatstep detected!");
    //Serial.flush();
  }
}

// called inside interrupt
void beatstep_on_restart() {
  if (midi_beatstep) {
    debug_println(F("beatstep_on_restart()"));
    //ATOMIC(
      midi_beatstep->sendStop();
      midi_beatstep->sendStart();
    //)
    debug_println(F("beatstep_on_restart done"));
  }
}

void beatstep_init() {
    debug_println(F("beatstep_init()"));
    ATOMIC(
      beatstep_started = false;
    )

    midi_beatstep->turnThruOff();
    midi_beatstep->setHandleControlChange(beatstep_control_change);
    midi_beatstep->setHandleStart(beatstep_handle_start);    
    debug_println(F("beatstep_init() finished"));
    //)
}

#endif
