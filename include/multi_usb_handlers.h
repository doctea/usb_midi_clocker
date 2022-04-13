#include "bpm.h"
/*
BEATSTEP:   Initialised device vendorid: 7285   productid: 518
BAMBLEWEENY:  Initialised device vendorid: 10374    productid: 32779
AKAI APCMINI: Initialised device vendorid: 2536   productid: 40
*/


void setupmidi(uint8_t idx)
{
  uint16_t vid = usbmidilist[idx]->idVendor();
  uint16_t pid = usbmidilist[idx]->idProduct();
  Serial.printf("Previously unknown device detected on usbmidi port %d:vid%04X,pid%04X - ",  idx, vid, pid);
  Serial.printf("%s - %s\n", usbmidilist[idx]->manufacturer(), usbmidilist[idx]->product());
  Serial.printf("MIDIDevice is at address &%i\n", &usbmidilist[idx]);
  Serial.printf("MIDIDevice is at address  %i\n", usbmidilist[idx]);

#ifdef ENABLE_BEATSTEP
  if ( vid == 0x1c75 && pid == 0x0206 ) {         //is Arturia BeatStep?
    ixBeatStep = idx;
    Serial.printf(F("BeatStep connected on idx %i...\n"),idx);
    midi_beatstep = usbmidilist[idx];
    usb_midi_connected[idx] = pid;
    beatstep_init();
    Serial.println(F("completed Beatstep init"));
    return;
  } 
#endif
#ifdef ENABLE_APCMINI
  if ( vid == 0x09e8 && pid == 0x0028 ) {   //is AKAI APCmini?
    ixAPCmini = idx;
    Serial.printf(F("AKAI APCmini connected on idx %i...\n"),idx);
    midi_apcmini = usbmidilist[idx];
    usb_midi_connected[idx] = pid;
    apcmini_init();
    return;
  }
#endif
#ifdef ENABLE_BAMBLE
  if ( vid == 0x2886 && pid == 0x800B ) {            //is BAMBLE?
    ixBamble = idx;
    Serial.printf(F("BAMBLEWEENY connected on idx %i....\n"),idx);
    midi_bamble = usbmidilist[idx];
    usb_midi_connected[idx] = pid;
    bamble_init();
    return;
  }
#endif

  Serial.print(F("Detected unknown (or disabled) device vid="));
  Serial.print(vid);
  Serial.print(F(", pid="));
  Serial.println(pid);
}


void update_usb_devices() {
  for (int port=0; port < 8; port++) {
    //Serial.printf("port %i is already %04X\n", port, usb_midi_connected[port]);
    //if (usb_midi_connected[port] != (((uint64_t)usbmidilist[port]->idVendor()<<16) | ((uint64_t)usbmidilist[port]->idProduct()))) {
    if (usb_midi_connected[port] != usbmidilist[port]->idProduct()) {
      Serial.printf("update_usb_devices: port %i value %04X differs from current %04X!\n", 
        port,
        //(((uint64_t)usbmidilist[port]->idVendor()<<16) | ((uint64_t)usbmidilist[port]->idProduct()))
        usbmidilist[port]->idProduct(),
        usb_midi_connected[port]
      );
      //usb_midi_connected[port] = ((uint64_t)usbmidilist[port]->idVendor()<<16) | ((uint64_t)usbmidilist[port]->idProduct());
      Serial.printf("Setting %i to %04X\n", port, usbmidilist[port]->idProduct());
      usb_midi_connected[port] = usbmidilist[port]->idProduct();
      Serial.printf("is now %04X\n", usb_midi_connected[port]);
      setupmidi(port);
      Serial.printf("and after setupmidi, is now %04X\n", usb_midi_connected[port]);
      Serial.println("-----");
      
      continue;

      if (!usb_midi_connected[port]) {
        Serial.printf("Received data from uninitalised port %i with ids %04x:%04x...\n", port, usbmidilist[port]->idVendor(), usbmidilist[port]->idProduct());
        setupmidi(port);
        usb_midi_connected[port] = true;
        Serial.printf("...Finished setupmidi for port %i\n", port);

        /*uint8_t type =       usbmidilist[port]->getType();
        uint8_t data1 =      usbmidilist[port]->getData1();
        uint8_t data2 =      usbmidilist[port]->getData2();
        uint8_t channel =    usbmidilist[port]->getChannel();
        const uint8_t *sys = usbmidilist[port]->getSysExArray();*/
        //sendToComputer(type, data1, data2, channel, sys, 8 + port);
        //activity = true;
      }
    }
    //while (usbmidilist[port]->read());
  }
}

// call this when global clock should be reset
void on_restart() {
  restart_on_next_bar = false;

  Serial.println(F("on_restart()==>"));
  
  Serial.println(F("reset ticks"));
  // TODO: cheapclock version
#ifdef USE_UCLOCK
  uClock.setTempo(bpm_current); // todo: probably not needed?
  Serial.println(F("reset tempo"));
  uClock.resetCounters();
  Serial.println(F("reset counters"));
#else
  ticks = 0;
#endif
  
#ifdef ENABLE_BEATSTEP
  Serial.print(F("restart beatstep..."));
  beatstep_on_restart();
  Serial.println(F("restarted"));
#endif
#ifdef ENABLE_BAMBLE
  Serial.print(F("restart bamble..."));
  bamble_on_restart();
  Serial.println(F("restarted"));
#endif
#ifdef ENABLE_APCMINI
  Serial.print(F("restart apcmini..."));
  apcmini_on_restart();
  Serial.println(F("restarted"));
  redraw_immediately = true;
#endif
  Serial.println(F("<==on_restart()"));
}

/*
void onInit1() {
  setupmidi(0);
}

void onInit2() {
  setupmidi(1);
}

void onInit3() {
  setupmidi(2);
}*/

void setup_multi_usb() {
  Serial.println(F("Arduino initialising usb/midi..."));

  /*__uhs2Midi1.attachOnInit(onInit1);
  Midi1.turnThruOff();
  Midi1.begin(MIDI_CHANNEL_OMNI);

  __uhs2Midi2.attachOnInit(onInit2);
  Midi2.turnThruOff();
  Midi2.begin(MIDI_CHANNEL_OMNI);

  __uhs2Midi3.attachOnInit(onInit3);
  Midi3.turnThruOff();
  Midi3.begin(MIDI_CHANNEL_OMNI);

  if (Usb.Init() == -1) {
    while (1); //halt
  }//if (Usb.Init() == -1...
  Serial.println(F("USB ready."));*/

/*
  MIDI1.begin(MIDI_CHANNEL_OMNI);
  MIDI2.begin(MIDI_CHANNEL_OMNI);
  MIDI3.begin(MIDI_CHANNEL_OMNI);
  MIDI4.begin(MIDI_CHANNEL_OMNI);
  MIDI5.begin(MIDI_CHANNEL_OMNI);
  MIDI6.begin(MIDI_CHANNEL_OMNI);
  MIDI7.begin(MIDI_CHANNEL_OMNI);
  MIDI8.begin(MIDI_CHANNEL_OMNI);  
  MIDI1.turnThruOff();
  MIDI2.turnThruOff();
  MIDI3.turnThruOff();
  MIDI4.turnThruOff();
  MIDI5.turnThruOff();
  MIDI6.turnThruOff();
  MIDI7.turnThruOff();
  MIDI8.turnThruOff();
*/
  Usb.begin();  
  delay(5000);
}

/*
void start_clocks_if_stopped() {
  if (!beatstep_started && midi_beatstep) {
    Serial.println("BEATSTEP not started -- starting!");
    midi_beatstep->sendStart();
    beatstep_started = true;
  }
  if (!bamble_started && midi_bamble) {
    midi_bamble->sendStart();
    bamble_started = true;
  }
  if (!apcmini_started && midi_apcmini) {
    apcmini_started = true;
  }
}
*/
