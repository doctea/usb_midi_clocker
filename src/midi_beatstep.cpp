//#define ATOMIC(X) noInterrupts(); X; interrupts();
#define ATOMIC(X) X

#include <Arduino.h>
#include "Config.h"
#include "USBHost_t36.h"
#include "midi_beatstep.h"
#include "bpm.h"
#include "midi_helpers.h"

MIDIDevice_BigBuffer *midi_beatstep;
volatile uint8_t ixBeatStep = 0xff;
volatile bool beatstep_started = false;

int current_beatstep_note = -1;
int last_beatstep_note = -1;

void beatstep_loop(unsigned long ticks) {
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
    #ifndef USE_UCLOCK
      //midi_beatstep->sendRealTime(usbMIDI.Clock); //sendClock();
      //midi_beatstep->send_now();
    #endif
    
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

void beatstep_handle_note_on(byte channel, byte note, byte velocity) {
  current_beatstep_note = note;
  //Serial.printf("beatstep got note on %i\n", note);
}
#
void beatstep_handle_note_off(byte channel, byte note, byte velocity) {
  if (current_beatstep_note==note) 
    current_beatstep_note = -1;
  last_beatstep_note = note;
  //Serial.printf("beatstep got note off %i\n", note);
}

void beatstep_init() {
  Serial.println(F("beatstep_init()"));
  ATOMIC(
    beatstep_started = false;
  )

  //midi_beatstep->turnThruOff();
  midi_beatstep->setHandleNoteOn(beatstep_handle_note_on);
  midi_beatstep->setHandleNoteOff(beatstep_handle_note_off);
  midi_beatstep->setHandleControlChange(beatstep_control_change);
  midi_beatstep->setHandleStart(beatstep_handle_start);    
  Serial.println(F("beatstep_init() finished"));
  //)
}

#ifdef ENABLE_SCREEN
  void beatstep_display_key_status(ST7789_t3 *tft) {
    tft_header(tft, "beatstep:");
    tft->setTextColor(rgb(0xFFFFFF),0);
    tft->setTextSize(2);

    tft->printf("%4s : %4s", get_note_name(last_beatstep_note).c_str(), get_note_name(current_beatstep_note).c_str());
    //Serial.printf("beatstep_display_key_status got %i aka %4s : %4s\n", current_beatstep_note, get_note_name(last_beatstep_note).c_str(), get_note_name(current_beatstep_note).c_str());
  }
#endif