//#define ATOMIC(X) noInterrupts(); X; interrupts();
#define ATOMIC(X) X

#include <Arduino.h>
#include "Config.h"
#include "ConfigMidi.h"
#include "MidiMappings.h"
#include "USBHost_t36.h"
#include "midi_beatstep.h"
#include "bpm.h"
#include "midi_helpers.h"

MIDIDevice_BigBuffer *midi_beatstep;
volatile uint8_t ixBeatStep = 0xff;
volatile bool beatstep_started = false;

int current_beatstep_note = -1;
int last_beatstep_note = -1;

int bass_transpose_octave = 2;

MIDIOutputWrapper *beatstep_output = &midi_out_bass_wrapper;
void beatstep_setOutputWrapper(MIDIOutputWrapper *wrapper) {
  beatstep_output->stop_all_notes();
  beatstep_output = wrapper;    
}

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
      //Serial.println("First beat of bar and BEATSTEP not started -- starting!");2
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
    
    #ifdef ENABLE_BEATSTEP_SYSEX
      if (is_bpm_on_phrase(ticks+1)) {
        static uint8_t phrase_number = 1;
        //int phrase_number = (ticks+1 / (PPQN*BEATS_PER_BAR*BARS_PER_PHRASE)) % 16;
        phrase_number ++;
        phrase_number %= 16;
        send_preset_change(phrase_number);
        //F0 00 20 6B 7F 42 05 mm F7
        Serial.printf("sending sysex to switch to phrase_number %i?\n", phrase_number);
        uint8_t data[] = {
          0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x05, phrase_number, 0xF7
        };
        midi_beatstep->sendSysEx(sizeof(data), data, true);
      }
    #endif

    //Serial.flush();
  } else {
    //Serial.println("..no midi_beatstep detected!");
    //Serial.flush();
  }
}


// called inside interrupt
void beatstep_on_restart() {
  if (midi_beatstep!=nullptr) {
    Serial.println(F("beatstep_on_restart()"));
    //ATOMIC(
      midi_beatstep->sendRealTime(usbMIDI.Stop); //sendStop();
      midi_beatstep->sendRealTime(usbMIDI.Start); //sendStart();
    //)

    Serial.println(F("beatstep_on_restart done"));
  }
}

void beatstep_handle_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
  current_beatstep_note = note;
  //Serial.printf("beatstep got note on %i\n", note);

  // send incoming notes from beatstep back out to neutron on serial3, but transposed down

  #ifdef ENABLE_BASS_TRANSPOSE
    //if(midi_out_bass_wrapper!=nullptr) {
      /*int note2 = note - 24;
      if (note2<=0) 
        note2 += 12;*/
      uint8_t note2 = note & 12;
      note2 += (bass_transpose_octave*12); //24;
      //Serial.printf("beatstep note on %i : %i : %i\n", BASS_MIDI_CHANNEL, note, velocity);
      //Serial.printf("beatstep note2 is %i\n", note2);
      beatstep_output->sendNoteOn((uint8_t)note2, 127); //, BASS_MIDI_CHANNEL);
    //}
  #endif
}

void beatstep_handle_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (current_beatstep_note==note) 
    current_beatstep_note = -1;
  last_beatstep_note = note;
  //Serial.printf("beatstep got note off %i\n", note);

  #ifdef ENABLE_BASS_TRANSPOSE
    //if(midi_out_bass_wrapper!=nullptr) {
      /*int note2 = note - 24;
      if (note2<=0) 
        note2 += 12;*/
      uint8_t note2 = note & 12;
      note2 += (bass_transpose_octave*12);// note2 += 24;
      beatstep_output->sendNoteOff((uint8_t)note2, velocity); //, BASS_MIDI_CHANNEL);
    //}
  #endif
}

#ifdef ENABLE_BEATSTEP_SYSEX
  void beatstep_handle_sysex (const uint8_t *data, uint16_t length, bool complete) {
    Serial.printf("beatstep_handle received sysx length %i: ", length);
    for (int i = 0 ; i < length ; i++) {
      Serial.printf("%2x ",data[i]);
    }
    if (complete) {
      Serial.println (" (complete)\n");
    } else {
      Serial.println (" (incomplete)\n");
    }
    //Serial.println("");
  }

  void send_preset_change(uint8_t preset) {
      static uint8_t phrase_number = 0;
      //int phrase_number = (ticks+1 / (PPQN*BEATS_PER_BAR*BARS_PER_PHRASE)) % 16;
      phrase_number ++;
      phrase_number %= 16;
      //F0 00 20 6B 7F 42 05 mm F7
      Serial.printf("sending sysex to switch to phrase_number %i?\n", phrase_number);
      uint8_t data[] = {
        0xF0, /*0x00,*/ 0x20, 0x6B, 0x7F, 0x42, 0x05, phrase_number, 0xF7
        ///*0xF0, 0x00,*/  0x20,  0x6B,  0x7F,  0x42,  0x02,  0x00,  0x50,  0x04,  phrase_number%3,  //0xF7
      };
      midi_beatstep->sendSysEx(sizeof(data), data, true);

      //uint8_t data2[] = { 0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x01, 0x00, 0x01, 0x70, 0xF7 };
      uint8_t data2[] = { /*0xF0, 0x00,*/ 0x02, 0x6B, 0x7F, 0x42, 0x01, 0x00, 0x06, 0x75 } ;//, 0xF7 };
      Serial.print("sending ");
      for (int i = 0 ; i < sizeof(data2) ; i++) {
        Serial.printf("%2x ", data2[i]);
      }
      midi_beatstep->sendSysEx(sizeof(data2), data2, false);
      Serial.println();
  }
#endif

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

  #ifdef ENABLE_BEATSTEP_SYSEX
    midi_beatstep->setHandleSysEx(beatstep_handle_sysex);
    //midi_beatstep->setHandleSystemExclusive(beatstep_handle_sysex);
  #endif

  Serial.println(F("beatstep_init() finished"));
  //)
}

/*
#ifdef ENABLE_SCREEN
  void beatstep_display_key_status(ST7789_t3 *tft) {
    tft_header(tft, "beatstep:");
    tft->setTextColor(rgb(0xFFFFFF),0);
    tft->setTextSize(2);

    tft->printf("%4s : %4s", get_note_name(last_beatstep_note).c_str(), get_note_name(current_beatstep_note).c_str());
    //Serial.printf("beatstep_display_key_status got %i aka %4s : %4s\n", current_beatstep_note, get_note_name(last_beatstep_note).c_str(), get_note_name(current_beatstep_note).c_str());
  }
#endif
*/