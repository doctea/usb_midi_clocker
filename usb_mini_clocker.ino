// note: as of 2022-02-17, requires https://github.com/felis/USB_Host_Shield_2.0/pull/438 to be applied to the USB_Host_Shield_2.0 library if using Arturia Beatstep, otherwise it won't receive MIDI data or clock!
// proof of concept for syncing multiple USB Midi devices

#include <UHS2-MIDI.h>
#include <usbhub.h>

//#define ENABLE_MIDI_SERIAL_FLUSH 1

USB Usb;
USBHub  Hub1(&Usb);
/*USBH_MIDI  Midi1(&Usb);
USBH_MIDI  Midi2(&Usb);
USBH_MIDI  Midi3(&Usb);*/

//USBH_MIDI *device;

//UHS2MIDI_CREATE_DEFAULT_INSTANCE(&Usb);

UHS2MIDI_CREATE_INSTANCE(&Usb,0,Midi1);
UHS2MIDI_CREATE_INSTANCE(&Usb,0,Midi2);
UHS2MIDI_CREATE_INSTANCE(&Usb,0,Midi3);

unsigned long t1 = millis();
unsigned long ticks = 0;

void handleNoteOn(byte inChannel, byte inNumber, byte inVelocity)
{
  Serial.print("NoteOn\tch");
  Serial.print(inChannel);
  Serial.print("\tNote ");
  Serial.print(inNumber);
  Serial.print("\tvelocity: ");
  Serial.println(inVelocity);

  //MIDI.sendNoteOn(inNumber, random(0,127), inChannel);
  Midi1.sendNoteOn(inNumber, random(0,127), 1);
  Midi2.sendNoteOn(inNumber, random(0,127), 1);
  Midi3.sendNoteOn(inNumber, random(0,127), 1);

}

void handleNoteOff(byte inChannel, byte inNumber, byte inVelocity)
{
  Serial.print("NoteOff\tch");
  Serial.print(inChannel);
  Serial.print("\tNote ");
  Serial.print(inNumber);
  Serial.print("\tvelocity: ");
  Serial.println(inVelocity);

  Midi1.sendNoteOn(inNumber, 0, 1);
  Midi2.sendNoteOn(inNumber, 0, 1);
  Midi3.sendNoteOn(inNumber, 0, 1);
  Midi1.sendNoteOff(inNumber, 0, 1);
  Midi2.sendNoteOff(inNumber, 0, 1);
  Midi3.sendNoteOff(inNumber, 0, 1);

}

void handleControlChange(byte inChannel, byte inNumber, byte inValue) {
  Serial.print("CC ch");
  Serial.print(inChannel);
  Serial.print("\tnum ");
  Serial.print(inNumber);
  Serial.print("\tvalue: ");
  Serial.println(inValue);

  Midi1.sendNoteOn(inValue, 127, 1);
  Midi2.sendNoteOn(inValue, 127, 1);
  Midi3.sendNoteOn(inValue, 127, 1);
}

void onInit() {
  Midi1.sendStart();
  Midi2.sendStart();
  Midi3.sendStart();
}

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  /*MIDI.begin();
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handleControlChange);*/

  Midi1.begin();
  Midi1.setHandleNoteOn(handleNoteOn);
  Midi1.setHandleNoteOff(handleNoteOff);
  Midi1.setHandleControlChange(handleControlChange);

  Midi2.begin();
  Midi2.setHandleNoteOn(handleNoteOn);
  Midi2.setHandleNoteOff(handleNoteOff);
  Midi2.setHandleControlChange(handleControlChange);

  Midi3.begin();
  Midi3.setHandleNoteOn(handleNoteOn);
  Midi3.setHandleNoteOff(handleNoteOff);
  Midi3.setHandleControlChange(handleControlChange);

  if (Usb.Init() == -1) {
    while (1); //halt
  }//if (Usb.Init() == -1...
  delay( 200 );
  
  Serial.println("Arduino ready.");
}

byte counter = 0;

unsigned int started_at = 0;
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

  if ((millis() - t1) > 20)
  {
    Midi1.sendClock();
    Midi2.sendClock();
    Midi3.sendClock();
    t1 = millis();
    ticks++;
    //Serial.println("tick!");

    //MIDI.sendNoteOn(random(0,127), random(0,6), random(1,16));
    //MIDI.sendNoteOff(random(0,127), random(0,6), random(1,16));
    /*MIDI.sendNoteOn(counter, 0, 1);
    counter = (counter+1)%8;
    MIDI.sendNoteOn(counter, 1, 1);*/
   
    if (ticks%24==0) {
      Midi1.sendNoteOn(counter, 0, 1);
      Midi2.sendNoteOn(counter, 0, 1);
      Midi3.sendNoteOn(counter, 0, 1);    
      counter = (counter+1)%8;
      Midi1.sendNoteOn(counter, 1, 1);
      Midi2.sendNoteOn(counter, 1, 1);
      Midi3.sendNoteOn(counter, 1, 1);
    }
  }
}
