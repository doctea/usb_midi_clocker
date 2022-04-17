#ifndef keystep__INCLUDED
#define keystep__INCLUDED

#ifdef ENABLE_KEYSTEP

#include "bpm.h"
#include "Config.h"
#include "ConfigMidi.h"
#include "midi_outs.h"

MIDIDevice *midi_keystep;  
uint8_t ixKeystep   = 0xff;

bool keystep_started = false;

inline void keystep_loop() {
  if ( ixKeystep == 0xff ) {
      return;
  }
  /*if ( ixKeystep != 0xff && midi_keystep) {
    while(midi_keystep->read());
  }*/

  /*ATOMIC(
    if ( ixkeystep != 0xff) {
      do {
        Midi[ixkeystep]->read();
      } while ( MidiTransports[ixkeystep]->available() > 0);
    }
  )*/
}

// called inside interrupt
void keystep_on_tick(uint32_t ticks) {
  if (ixKeystep != 0xFF) {
#ifdef DEBUG_TICKS
    Serial.print(F(" keystep "));
#endif
    /*if (is_bpm_on_bar(ticks) && !keystep_started) {
      Serial.println(F("First beat of bar and keystep not started -- starting!"));
      midi_keystep->sendRealTime(usbMIDI.Start); //sendStart();
      keystep_started = true;
    }
    
    midi_keystep->sendRealTime(usbMIDI.Clock); //Clock();
    midi_keystep->send_now();*/
  }
}

// called inside interrupt
void keystep_on_restart() {
  /*if (midi_keystep) {
    //ATOMIC(
      midi_keystep->sendRealTime(usbMIDI.Stop); //sendStop();
      midi_keystep->sendRealTime(usbMIDI.Start); //sendStart();
    //)
  }*/
}

void keystep_handle_note_on(byte channel, byte note, byte velocity) {
    static int counter = 0;
    Serial.printf("%i: keystep_handle_note_on %i, %i, %i: \n", counter++, channel, note, velocity);

    if (midi_out_bitbox) {
        Serial.printf("sending to midi_out_bitbox\n");
        midi_out_bitbox->sendNoteOn(note, velocity, 3);
    } else {
        Serial.println();
    }
}
void keystep_handle_note_off(byte channel, byte note, byte velocity) {
    static int counter = 0;
    Serial.printf("%i: keystep_handle_note_off %i, %i, %i: \n", counter++, channel, note, velocity);

    if (midi_out_bitbox) {
        Serial.printf("sending note off to midi_out_bitbox\n");
        midi_out_bitbox->sendNoteOff(note, velocity, 3);
    }
}

void keystep_init() {
    keystep_started = false;

    //midi_out_cv12_poly = midi_keystep;
    //midi_keystep->begin();

    //midi_keystep->turnThruOff();
    //midi_keystep->setHandleControlChange(keystep_control_change);
    //midi_keystep->setHandleStart(keystep_handle_start);
    midi_keystep->setHandleNoteOn(keystep_handle_note_on);
    midi_keystep->setHandleNoteOff(keystep_handle_note_off);
}

#endif


#endif