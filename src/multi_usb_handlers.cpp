#include "bpm.h"
#include "midi_outs.h"

#include "midi_bamble.h"
#include "midi_beatstep.h"

/*
usb_midi_device[0] is 1C75:0288 aka Arturia:Arturia KeyStep 32
usb_midi_device[1] is 2886:800B aka The Tyrell Corporation:Bambleweeny57
usb_midi_device[2] is 1C75:0206 aka Arturia:Arturia BeatStep
usb_midi_device[3] is 09E8:0028 aka AKAI PROFESSIONAL,LP:APC MINI       
usb_midi_device[4] is 09E8:006B aka Akai:Akai MPK49
*/

#define NUM_USB_DEVICES 8

// assign device to port and set appropriate handlers
void setupmidi(uint8_t idx, uint32_t packed_id = 0x0000) {
  uint16_t vid, pid;
  if (packed_id==0) {
    vid = usb_midi_device[idx]->idVendor();
    pid = usb_midi_device[idx]->idProduct();
    packed_id = (usb_midi_device[idx]->idVendor()<<16) | (usb_midi_device[idx]->idProduct());
  } else {
    vid = packed_id >> 16;
    pid = 0x0000FFFF & packed_id;
  }
  Serial.printf("USB Port %d changed from %08X to %08X (now ", idx, usb_midi_connected[idx], packed_id);
  Serial.printf("'%s' '%s')\n", usb_midi_device[idx]->manufacturer(), usb_midi_device[idx]->product());

  // remove handlers that might already be set on this port -- new ones assigned below thru xxx_init() functions
  usb_midi_device[idx]->setHandleNoteOn(nullptr);
  usb_midi_device[idx]->setHandleNoteOff(nullptr);
  usb_midi_device[idx]->setHandleControlChange(nullptr);
  usb_midi_device[idx]->setHandleClock(nullptr);
  usb_midi_device[idx]->setHandleStart(nullptr);
  usb_midi_device[idx]->setHandleStop(nullptr);

  if (packed_id==0) {
    usb_midi_connected[idx] = 0;
    Serial.printf("Disconnected device on port %i\n", idx);
    return;
  }

#ifdef ENABLE_BEATSTEP
  if ( vid == 0x1c75 && pid == 0x0206 ) {         //is Arturia BeatStep?
    ixBeatStep = idx;
    Serial.printf(F("BeatStep connected on idx %i...\n"),idx);
    midi_beatstep = usb_midi_device[idx];
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
    midi_apcmini = usb_midi_device[idx];
    usb_midi_connected[idx] = packed_id;
    apcmini_init();
    return;
  }
#endif
#ifdef ENABLE_BAMBLE
  if ( vid == 0x2886 && pid == 0x800B ) {            //is BAMBLE?
    ixBamble = idx;
    Serial.printf(F("BAMBLEWEENY connected on idx %i....\n"),idx);
    midi_bamble = usb_midi_device[idx];
    usb_midi_connected[idx] = packed_id;
    bamble_init();
    return;
  }
#endif
#ifdef ENABLE_MPK49
  if (vid == 0x09E8 && pid== 0x006B) {
    ixMPK49 = idx;
    Serial.printf(F("MPK49 connected on idx %i....\n"),idx);
    midi_MPK49 = usb_midi_device[idx];
    usb_midi_connected[idx] = packed_id;
    MPK49_init();
    return;
  }
#endif
#ifdef ENABLE_KEYSTEP
  if (vid == 0x1C75 && pid== 0x0288) {
    ixKeystep = idx;
    Serial.printf(F("Keystep connected on idx %i....\n"),idx);
    midi_keystep = usb_midi_device[idx];
    usb_midi_connected[idx] = packed_id;
    keystep_init();
    return;
  }
#endif

  usb_midi_connected[idx] = packed_id;
  Serial.print(F("Detected unknown (or disabled) device vid="));
  Serial.print(vid);
  Serial.print(F(", pid="));
  Serial.println(pid);
}


void update_usb_device_connections() {
  for (int port=0; port < NUM_USB_DEVICES; port++) {
    uint32_t packed_id = (usb_midi_device[port]->idVendor()<<16) | (usb_midi_device[port]->idProduct());
    //Serial.printf("packed %04X and %04X to %08X\n", usb_midi_device[port]->idVendor(),  usb_midi_device[port]->idProduct(), packed_id);
    if (usb_midi_connected[port] != packed_id) {
      // device at this port has changed since we last saw it -- ie, disconnection or connection
      // unassign the midi_xxx helper pointers if appropriate
      #ifdef ENABLE_BAMBLE
      if (midi_bamble==usb_midi_device[port]) {
        Serial.printf("Nulling ixBamble and midi_bamble\n");
        ixBamble = 0xFF;
        midi_bamble = nullptr;
      }
      #endif
      #ifdef ENABLE_BEATSTEP
      if (midi_beatstep==usb_midi_device[port]) {
        Serial.printf("Nulling ixBeatStep and midi_beatstep\n");
        ixBeatStep = 0xFF;
        midi_beatstep = nullptr;
      }
      #endif
      #ifdef ENABLE_APCMINI
      if (midi_apcmini==usb_midi_device[port]) {
        Serial.printf("Nulling ixAPCmini and midi_apcmini\n");
        ixAPCmini = 0xFF;
        midi_apcmini = nullptr;
      }
      #endif
      #ifdef ENABLE_MPK49
      if (midi_MPK49==usb_midi_device[port]) {
        Serial.printf("Nulling ixMPK49 and midi_MPK49\n");
        ixMPK49 = 0xFF;
        midi_MPK49 = nullptr;
      }
      #endif
      #ifdef ENABLE_KEYSTEP
      if (midi_keystep==usb_midi_device[port]) {
        Serial.printf("Nulling ixKeystep and midi_keystep\n");
        ixKeystep = 0xFF;
        midi_keystep = nullptr;
      }
      #endif

      Serial.printf("update_usb_device_connections: device at port %i is %08X which differs from current %08X!\n", port, packed_id, usb_midi_connected[port]);

      // call setupmidi() to assign device to port and set handlers
      setupmidi(port, packed_id);
      Serial.println("-----");
    }
  }
}

#define SINGLE_FRAME_READ

void read_midi_usb_devices() {
  #ifdef SINGLE_FRAME_READ
    static int counter;
    for (int i = 0 ; i < NUM_USB_DEVICES ; i++) {
      //while(usb_midi_device[i]->read());
      if (usb_midi_device[i]->read()) {
        //usb_midi_device[counter%NUM_USB_DEVICES]->sendNoteOn(random(0,127),random(0,127),random(1,16));
        counter++;
        //Serial.printf("%i: read data from %04x:%04x\n", counter, usb_midi_device[i]->idVendor(), usb_midi_device[i]->idProduct());
      }
    }
  #else
    static int counter;
    // only process one device per loop
    if (counter>=NUM_USB_DEVICES)
      counter = 0;
    while(usb_midi_device[counter]->read());
    counter++;
  #endif
}

void send_midi_usb_clocks() {
  #ifdef ENABLE_BEATSTEP
    if(ixBeatStep!=0xFF) {
      midi_beatstep->sendRealTime(midi::Clock);
      //midi_beatstep->send_now();
    }
  #endif
  /*if(ixAPCmini!=0xFF) {
    midi_apcmini->sendRealTime(midi::Clock);
  }*/
  #ifdef ENABLE_BAMBLE
    if (ixBamble!=0xFF) {
      midi_bamble->sendRealTime(midi::Clock);
      //midi_bamble->send_now();
    }
  #endif
}

void loop_midi_usb_devices() {
  unsigned long temp_tick;
  noInterrupts();
  temp_tick = ticks;
  interrupts();
  #ifdef ENABLE_BEATSTEP
      beatstep_loop(temp_tick);
  #endif

  #ifdef ENABLE_APCMINI
      apcmini_loop(temp_tick);
  #endif

  #ifdef ENABLE_BAMBLE
      bamble_loop(temp_tick);
  #endif

  #ifdef ENABLE_MPK49
      MPK49_loop(temp_tick);
  #endif

  #ifdef ENABLE_KEYSTEP
      keystep_loop(temp_tick);
  #endif
}

// call this when global clock should be reset
void on_restart() {
  restart_on_next_bar = false;

  Serial.println(F("on_restart()==>"));

#ifdef USE_UCLOCK
  /*uClock.setTempo(bpm_current); // todo: probably not needed?
  Serial.println(F("reset tempo"));
  uClock.resetCounters();
  Serial.println(F("reset counters"));*/
#else
  ticks = 0;
  Serial.println(F("reset ticks"));
#endif
  noInterrupts();
  ticks = 0;
  interrupts();
  last_processed_tick = -1;
  
  send_midi_serial_stop_start();

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
#ifdef ENABLE_KEYSTEP
  Serial.print(F("restart keystep..."));
  keystep_on_restart();
  Serial.println(F("restarted"));
#endif

  Serial.println(F("<==on_restart()"));
}

void setup_multi_usb() {
  Serial.println(F("Arduino initialising usb/midi..."));

  Usb.begin();
  for (int i = 0 ; i < 5 ; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
  }
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
