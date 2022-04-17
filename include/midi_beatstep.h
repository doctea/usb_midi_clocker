#ifndef BEATSTEP__INCLUDED
#define BEATSTEP__INCLUDED
#ifdef ENABLE_BEATSTEP

#include <Arduino.h>
#include "bpm.h"

MIDIDevice *midi_beatstep;
volatile uint8_t ixBeatStep = 0xff;

volatile bool beatstep_started = false;

inline void beatstep_loop() {
  if ( ixBeatStep == 0xff) {
    return;
  }
  /*if ( ixBeatStep != 0xff) {
    while (midi_beatstep->read());
  }*/
}

void beatstep_control_change (byte inChannel, byte inNumber, byte inValue) {
  if (inNumber==54 && inValue==127) {
    Serial.println(F("BEATSTEP START PRESSED!"));
    beatstep_started = false;
  } else {
    Serial.print(F("Received Beatstep CC "));
    Serial.print(inNumber);
    Serial.print(F(" with value "));
    Serial.println(inValue);
    //handleNoteOn(inChannel, inNumber, inValue);
  }
}

void beatstep_handle_start() {
  Serial.println(F("beatstep_handle_start()"));
  //ATOMIC(
    midi_beatstep->sendRealTime(usbMIDI.Start); //sendStart();
  //)
  beatstep_started = true;
  Serial.println(F("beatstep_handle_start() finished"));
}

void beatstep_on_tick(volatile uint32_t ticks) {
  //Serial.flush();
  //Serial.println("beatstep_on_tick()");
  
  if (ixBeatStep!=0xFF) { //} midi_beatstep!=nullptr) {
    //Serial.print(F("beatstep_on_tick:"));
#ifdef DEBUG_TICKS
    if (DEBUG_TICKS) Serial.print(F(" beatstep "));
#endif
    //Serial.println("about to test bpm on bar..");
    //Serial.flush();
    if (is_bpm_on_bar(ticks) && !beatstep_started) {
      Serial.println(F("First beat of bar and BEATSTEP not started -- starting!"));
      //Serial.println("First beat of bar and BEATSTEP not started -- starting!");
      //Serial.flush();
      //ATOMIC(
        midi_beatstep->sendRealTime(usbMIDI.Start); //sendStart();
      //);
      Serial.println("sent start");
      //Serial.flush();
      beatstep_started = true;
    }

    //Serial.println("about to send clock message to beatstep");
    //Serial.flush();
    //ATOMIC(
      midi_beatstep->sendRealTime(usbMIDI.Clock); //sendClock();
      midi_beatstep->send_now();
    //);
    //Serial.println(F("sent clock"));
    
    //Serial.flush();
  } else {
    //Serial.println("..no midi_beatstep detected!");
    //Serial.flush();
  }
}

// called inside interrupt
void beatstep_on_restart() {
  if (midi_beatstep) {
    Serial.println(F("beatstep_on_restart()"));
    //ATOMIC(
      midi_beatstep->sendRealTime(usbMIDI.Stop); //sendStop();
      midi_beatstep->sendRealTime(usbMIDI.Start); //sendStart();
    //)
    Serial.println(F("beatstep_on_restart done"));
  }
}

void beatstep_init() {
    Serial.println(F("beatstep_init()"));
    ATOMIC(
      beatstep_started = false;
    )

    //midi_beatstep->turnThruOff();
    midi_beatstep->setHandleNoteOn(nullptr);
    midi_beatstep->setHandleNoteOff(nullptr);
    midi_beatstep->setHandleControlChange(beatstep_control_change);
    midi_beatstep->setHandleStart(beatstep_handle_start);    
    Serial.println(F("beatstep_init() finished"));
    //)
}

#endif

#endif