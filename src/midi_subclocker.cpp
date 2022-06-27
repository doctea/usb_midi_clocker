//#define ATOMIC(X) noInterrupts(); X; interrupts();
#define ATOMIC(X) X

#include <Arduino.h>
#include "Config.h"
#include "ConfigMidi.h"
#include "MidiMappings.h"
#include "USBHost_t36.h"
#include "midi_subclocker.h"
#include "bpm.h"
#include "midi_helpers.h"

MIDIDevice_BigBuffer *midi_subclocker;
volatile uint8_t ixSubclocker = 0xff;
volatile bool subclocker_started = false;

void subclocker_loop(unsigned long ticks) {
  if ( ixSubclocker == 0xff) {
    return;
  }
  /*if ( ixSubclocker != 0xff) {
    while (midi_subclocker->read());
  }*/
}

/*void subclocker_control_change (byte inChannel, byte inNumber, byte inValue) {
  if (inNumber==54 && inValue==127) {
    Serial.println(F("SUBCLOCKER START PRESSED!"));
    subclocker_started = false;
  } else {
    Serial.print(F("Received SUBCLOCKER CC "));
    Serial.print(inNumber);
    Serial.print(F(" with value "));
    Serial.println(inValue);
    //handleNoteOn(inChannel, inNumber, inValue);
  }
}*/

int subclocker_divisor = DEFAULT_SUBCLOCKER_DIVISOR;
int subclocker_delay_ticks = DEFAULT_SUBCLOCKER_DELAY_TICKS;

void on_subclocker_divisor_changed(int new_value, int old_value) {

}

/*void update_subclocker_divisor(int divisor) {
    subclocker_divisor = divisor;
}*/

void subclocker_send_clock(unsigned long ticks) {
    if (ticks<subclocker_delay_ticks) return;
    if (ixSubclocker!=0xFF) 
        if (ticks==0 || ticks % subclocker_divisor == 0)
            midi_subclocker->sendRealTime(usbMIDI.Clock);
}

/*void subclocker_handle_start() {
  Serial.println(F("subclocker_handle_start()"));
  //ATOMIC(
    midi_subclocker->sendRealTime(usbMIDI.Start); //sendStart();
  //)
  subclocker_started = true;
  Serial.println(F("subclocker_handle_start() finished"));
}*/

void subclocker_on_tick(volatile uint32_t ticks) {
  //Serial.flush();
  //Serial.println("subclocker_on_tick()");
  
  if (ixSubclocker!=0xFF) { //} midi_subclocker!=nullptr) {
    //Serial.print(F("subclocker_on_tick:"));
    #ifdef DEBUG_TICKS
      if (DEBUG_TICKS) Serial.print(F(" subclocker "));
    #endif
    //Serial.println("about to test bpm on bar..");
    //Serial.flush();
    if (is_bpm_on_bar(ticks) && !subclocker_started) {
      Serial.println(F("First beat of bar and SUBCLOCKER not started -- starting!"));
      //Serial.println("First beat of bar and BEATSTEP not started -- starting!");2
      //Serial.flush();
      //ATOMIC(
        midi_subclocker->sendRealTime(usbMIDI.Start); //sendStart();
      //);
      Serial.println("sent start");
      //Serial.flush();
      subclocker_started = true;
    }
    //Serial.flush();
  } else {
    //Serial.println("..no midi_subclocker detected!");
    //Serial.flush();
  }
}


// called inside interrupt
void subclocker_on_restart() {
  if (ixSubclocker!=0xFF && midi_subclocker!=nullptr) {
    Serial.println(F("subclocker_on_restart()"));
    //ATOMIC(
      midi_subclocker->sendRealTime(usbMIDI.Stop); //sendStop();
      midi_subclocker->sendRealTime(usbMIDI.Start); //sendStart();
    //)

    Serial.println(F("subclocker_on_restart done"));
  }
}

void subclocker_init() {
  Serial.println(F("subclocker_init()"));
  ATOMIC(
    subclocker_started = false;
  )

  //midi_subclocker->turnThruOff();
  //midi_subclocker->setHandleNoteOn(subclocker_handle_note_on);
  //midi_subclocker->setHandleNoteOff(subclocker_handle_note_off);
  //midi_subclocker->setHandleControlChange(subclocker_control_change);
  //midi_subclocker->setHandleStart(subclocker_handle_start);    

  Serial.println(F("subclocker_init() finished"));
  //)
}

/*
#ifdef ENABLE_SCREEN
  void subclocker_display_key_status(ST7789_t3 *tft) {
    tft_header(tft, "subclocker:");
    tft->setTextColor(rgb(0xFFFFFF),0);
    tft->setTextSize(2);

    tft->printf("%4s : %4s", get_note_name(last_subclocker_note).c_str(), get_note_name(current_subclocker_note).c_str());
    //Serial.printf("subclocker_display_key_status got %i aka %4s : %4s\n", current_subclocker_note, get_note_name(last_subclocker_note).c_str(), get_note_name(current_subclocker_note).c_str());
  }
#endif
*/