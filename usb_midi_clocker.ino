// note: as of 2022-02-17, requires https://github.com/felis/USB_Host_Shield_2.0/pull/438 to be applied to the USB_Host_Shield_2.0 library if using Arturia Beatstep, otherwise it won't receive MIDI data or clock!
// proof of concept for syncing multiple USB Midi devices

#include <UHS2-MIDI.h>
#include <usbhub.h>

int duration = 2;

#define DEBUG_TICKS true

USB Usb;
USBHub  Hub1(&Usb);

#define NUMBER_OF_DEVICES 3
UHS2MIDI_CREATE_INSTANCE(&Usb, 0, Midi1);
UHS2MIDI_CREATE_INSTANCE(&Usb, 0, Midi2);
UHS2MIDI_CREATE_INSTANCE(&Usb, 0, Midi3);

// The Midi[] array holds the MidiInterface objects (that you can call sendNoteOn(), sendClock(), setHandleNoteOn() etc on)
MIDI_NAMESPACE::MidiInterface<UHS2MIDI_NAMESPACE::uhs2MidiTransport> *Midi[] {&Midi1, &Midi2, &Midi3};
//The instance name of uhs2MidiTransport is prefixed with __uhs2.
UHS2MIDI_NAMESPACE::uhs2MidiTransport *MidiTransports[] {&__uhs2Midi1, &__uhs2Midi2, &__uhs2Midi3};

void on_restart();

#include "bpm.h"

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
  delay( 200 );
  
  Serial.println(F("Arduino ready."));
}

// -----------------------------------------------------------------------------`
//
// -----------------------------------------------------------------------------
void loop()
{
  Usb.Task();

  beatstep_loop();
  apcmini_loop();
  bamble_loop();

  if ((millis() - t1) > ms_per_tick)
  {
    unsigned int delta = millis()-t1;
    
    if (DEBUG_TICKS) {
      Serial.print(ticks);
      Serial.print(F(":\tTicked with delta\t"));
      Serial.print(delta);
      Serial.print(F("!\t(ms_per_tick is "));
      Serial.print(ms_per_tick);
      Serial.print(F(") sending clock for [ "));
    }

    update_cv_outs(ticks);
    
    beatstep_on_tick(ticks);
    bamble_on_tick(ticks);
    apcmini_on_tick(ticks);
        
    if (DEBUG_TICKS) Serial.println(F(" ]"));

    ticks++;
    t1 = millis();
  }
}
