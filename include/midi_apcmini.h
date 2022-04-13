#ifdef ENABLE_APCMINI

#include "bpm.h"

#define APCMINI_NUM_ROWS      8
#define APCMINI_DISPLAY_WIDTH 8

#define APCMINI_BUTTON_CLIP_STOP 82
#define APCMINI_BUTTON_SOLO      83
#define APCMINI_BUTTON_REC_ARM   84
#define APCMINI_BUTTON_MUTE      85
#define APCMINI_BUTTON_SELECT    86
#define APCMINI_BUTTON_UNLABELED_1    87
#define APCMINI_BUTTON_UNLABELED_2    88
#define APCMINI_BUTTON_STOP_ALL_CLIPS 89
#define APCMINI_BUTTON_SHIFT     98
#define APCMINI_BUTTON_UP        64
#define APCMINI_BUTTON_DOWN      65
#define APCMINI_BUTTON_LEFT      66
#define APCMINI_BUTTON_RIGHT     67
#define APCMINI_BUTTON_VOLUME    68
#define APCMINI_BUTTON_PAN       69
#define APCMINI_BUTTON_SEND      70
#define APCMINI_BUTTON_DEVICE    71

//#define START_BEAT_INDICATOR 0
#define START_BEAT_INDICATOR  APCMINI_BUTTON_UP

// restart buttons mapping - these are shifted
#define BUTTON_RESTART_IMMEDIATELY    APCMINI_BUTTON_UP
#define BUTTON_RESTART_AT_END_OF_BAR  APCMINI_BUTTON_DEVICE

//MIDI_NAMESPACE::MidiInterface<UHS2MIDI_NAMESPACE::uhs2MidiTransport> *midi_apcmini;
MIDIDevice *midi_apcmini;
volatile uint8_t ixAPCmini  = 0xff;

volatile bool apcmini_started = false;
bool apcmini_shift_held = false;

#ifdef ENABLE_CLOCKS
byte clock_selected = 0;
#endif

bool redraw_immediately = false;
unsigned long last_updated_display = 0;

#ifdef ENABLE_APCMINI_DISPLAY
void apcmini_update_clock_display();
void apcmini_update_position_display(int ticks);
void apcmini_clear_display();
#endif

inline void apcmini_loop() {
  if ( ixAPCmini != 0xff) {
    //ATOMIC(
      while (usbmidilist[ixAPCmini]->read());
      /*do {
        usbmidilist[ixAPCmini]->read();
        //Serial.println(F("Read from apcmini in apcmini_loop!"));
      } while ( usbmidilist[ixAPCmini]->getTransport()->available() > 0);*/
    //)
  } else {
    return;
  }

#ifdef ENABLE_APCMINI_DISPLAY
  static unsigned long last_processed_tick;

  if (last_processed_tick!=ticks) {
    apcmini_update_position_display(ticks);
 
    if (midi_apcmini && (redraw_immediately || millis() - last_updated_display > 50)) { // || ticks - last_updated_display > PPQN) {
      //Serial.println(F("redraw_immediately is set!"));
      apcmini_update_clock_display();
      redraw_immediately = false;
    }
    //ATOMIC(
      last_processed_tick = ticks;
    //)
  }
#endif
  //Serial.println(F("finished apcmini_loop"));
}

#ifdef ENABLE_APCMINI_DISPLAY
#include "midi_apcmini_display.h"
#endif


// called from loop, already inside ATOMIC, so don't use ATOMIC here
void apcmini_note_on(byte inChannel, byte inNumber, byte inVelocity) {
  Serial.printf("apcmini_note_on for %i, %i, %i\n", inChannel, inNumber, inVelocity);
  if (inNumber==APCMINI_BUTTON_STOP_ALL_CLIPS) {
    // start / stop play
    if (!playing)
      on_restart();
      
    playing = !playing;
    
#ifdef USE_UCLOCK
    if (playing) {
      on_restart();
      uClock.start();
    } else
      uClock.stop();
#endif
  } else if (inNumber==BUTTON_RESTART_IMMEDIATELY && apcmini_shift_held) { // up pressed with shift
    // restart/resync immediately
    Serial.println(F("APCmini pressed, restarting downbeat"));
    on_restart();
  } else if (inNumber==BUTTON_RESTART_AT_END_OF_BAR && apcmini_shift_held) {
    // restart/resync at end of bar
    Serial.println(F("APCmini pressed, restarting downbeat on next bar"));
#ifdef ENABLE_APCMINI_DISPLAY
    //ATOMIC(
      midi_apcmini->sendNoteOn(7, APCMINI_GREEN_BLINK, 1);
    //)  // turn on the 'going to restart on next bar' flashing indicator
#endif
    restart_on_next_bar = true;
#ifdef ENABLE_CLOCKS
  } else if (inNumber==APCMINI_BUTTON_UP) {
    // move clock selection up
    byte old_clock_selected  = clock_selected;
    //redraw_immediately = true;
    if (clock_selected==0)
      clock_selected = NUM_CLOCKS-1;
    else
      clock_selected--;
#ifdef ENABLE_APCMINI_DISPLAY
    redraw_clock_selected(old_clock_selected, clock_selected);
#endif
  } else if (inNumber==APCMINI_BUTTON_DOWN) {
    // move clock selection down
    byte old_clock_selected  = clock_selected;
    //redraw_immediately = true;
    clock_selected++;
    if (clock_selected>=NUM_CLOCKS) 
      clock_selected = 0;
#ifdef ENABLE_APCMINI_DISPLAY
    redraw_clock_selected(old_clock_selected, clock_selected);
#endif
  } else if (inNumber==APCMINI_BUTTON_LEFT) {
    // shift clock offset left
    redraw_immediately = true;
    decrease_clock_delay(clock_selected);
    //redraw_immediately = true;
#ifdef ENABLE_APCMINI_DISPLAY
    redraw_clock_row(clock_selected);
#endif
  } else if (inNumber==APCMINI_BUTTON_RIGHT) {
    // shift clock offset right
    redraw_immediately = true;
    increase_clock_delay(clock_selected);
#ifdef ENABLE_APCMINI_DISPLAY
    redraw_clock_row(clock_selected);
#endif
  } else if (inNumber>=APCMINI_BUTTON_CLIP_STOP && inNumber<=APCMINI_BUTTON_MUTE) {
    // button between Clip Stop -> Solo -> Rec arm -> Mute buttons
    // change divisions/multiplier of corresponding clock
    byte clock_number = inNumber - APCMINI_BUTTON_CLIP_STOP;  
    byte old_clock_selected = clock_selected;
    clock_selected = clock_number;
    
    if (apcmini_shift_held) {
      increase_clock_multiplier(clock_number);
      //clock_multiplier[clock_number] *= 2;   // double the selected clock multiplier -> more pulses
    } else {
      decrease_clock_multiplier(clock_number);
      //clock_multiplier[clock_number] /= 2;   // halve the selected clock multiplier -> fewer pulses
    }
    
    /*if (clock_multiplier[clock_number]>CLOCK_MULTIPLIER_MAX)
      clock_multiplier[clock_number] = CLOCK_MULTIPLIER_MIN;
    else if (clock_multiplier[clock_number]<CLOCK_MULTIPLIER_MIN) 
      clock_multiplier[clock_number] = CLOCK_MULTIPLIER_MAX;*/

#ifdef ENABLE_APCMINI_DISPLAY
    redraw_clock_row(clock_selected);
    redraw_clock_selected(old_clock_selected, clock_selected);
#endif
#endif
  } else if (inNumber==APCMINI_BUTTON_SHIFT) {
    apcmini_shift_held = true;
/*  } else if (inNumber==APCMINI_BUTTON_UNLABELED_1) {
    // for debugging -- single-step through a tick
    single_step = true;
    //ticks += 1;
    Serial.print(F("Single-stepped to tick "));
    Serial.println(ticks);*/
  } else if (apcmini_shift_held && inNumber==APCMINI_BUTTON_UNLABELED_1) {
#ifdef ENABLE_APCMINI_DISPLAY
    apcmini_clear_display();
    /*for (int i = 0 ; i < 64 ; i++) {
      if (midi_apcmini) {
        midi_apcmini->sendNoteOn(i, random(0,6), 1);
      }
    }
    apcmini_clear_display();*/
#endif
    load_state(0, &current_state);
  } else if (apcmini_shift_held && inNumber==APCMINI_BUTTON_UNLABELED_2) {
    save_state(0, &current_state);
    Serial.println("---- debug");
    for (int i = 0 ; i < 8 ; i++) {
      Serial.printf("usbmidilist[%i] is %04X:%04X aka %s:%s\n", i, usbmidilist[i]->idVendor(), usbmidilist[i]->idProduct(), usbmidilist[i]->manufacturer(), usbmidilist[i]->product() );
    }
    Serial.println("---- debug");
#ifdef ENABLE_SEQUENCER
  } else if (inNumber>=0 && inNumber < NUM_SEQUENCES * APCMINI_DISPLAY_WIDTH) {
    byte row = 3 - (inNumber / APCMINI_DISPLAY_WIDTH);
    Serial.print(F("For inNumber "));
    Serial.print(inNumber);
    Serial.print(F(" got row (ie clock) "));
    Serial.print(row);
    Serial.print(F(" and column "));
    byte col = inNumber - ((3-row)*APCMINI_DISPLAY_WIDTH);
    Serial.println(col);
    sequencer_press(row, col, apcmini_shift_held);
#ifdef ENABLE_APCMINI_DISPLAY
    redraw_sequence_row(row);
#endif
#endif 
  } else {
    Serial.print(F("Unknown akaiAPC button with note number "));
    Serial.println(inNumber);//if (inNumber<(8*8) && inNumber>=(8*5)) {
  }

  if (redraw_immediately) 
    last_updated_display = 0;
}

// called from loop, already inside ATOMIC, so don't use ATOMIC here
void apcmini_note_off(byte inChannel, byte inNumber, byte inVelocity) {
  if (inNumber==APCMINI_BUTTON_SHIFT) {
    apcmini_shift_held = false;
  }
}

// called from loop, already inside ATOMIC, so don't use ATOMIC here
void apcmini_control_change (byte inChannel, byte inNumber, byte inValue) {
  //ATOMIC(
    /*Serial.print(F("APCMINI CC ch"));
    Serial.print(inChannel);
    Serial.print(F("\tnum "));
    Serial.print(inNumber);
    Serial.print(F("\tvalue: "));
    Serial.println(inValue);*/
  //)
  debug_free_ram();

#ifdef ENABLE_BPM
  if (inNumber==56) {   // 56 == "master" fader set bpm 
    set_bpm(map(inValue, 0, 127, BPM_MINIMUM, BPM_MAXIMUM)); // scale CC value
  }
#endif
}

// called inside interrupt
void apcmini_on_tick(volatile uint32_t ticks) {
  if (midi_apcmini) {
    //ATOMIC(
      //midi_apcmini->sendRealTime(usbMIDI.Clock); // sendRealTime(MIDI);
    //)
  }
}

// called inside interrupt
void apcmini_on_restart() {
  if (midi_apcmini) {
    //ATOMIC(
      //midi_apcmini->sendRealTime(usbMIDI.Stop); //sendStop();
      //midi_apcmini->sendRealTime(usbMIDI.Start);
    //)
    //ATOMIC(
      midi_apcmini->sendNoteOn(7, APCMINI_OFF, 1);  // turn off the flashing 'going to restart on next bar' indicator
    //)
  }
}


void apcmini_init() {
    //midi_apcmini->turnThruOff();
    Serial.println("apcmini_init()");
    midi_apcmini->setHandleControlChange(apcmini_control_change);
    midi_apcmini->setHandleNoteOn(apcmini_note_on);
    midi_apcmini->setHandleNoteOff(apcmini_note_off);
#ifdef ENABLE_APCMINI_DISPLAY
    apcmini_clear_display();
    redraw_immediately = true;
#endif
}

#endif
