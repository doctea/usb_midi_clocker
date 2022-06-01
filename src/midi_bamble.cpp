#include <MIDI.h>
#include <USBHost_t36.h>

#include "bpm.h"
#include "Config.h"
#include "ConfigMidi.h"
#include "midi_outs.h"
#include "midi_bamble.h"

#include "midi_out_wrapper.h"

MIDIDevice_BigBuffer *midi_bamble;  
uint8_t ixBamble   = 0xff;

//MIDIOutputWrapper *bamble_1_output  = &MIDIOutputWrapper("USB : Bamble : ch 1", &midi_bamble, 1); // late binding pointer to usb
//MIDIOutputWrapper *bamble_2_output  = &MIDIOutputWrapper("USB : Bamble : ch 2", &midi_bamble, 2); // late binding pointer to usb

bool bamble_started = false;

void bamble_loop(unsigned long ticks) {
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
    /*if (is_bpm_on_bar(ticks) && !bamble_started) {
      Serial.println(F("First beat of bar and BAMBLE not started -- starting!"));
      midi_bamble->sendRealTime(usbMIDI.Start); //sendStart();
      bamble_started = true;
    }*/
  
    //midi_bamble->sendRealTime(usbMIDI.Clock); //Clock();
    //midi_bamble->send_now();
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

    // this should disable euclidian pulses on the pitch outputs ch1 + ch2
    midi_bamble->sendControlChange(78, 0, 10);
    midi_bamble->sendControlChange(79, 0, 10);
    midi_bamble->sendControlChange(50, 0, 10);
    midi_bamble->sendControlChange(51, 0, 10);

    // sustain to max for the envelope outputs
    midi_bamble->sendControlChange(67, 127, 10);
    midi_bamble->sendControlChange(67, 127, 11);
    midi_bamble->sendControlChange(75, 127, 10);
    midi_bamble->sendControlChange(75, 127, 11);
    midi_bamble->sendControlChange(83, 127, 10);
    midi_bamble->sendControlChange(83, 127, 11);
    midi_bamble->sendControlChange(91, 127, 10);
    midi_bamble->sendControlChange(91, 127, 11);
    midi_bamble->sendControlChange(99, 127, 10);
    midi_bamble->sendControlChange(99, 127, 11);

    //midi_out_cv12_poly = midi_bamble;

    //midi_bamble->turnThruOff();
    //midi_bamble->setHandleControlChange(bamble_control_change);
    //midi_bamble->setHandleStart(bamble_handle_start);    
}
