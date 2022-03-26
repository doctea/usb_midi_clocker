// note: as of 2022-02-17, requires https://github.com/felis/USB_Host_Shield_2.0/pull/438 to be applied to the USB_Host_Shield_2.0 library if using Arturia Beatstep, otherwise it won't receive MIDI data or clock!
// proof of concept for syncing multiple USB Midi devices

#define ENABLE_APCMINI
#define ENABLE_BEATSTEP
#define ENABLE_BAMBLE

#define ENABLE_APCMINI_DISPLAY
#define ENABLE_BPM
//#define ENABLE_SEQUENCER
//#define DEBUG_TICKS

#include <UHS2-MIDI.h>
#include <usbhub.h>

void do_tick(uint32_t ticks);

#include <uClock.h>

int duration = 2;

USB Usb;
USBHub  Hub1(&Usb);
//USBHub  Hub2(&Usb);

#define NUMBER_OF_DEVICES 3
UHS2MIDI_CREATE_INSTANCE(&Usb, 0, Midi1);
UHS2MIDI_CREATE_INSTANCE(&Usb, 0, Midi2);
UHS2MIDI_CREATE_INSTANCE(&Usb, 0, Midi3);

// The Midi[] array holds the MidiInterface objects (that you can call sendNoteOn(), sendClock(), setHandleNoteOn() etc on)
MIDI_NAMESPACE::MidiInterface<UHS2MIDI_NAMESPACE::uhs2MidiTransport> *Midi[] {&Midi1, &Midi2, &Midi3};
//The instance name of uhs2MidiTransport is prefixed with __uhs2.  Can also fetch this from eg Midi1->getTransport()
UHS2MIDI_NAMESPACE::uhs2MidiTransport *MidiTransports[] {&__uhs2Midi1, &__uhs2Midi2, &__uhs2Midi3};

void on_restart();

#include "bpm.h"
#include "clock.h"

#ifdef ENABLE_SEQUENCER
#include "sequencer.h"
#endif
#include "cv_outs.h"

#include "midi_beatstep.h"
#include "midi_apcmini.h"
#include "midi_bamble.h"

#include "multi_usb_handlers.h"

void onInit1() {
  setupmidi(0);
}

void onInit2() {
  setupmidi(1);
}

void onInit3() {
  setupmidi(2);
}

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  Serial.println(F("Arduino initialising usb/midi..."));

  __uhs2Midi1.attachOnInit(onInit1);
  Midi1.turnThruOff();
  Midi1.begin(MIDI_CHANNEL_OMNI);

  __uhs2Midi2.attachOnInit(onInit2);
  Midi2.turnThruOff();
  Midi2.begin(MIDI_CHANNEL_OMNI);

  __uhs2Midi3.attachOnInit(onInit3);
  Midi3.turnThruOff();
  Midi3.begin(MIDI_CHANNEL_OMNI);

  pinMode(PIN_CLOCK_1, OUTPUT);
  pinMode(PIN_CLOCK_2, OUTPUT);

  pinMode(PIN_CLOCK_3, OUTPUT);
  pinMode(PIN_CLOCK_4, OUTPUT);

  if (Usb.Init() == -1) {
    while (1); //halt
  }//if (Usb.Init() == -1...
  Serial.println(F("USB ready."));
  delay( 1000 );
  
  Serial.println(F("Arduino ready."));

#ifdef ENABLE_SEQUENCER
  init_sequence();
#endif

  Serial.println(F("Initialising uClock.."));
  setup_uclock();

  Serial.println(F("Arduino ready."));
}

long loop_counter = 0;

// -----------------------------------------------------------------------------`
//
// -----------------------------------------------------------------------------
void loop()
{
  //if (loop_counter%100==0) Serial.println(F("100th loop()"));
  ATOMIC(
    Usb.Task();
  )

  //unsigned long ticks;
  //uClock.getTick(&ticks);

#ifdef ENABLE_BEATSTEP
  //ATOMIC(
  //Serial.println("==>doing beatstep_loop()");
    beatstep_loop();
  //Serial.println("<==done beatstep_loop()!");
  //)
#endif

#ifdef ENABLE_APCMINI
  //ATOMIC(
    apcmini_loop();
  //)
#endif

#ifdef ENABLE_BAMBLE
  //ATOMIC(
    bamble_loop();
  //)
#endif

  //Serial.println(F("."));
  /*if (!playing && single_step) {
    do_tick(ticks);
  }*/
  if (loop_counter%1000==0) Serial.println(F("main loop() - 1000 loops passed"));
  loop_counter++;
}

// called inside interrupt
void do_tick(uint32_t in_ticks) {  
#ifdef DEBUG_TICKS
    unsigned int delta = millis()-t1;

    Serial.print(ticks);
    Serial.print(F(":\tTicked with delta\t"));
    Serial.print(delta);
    Serial.print(F("!\t(ms_per_tick is "));
    Serial.print(ms_per_tick);
    Serial.print(F(") sending clock for [ "));
#endif

    ticks = in_ticks;
    
    if (restart_on_next_bar && is_bpm_on_bar(in_ticks)) {
      //in_ticks = ticks = 0;
      on_restart();
      //ATOMIC(
        //midi_apcmini->sendNoteOn(7, APCMINI_OFF, 1);
      //)
      restart_on_next_bar = false;
    }

  //ATOMIC(
    update_cv_outs(in_ticks);

    //Serial.print(F("about to beatstep_on_tick for "));
    //Serial.println(ticks);
#ifdef ENABLE_BEATSTEP
    beatstep_on_tick(in_ticks);
#endif
    //Serial.println(F("finished beatstep_on_tick!"));
#ifdef ENABLE_BAMBLE
    bamble_on_tick(in_ticks);
#endif

#ifdef ENABLE_APCMINI
    apcmini_on_tick(in_ticks);
#endif
  //)

#ifdef DEBUG_TICKS
  Serial.println(F(" ]"));
#endif 

  //ticks++;
  //t1 = millis();
  //single_step = false;
}
