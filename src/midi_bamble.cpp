#include <MIDI.h>
#include <USBHost_t36.h>

#include "bpm.h"
#include "Config.h"
#include "ConfigMidi.h"
#include "midi_outs.h"
#include "midi_bamble.h"

MIDIDevice *midi_bamble;  
uint8_t ixBamble   = 0xff;

bool bamble_started = false;

void bamble_loop() {
  if ( ixBamble == 0xff) { 
    return;
  }
  /*if ( ixBamble != 0xff) {
    while(midi_bamble->read());
  }*/

  /*ATOMIC(
    if ( ixBamble != 0xff) {
      do {
        Midi[ixBamble]->read();
      } while ( MidiTransports[ixBamble]->available() > 0);
    }
  )*/
}

// called inside interrupt
void bamble_on_tick(uint32_t ticks) {
  if (ixBamble != 0xFF) {
#ifdef DEBUG_TICKS
    Serial.print(F(" bamble "));
#endif
    #ifndef USE_UCLOCK
      if (is_bpm_on_bar(ticks) && !bamble_started) {
        Serial.println(F("First beat of bar and BAMBLE not started -- starting!"));
        midi_bamble->sendRealTime(usbMIDI.Start); //sendStart();
        bamble_started = true;
      }
    
      midi_bamble->sendRealTime(usbMIDI.Clock); //Clock();
      midi_bamble->send_now();
    #endif
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

    //midi_out_cv12_poly = midi_bamble;

    //midi_bamble->turnThruOff();
    //midi_bamble->setHandleControlChange(bamble_control_change);
    //midi_bamble->setHandleStart(bamble_handle_start);    
}
