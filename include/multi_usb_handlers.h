#include "bpm.h"
/*
BEATSTEP:   Initialised device vendorid: 7285   productid: 518
BAMBLEWEENY:  Initialised device vendorid: 10374    productid: 32779
AKAI APCMINI: Initialised device vendorid: 2536   productid: 40
*/

// assign device to port and set appropriate handlers
void setupmidi(uint8_t idx, uint32_t packed_id = 0x0000)
{
  uint16_t vid, pid;
  if (packed_id==0) {
    vid = usbmidilist[idx]->idVendor();
    pid = usbmidilist[idx]->idProduct();
    packed_id = (usbmidilist[idx]->idVendor()<<16) | (usbmidilist[idx]->idProduct());
  } else {
    vid = packed_id >> 16;
    pid = 0x0000FFFF & packed_id;
  }
  Serial.printf("USB Port %d changed from %08X to %08X (now ", usb_midi_connected[idx], packed_id);
  Serial.printf("'%s' '%s')\n", usbmidilist[idx]->manufacturer(), usbmidilist[idx]->product());

  // remove handlers that might already be set on this port -- new ones assigned below thru xxx_init() functions
  usbmidilist[idx]->setHandleNoteOn(nullptr);
  usbmidilist[idx]->setHandleNoteOff(nullptr);
  usbmidilist[idx]->setHandleControlChange(nullptr);
  usbmidilist[idx]->setHandleClock(nullptr);
  usbmidilist[idx]->setHandleStart(nullptr);
  usbmidilist[idx]->setHandleStop(nullptr);

  if (packed_id==0) {
    usb_midi_connected[idx] = 0;
    Serial.printf("Disconnected device on port %i\n", idx);
    return;
  }

#ifdef ENABLE_BEATSTEP
  if ( vid == 0x1c75 && pid == 0x0206 ) {         //is Arturia BeatStep?
    ixBeatStep = idx;
    Serial.printf(F("BeatStep connected on idx %i...\n"),idx);
    midi_beatstep = usbmidilist[idx];
    usb_midi_connected[idx] = packed_id;
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
    usb_midi_connected[idx] = packed_id;
    apcmini_init();
    return;
  }
#endif
#ifdef ENABLE_BAMBLE
  if ( vid == 0x2886 && pid == 0x800B ) {            //is BAMBLE?
    ixBamble = idx;
    Serial.printf(F("BAMBLEWEENY connected on idx %i....\n"),idx);
    midi_bamble = usbmidilist[idx];
    usb_midi_connected[idx] = packed_id;
    bamble_init();
    return;
  }
#endif

  usb_midi_connected[idx] = packed_id;
  Serial.print(F("Detected unknown (or disabled) device vid="));
  Serial.print(vid);
  Serial.print(F(", pid="));
  Serial.println(pid);
}


void update_usb_devices() {
  for (int port=0; port < 8; port++) {
    uint32_t packed_id = (usbmidilist[port]->idVendor()<<16) | (usbmidilist[port]->idProduct());
    //Serial.printf("packed %04X and %04X to %08X\n", usbmidilist[port]->idVendor(),  usbmidilist[port]->idProduct(), packed_id);
    if (usb_midi_connected[port] != packed_id) {
      // device at this port has changed since we last saw it -- ie, disconnection or connection

      // unassign the midi_xxx helper pointers if appropriate
      if (midi_bamble==usbmidilist[port]) {
        Serial.printf("Nulling ixBamble and midi_bamble\n");
        ixBamble = 0xFF;
        midi_bamble = nullptr;
      }
      if (midi_beatstep==usbmidilist[port]) {
        Serial.printf("Nulling ixBeatStep and midi_beatstep\n");
        ixBeatStep = 0xFF;
        midi_beatstep = nullptr;
      }
      if (midi_apcmini==usbmidilist[port]) {
        Serial.printf("Nulling ixAPCmini and midi_apcmini\n");
        ixAPCmini = 0xFF;
        midi_apcmini = nullptr;
      }

      Serial.printf("update_usb_devices: device at port %i is %08X which differs from current %08X!\n", port, usbmidilist[port]->idProduct(), usb_midi_connected[port]);

      // call setupmidi() to assign device to port and set handlers
      setupmidi(port, packed_id);
      Serial.println("-----");
    }
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
  
  send_midi_device_stop_start();

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
