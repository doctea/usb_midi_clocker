#include "bpm.h"
#include "Config.h"
#include "ConfigMidi.h"
#include "midi_apk49.h"
#include "midi_outs.h"

MIDIDevice *midi_MPK49;  
uint8_t ixMPK49   = 0xff;

bool MPK49_started = false;

void MPK49_loop() {
  if ( ixMPK49 == 0xff ) {
    return;
  }

  /*if ( ixMPK49 != 0xff && midi_MPK49) {
    while(midi_MPK49->read());
  }*/

  /*ATOMIC(
    if ( ixMPK49 != 0xff) {
      do {
        Midi[ixMPK49]->read();
      } while ( MidiTransports[ixMPK49]->available() > 0);
    }
  )*/
}

// called inside interrupt
void MPK49_on_tick(uint32_t ticks) {
  if (ixMPK49 != 0xFF) {
#ifdef DEBUG_TICKS
    Serial.print(F(" MPK49 "));
#endif
    /*if (is_bpm_on_bar(ticks) && !MPK49_started) {
      Serial.println(F("First beat of bar and MPK49 not started -- starting!"));
      midi_MPK49->sendRealTime(usbMIDI.Start); //sendStart();
      MPK49_started = true;
    }
    
    midi_MPK49->sendRealTime(usbMIDI.Clock); //Clock();
    midi_MPK49->send_now();*/
  }
}

// called inside interrupt
void MPK49_on_restart() {
  /*if (midi_MPK49) {
    //ATOMIC(
      midi_MPK49->sendRealTime(usbMIDI.Stop); //sendStop();
      midi_MPK49->sendRealTime(usbMIDI.Start); //sendStart();
    //)
  }*/
}

void mpk49_handle_note_on(byte channel, byte note, byte velocity) {
    static int counter = 0;
    Serial.printf("%i: mpk49_handle_note_on %i, %i, %i: \n", counter++, channel, note, velocity);

    if (midi_out_bitbox) {
        Serial.printf("sending to midi_out_bitbox\n");
        midi_out_bitbox->sendNoteOn(note, velocity, 3);
    } else {
        Serial.println();
    }
}
void mpk49_handle_note_off(byte channel, byte note, byte velocity) {
    if (midi_out_bitbox) {
        Serial.printf("sending note off to midi_out_bitbox\n");
        midi_out_bitbox->sendNoteOff(note, velocity, 3);
    }
}

void MPK49_init() {
    MPK49_started = false;

    //midi_out_cv12_poly = midi_MPK49;
    //midi_MPK49->begin();

    //midi_MPK49->turnThruOff();
    //midi_MPK49->setHandleControlChange(MPK49_control_change);
    //midi_MPK49->setHandleStart(MPK49_handle_start);
    midi_MPK49->setHandleNoteOn(mpk49_handle_note_on);
    midi_MPK49->setHandleNoteOff(mpk49_handle_note_off);
}
