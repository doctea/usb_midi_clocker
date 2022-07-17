#include "bpm.h"
#include "midi_outs.h"

#include "multi_usb_handlers.h"

#include "tft.h"

#include "behaviour_manager.h"

/*
usb_midi_device[0] is 1C75:0288 aka Arturia:Arturia KeyStep 32
usb_midi_device[1] is 2886:800B aka The Tyrell Corporation:Bambleweeny57
usb_midi_device[2] is 1C75:0206 aka Arturia:Arturia BeatStep
usb_midi_device[3] is 09E8:0028 aka AKAI PROFESSIONAL,LP:APC MINI       
usb_midi_device[4] is 09E8:006B aka Akai:Akai MPK49
*/

USBHost Usb;
USBHub hub1(Usb);
USBHub hub2(Usb);
USBHub hub3(Usb);
USBHub hub4(Usb);
MIDIDevice_BigBuffer midi01(Usb);
MIDIDevice_BigBuffer midi02(Usb);
MIDIDevice_BigBuffer midi03(Usb);
MIDIDevice_BigBuffer midi04(Usb);
MIDIDevice_BigBuffer midi05(Usb);
MIDIDevice_BigBuffer midi06(Usb);
MIDIDevice_BigBuffer midi07(Usb);
MIDIDevice_BigBuffer midi08(Usb);

/*MIDIDevice_BigBuffer * usb_midi_device[NUM_USB_DEVICES] = {
  &midi01, &midi02, &midi03, &midi04, &midi05, &midi06, &midi07, &midi08,
};*/

usb_midi_slot usb_midi_slots[NUM_USB_DEVICES] = {
  { 0x00, 0x00, 0x0000, &midi01, nullptr },
  { 0x00, 0x00, 0x0000, &midi02, nullptr },
  { 0x00, 0x00, 0x0000, &midi03, nullptr },
  { 0x00, 0x00, 0x0000, &midi04, nullptr },
  { 0x00, 0x00, 0x0000, &midi05, nullptr },
  { 0x00, 0x00, 0x0000, &midi06, nullptr },
  { 0x00, 0x00, 0x0000, &midi07, nullptr },
  { 0x00, 0x00, 0x0000, &midi08, nullptr },
};

//uint64_t usb_midi_connected[NUM_USB_DEVICES] = { 0,0,0,0,0,0,0,0 };

// assign device to port and set appropriate handlers
void setup_usb_midi_device(uint8_t idx, uint32_t packed_id = 0x0000) {
  uint16_t vid, pid;
  if (packed_id==0) {
    vid = usb_midi_slots[idx].device->idVendor();
    pid = usb_midi_slots[idx].device->idProduct();
    packed_id = (usb_midi_slots[idx].device->idVendor()<<16) | (usb_midi_slots[idx].device->idProduct());
  } else {
    vid = packed_id >> 16;
    pid = 0x0000FFFF & packed_id;
  }
  if ((uint32_t)((usb_midi_slots[idx].device->idVendor()<<16) | (usb_midi_slots[idx].device->idProduct())) != packed_id) {
      Serial.printf("packed_id %08X and newly-generated packed_id %08X don't match already?!", 
        (usb_midi_slots[idx].device->idVendor()<<16) | (usb_midi_slots[idx].device->idProduct()),
        packed_id 
      );
      return;
  }
  Serial.printf("USB Port %d changed from %08X to %08X (now ", idx, usb_midi_slots[idx].packed_id, packed_id);
  Serial.printf("'%s' '%s')\n", usb_midi_slots[idx].device->manufacturer(), usb_midi_slots[idx].device->product());
  usb_midi_slots[idx].packed_id = packed_id;

  // remove handlers that might already be set on this port -- new ones assigned below thru xxx_init() functions
  if (usb_midi_slots[idx].behaviour!=nullptr) {
    Serial.printf("Disconnecting usb_midi_slot %i behaviour\n", idx);
    usb_midi_slots[idx].behaviour->disconnect_device();
  }

  if (packed_id==0) {
    usb_midi_slots[idx].packed_id = 0;
    Serial.printf("Disconnected device on port %i\n", idx);
    return;
  }

  // attempt to connect this device to a registered behaviour type
  if (behaviour_manager->attempt_device_connect(idx, packed_id))
    return;

  //usb_midi_connected[idx] = packed_id;
  usb_midi_slots[idx].packed_id = packed_id;

  Serial.print(F("Detected unknown (or disabled) device vid="));
  Serial.print(vid);
  Serial.print(F(", pid="));
  Serial.println(pid);
}


void update_usb_device_connections() {
  for (int port=0; port < NUM_USB_DEVICES; port++) {
    uint32_t packed_id = (usb_midi_slots[port].device->idVendor()<<16) | (usb_midi_slots[port].device->idProduct());
    //Serial.printf("packed %04X and %04X to %08X\n", usb_midi_slots[port].device->idVendor(),  usb_midi_slots[port].device->idProduct(), packed_id);
    if (usb_midi_slots[port].packed_id != packed_id) {
      // device at this port has changed since we last saw it -- ie, disconnection or connection
      // unassign the midi_xxx helper pointers if appropriate
      usb_midi_slots[port].behaviour = nullptr;
      Serial.printf("update_usb_device_connections: device at port %i is %08X which differs from current %08X!\n", port, packed_id, usb_midi_slots[port].packed_id);
      // call setup_usb_midi_device() to assign device to port and set handlers
      setup_usb_midi_device(port, packed_id);
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
      if (usb_midi_slots[i].device->read()) {
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



// call this when global clock should be reset
void global_on_restart() {
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
  //noInterrupts();
  ticks = 0;
  //interrupts();
  last_processed_tick = -1;
  
  send_midi_serial_stop_start();

  behaviour_manager->on_restart();

  Serial.println(F("<==on_restart()"));
}

void setup_multi_usb() {
  Serial.print(F("Arduino initialising usb/midi..."));

  Usb.begin();
  for (int i = 0 ; i < 5 ; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    tft_print((char*)".");
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
  }
  tft_print((char*)"\n");
}
