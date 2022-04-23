#include "bpm.h"
#include "Config.h"
#include "ConfigMidi.h"
#include "midi_mpk49.h"
#include "midi_outs.h"

#include "midi_looper.h"

MIDIDevice_BigBuffer *midi_MPK49;  
uint8_t ixMPK49   = 0xff;

bool MPK49_started = false;
bool mpk49_recording = false;

void MPK49_loop(unsigned long ticks) {
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
  // playLoop(ticks%(PPQN*4*4*4));
  #ifdef ENABLE_RECORDING
    playInstruction(ticks); //%(LOOP_LENGTH));
  #endif
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

    #ifdef ENABLE_RECORDING
    if (mpk49_recording)
      recordInstruction(midi::NoteOn, channel, note, velocity);
    #endif

    #ifdef ENABLE_BITBOX
      if (midi_out_bitbox) {
          Serial.printf("sending to midi_out_bitbox\n");
          midi_out_bitbox->sendNoteOn(note, velocity, 3);
      } else {
          Serial.println();
      }
    #else
      Serial.println("No output device configured");
    #endif
}
void mpk49_handle_note_off(byte channel, byte note, byte velocity) {

  #ifdef ENABLE_RECORDING
  if (mpk49_recording)
    recordInstruction(midi::NoteOff, channel, note, velocity);
  #endif
    
  #ifdef ENABLE_BITBOX
    if (midi_out_bitbox) {
        Serial.printf("sending note off to midi_out_bitbox\n");
        midi_out_bitbox->sendNoteOff(note, velocity, 3);
    }
  #else
    Serial.println("mpk49_handle_note_off: no output device configured");
  #endif
}

#ifdef ENABLE_RECORDING
void mpk49_handle_mmc_record() {
  mpk49_recording = !mpk49_recording;
}

void mpk49_handle_system_exclusive(uint8_t *data, unsigned int size) {
  Serial.printf("mpk_handle_system_exclusive of size %i: [",size);
  for (int i = 0 ; i < size ; i++) {
    Serial.printf("%02x ", data[i]);
  }
  Serial.println("]");

  if (data[3]==0x06 && data[4]==0x06) {
    mpk49_handle_mmc_record();
  }
}
#endif

void MPK49_init() {
    MPK49_started = false;

    //midi_out_cv12_poly = midi_MPK49;
    //midi_MPK49->begin();

    //midi_MPK49->turnThruOff();
    //midi_MPK49->setHandleControlChange(MPK49_control_change);
    //midi_MPK49->setHandleStart(MPK49_handle_start);
    midi_MPK49->setHandleNoteOn(mpk49_handle_note_on);
    midi_MPK49->setHandleNoteOff(mpk49_handle_note_off);
    #ifdef ENABLE_RECORDING
    midi_MPK49->setHandleSystemExclusive(mpk49_handle_system_exclusive);
    #endif
}
