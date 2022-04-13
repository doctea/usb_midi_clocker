#ifdef ENABLE_BAMBLE

#include "bpm.h"

MIDIDevice *midi_bamble;
volatile uint8_t ixBamble   = 0xff;

volatile bool bamble_started = false;

inline void bamble_loop() {
  if ( ixBamble != 0xff) {
    while(midi_bamble->read());
  }

  /*ATOMIC(
    if ( ixBamble != 0xff) {
      do {
        Midi[ixBamble]->read();
      } while ( MidiTransports[ixBamble]->available() > 0);
    }
  )*/
}

// called inside interrupt
void bamble_on_tick(volatile uint32_t ticks) {
  if (midi_bamble) {
#ifdef DEBUG_TICKS
    Serial.print(F(" bamble "));
#endif
    if (is_bpm_on_bar(ticks) && !bamble_started) {
      Serial.println(F("First beat of bar and BAMBLE not started -- starting!"));
      midi_bamble->sendRealTime(usbMIDI.Start); //sendStart();
      bamble_started = true;
    }
    
    midi_bamble->sendRealTime(usbMIDI.Clock); //Clock();
    midi_bamble->send_now();
  }
}

// called inside interrupt
void bamble_on_restart() {
  if (midi_bamble) {
    //ATOMIC(
      midi_bamble->sendRealTime(usbMIDI.Stop); //sendStop();
      midi_bamble->sendRealTime(usbMIDI.Start); //sendStart();
    //)
  }
}

void bamble_init() {
    bamble_started = false;

    //midi_bamble->turnThruOff();
    //midi_bamble->setHandleControlChange(bamble_control_change);
    //midi_bamble->setHandleStart(bamble_handle_start);    
}

#endif
