// note: as of 2022-02-17, requires https://github.com/felis/USB_Host_Shield_2.0/pull/438 to be applied to the USB_Host_Shield_2.0 library if using Arturia Beatstep, otherwise it won't receive MIDI data or clock!
// proof of concept for syncing multiple USB Midi devices

#include <UHS2-MIDI.h>
#include <usbhub.h>

int duration = 2;

#define DEBUG_TICKS false

#define PPQN  24

USB Usb;
USBHub  Hub1(&Usb);

UHS2MIDI_CREATE_INSTANCE(&Usb,0,Midi1);
UHS2MIDI_CREATE_INSTANCE(&Usb,0,Midi2);
UHS2MIDI_CREATE_INSTANCE(&Usb,0,Midi3);

// tracking which beat we're on
byte beat_counter = 0;

float bpm_current = 60.0f;
double ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));

unsigned int started_at = 0;

#include "multi_usb_handlers.h"

unsigned long t1 = millis();
unsigned long ticks = 0;

void handleStart() {
  Serial.println("Received START!");
  Midi1.sendStart();
  Midi2.sendStart();
  Midi3.sendStart();
}

void handleNoteOn(byte inChannel, byte inNumber, byte inVelocity)
{
  Serial.print("NoteOn\tch");
  Serial.print(inChannel);
  Serial.print("\tNote ");
  Serial.print(inNumber);
  Serial.print("\tvelocity: ");
  Serial.println(inVelocity);

  //MIDI.sendNoteOn(inNumber, random(0,127), inChannel);
  /*Midi1.sendNoteOn(inNumber, random(0,127), 1);
  Midi2.sendNoteOn(inNumber, random(0,127), 1);
  Midi3.sendNoteOn(inNumber, random(0,127), 1);*/

}

void handleNoteOff(byte inChannel, byte inNumber, byte inVelocity)
{
  Serial.print("NoteOff\tch");
  Serial.print(inChannel);
  Serial.print("\tNote ");
  Serial.print(inNumber);
  Serial.print("\tvelocity: ");
  Serial.println(inVelocity);

  /*Midi1.sendNoteOn(inNumber, 0, 1);
  Midi2.sendNoteOn(inNumber, 0, 1);
  Midi3.sendNoteOn(inNumber, 0, 1);
  Midi1.sendNoteOff(inNumber, 0, 1);
  Midi2.sendNoteOff(inNumber, 0, 1);
  Midi3.sendNoteOff(inNumber, 0, 1);*/

}

void handleControlChange(byte inChannel, byte inNumber, byte inValue) {
  Serial.print("CC ch");
  Serial.print(inChannel);
  Serial.print("\tnum ");
  Serial.print(inNumber);
  Serial.print("\tvalue: ");
  Serial.println(inValue);

  /*if (inNumber==54 && inValue==127) {
    Midi1.sendStart();
    Midi2.sendStart();
    Midi3.sendStart();
  }*/

  /*Midi1.sendNoteOn(inValue, 127, 1);
  Midi2.sendNoteOn(inValue, 127, 1);
  Midi3.sendNoteOn(inValue, 127, 1);*/
}
void setup()
{
  Serial.begin(115200);
  while (!Serial);

  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);

  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);

  /*MIDI.begin();
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handleControlChange);*/

  Midi1.turnThruOff();
  Midi2.turnThruOff();
  Midi3.turnThruOff();

  __uhs2Midi1.attachOnInit(onInit1);
  __uhs2Midi2.attachOnInit(onInit2);
  __uhs2Midi3.attachOnInit(onInit3);

/*
  Midi1.begin();
  //Midi1.setHandleNoteOn(handleNoteOn);
  //Midi1.setHandleNoteOff(handleNoteOff);
  Midi1.setHandleControlChange(handleControlChange);*/
  Midi1.setHandleStart(handle_beatstep_start);

/*  Midi2.begin();
  //Midi2.setHandleNoteOn(handleNoteOn);
  //Midi2.setHandleNoteOff(handleNoteOff);
  Midi2.setHandleControlChange(handleControlChange);*/
  Midi2.setHandleStart(handle_beatstep_start);

/*  Midi3.begin();
  //Midi3.setHandleNoteOn(handleNoteOn);
  //Midi3.setHandleNoteOff(handleNoteOff);
  Midi3.setHandleControlChange(handleControlChange);*/
  Midi3.setHandleStart(handle_beatstep_start);

  if (Usb.Init() == -1) {
    while (1); //halt
  }//if (Usb.Init() == -1...
  delay( 200 );
  
  Serial.println("Arduino ready.");
}

// -----------------------------------------------------------------------------`
//
// -----------------------------------------------------------------------------
void loop()
{
  Usb.Task();
  Midi1.read();
  Midi2.read();
  Midi3.read();

  /*if (millis()-started_at>=10000) {
    Midi1.sendStart();
    Midi2.sendStart();
    Midi3.sendStart();
    started_at = millis();
  }*/

  if ((millis() - t1) > ms_per_tick)
  {
    ticks++;
    /*Midi1.sendClock();
    Midi2.sendClock();
    Midi3.sendClock();*/
    unsigned int delta = millis()-t1;
    if (DEBUG_TICKS) {
      Serial.print(ticks);
      Serial.print(":\tTicked with delta\t");
      Serial.print(delta);
      Serial.print("!\t(ms_per_tick is ");
      Serial.print(ms_per_tick);
      Serial.print(") sending clock for [ ");
    }
    if (midi_beatstep)  {
      midi_beatstep->sendClock();
      if (DEBUG_TICKS) Serial.print(" beatstep ");
    }
    if (midi_bamble)    {
      if (DEBUG_TICKS) Serial.print(" bamble ");
      midi_bamble->sendClock();   
    }
    if (DEBUG_TICKS) Serial.println(" ]");
    //ticks++;
    //Serial.println("tick!");
    t1 = millis();

    //MIDI.sendNoteOn(random(0,127), random(0,6), random(1,16));
    //MIDI.sendNoteOff(random(0,127), random(0,6), random(1,16));
    /*MIDI.sendNoteOn(counter, 0, 1);
    counter = (counter+1)%8;
    MIDI.sendNoteOn(counter, 1, 1);*/

    // start bar (every fourth quarter note)
    if (ticks%(PPQN*4)==0) {

      start_clocks_if_stopped();
      /*Midi1.sendContinue();
      Midi2.sendContinue();
      Midi3.sendContinue();*/
     
      digitalWrite(4, HIGH);
    } else if (ticks%(PPQN*4)==duration) {
      digitalWrite(4, LOW);
    }

    // every two quarter notes
    if (ticks%(PPQN*2)==0) {
      digitalWrite(5, HIGH);
    } else if (ticks%(PPQN*2)==duration) {
      digitalWrite(5, LOW);
    }

    // every quarter note
    if (ticks%(PPQN)==0) {
      digitalWrite(6, HIGH);

      if (midi_apcmini) {
        Serial.print("Ticks are ");
        Serial.print(ticks);
        Serial.print("\tCounter is ");
        Serial.println(beat_counter);
        midi_apcmini->sendNoteOn(beat_counter, 0, 1);
        beat_counter = (beat_counter+1)%8;
        //midi_apcmini->sendNoteOn(counter, 1, 1);
      }
    } else if (ticks%PPQN==duration) {
      digitalWrite(6, LOW);
      if (midi_apcmini) {
        midi_apcmini->sendNoteOn(beat_counter, 1, 1);
      }
    }

    // every sixteenth note
    if (ticks%(PPQN/2)==0) {
      digitalWrite(7, HIGH);
      //digitalWrite(6, LOW);
      //digitalWrite(7, LOW);
    } else if (ticks%(PPQN/2)==duration) {
      digitalWrite(7, LOW);
    }
  }
}
