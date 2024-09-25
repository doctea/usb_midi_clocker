#include "Config.h"

#include "clock.h"
#include "bpm.h"
#include "debug.h"
#include "behaviours/behaviour_manager.h"

// call this when global clock should be reset
// todo: should probably move this elsewhere..?
void global_on_restart() {
  set_restart_on_next_bar(false);

  Serial_println(F("on_restart()==>"));

  clock_reset();
  //interrupts();
  last_processed_tick = -1;
  
  //send_midi_serial_stop_start();

  behaviour_manager->on_restart();

  Serial.println(F("<==on_restart()"));
}

#ifdef ENABLE_USB

#include "midi/midi_outs.h"
#include "usb/multi_usb_handlers.h"
#include "tft.h"
#include <util/atomic.h>

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
use_MIDIDevice_BigBuffer midi01(Usb);
use_MIDIDevice_BigBuffer midi02(Usb);
use_MIDIDevice_BigBuffer midi03(Usb);
use_MIDIDevice_BigBuffer midi04(Usb);
use_MIDIDevice_BigBuffer midi05(Usb);
use_MIDIDevice_BigBuffer midi06(Usb);
use_MIDIDevice_BigBuffer midi07(Usb);
use_MIDIDevice_BigBuffer midi08(Usb);
use_MIDIDevice_BigBuffer midi09(Usb);
use_MIDIDevice_BigBuffer midi10(Usb);
use_MIDIDevice_BigBuffer midi11(Usb);
use_MIDIDevice_BigBuffer midi12(Usb);
use_MIDIDevice_BigBuffer midi13(Usb);
use_MIDIDevice_BigBuffer midi14(Usb);
use_MIDIDevice_BigBuffer midi15(Usb);
use_MIDIDevice_BigBuffer midi16(Usb);

/*MIDIDevice_BigBuffer * usb_midi_device[NUM_USB_MIDI_DEVICES] = {
  &midi01, &midi02, &midi03, &midi04, &midi05, &midi06, &midi07, &midi08,
};*/

usb_midi_slot usb_midi_slots[NUM_USB_MIDI_DEVICES] = {
  { 0x00, 0x00, 0x0000, &midi01, nullptr },
  { 0x00, 0x00, 0x0000, &midi02, nullptr },
  { 0x00, 0x00, 0x0000, &midi03, nullptr },
  { 0x00, 0x00, 0x0000, &midi04, nullptr },
  { 0x00, 0x00, 0x0000, &midi05, nullptr },
  { 0x00, 0x00, 0x0000, &midi06, nullptr },
  { 0x00, 0x00, 0x0000, &midi07, nullptr },
  { 0x00, 0x00, 0x0000, &midi08, nullptr },
  { 0x00, 0x00, 0x0000, &midi09, nullptr },
  { 0x00, 0x00, 0x0000, &midi10, nullptr },
  { 0x00, 0x00, 0x0000, &midi11, nullptr },
  { 0x00, 0x00, 0x0000, &midi12, nullptr },
  { 0x00, 0x00, 0x0000, &midi13, nullptr },
  { 0x00, 0x00, 0x0000, &midi14, nullptr },
  { 0x00, 0x00, 0x0000, &midi15, nullptr },
  { 0x00, 0x00, 0x0000, &midi16, nullptr }
};

//uint64_t usb_midi_connected[NUM_USB_MIDI_DEVICES] = { 0,0,0,0,0,0,0,0 };

// assign device to port and set appropriate handlers
void setup_usb_midi_device(uint8_t idx, uint32_t packed_id = 0x00000000) {
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
      Serial.printf(F("packed_id %08X and newly-generated packed_id %08X don't match already?!"), 
        (usb_midi_slots[idx].device->idVendor()<<16) | (usb_midi_slots[idx].device->idProduct()),
        packed_id 
      );
      return;
  }
  Serial.printf(F("USB Port %d changed from %08X to %08X (now "), idx, usb_midi_slots[idx].packed_id, packed_id);
  if (usb_midi_slots[idx].device!=nullptr)
    Serial.printf("'%s' '%s')\n", usb_midi_slots[idx].device->manufacturer(), usb_midi_slots[idx].device->product());
  else
    Serial.println(F("disconnected)"));
  usb_midi_slots[idx].packed_id = packed_id;

  // remove handlers that might already be set on this port -- new ones assigned below thru xxx_init() functions
  if (usb_midi_slots[idx].behaviour!=nullptr) {
    Serial.printf(F("Disconnecting usb_midi_slot %i behaviour\n"), idx);
    usb_midi_slots[idx].behaviour->disconnect_device();
  }

  if (packed_id==0) {
    usb_midi_slots[idx].packed_id = 0;
    Serial.printf(F("Disconnected device on port %i\n"), idx);
    return;
  }

  // attempt to connect this device to a registered behaviour type
  if (behaviour_manager->attempt_usb_device_connect(idx, packed_id))
    return;

  //usb_midi_connected[idx] = packed_id;
  usb_midi_slots[idx].packed_id = packed_id;

  Serial.printf(F("Detected unknown (or disabled) USBMIDI device vid=%04x pid=%04x\n"), vid, pid);
}


void update_usb_midi_device_connections() {
  #ifdef IRQ_PROTECT_USB_CHANGES
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  #endif
  {
    for (int port = 0 ; port < NUM_USB_MIDI_DEVICES ; port++) {
      //Serial_printf("update_usb_midi_device_connections() checking port %i/%i\n", port+1, NUM_USB_MIDI_DEVICES);
      uint32_t packed_id = (usb_midi_slots[port].device->idVendor()<<16) | (usb_midi_slots[port].device->idProduct());
      //Serial.printf("packed %04X and %04X to %08X\n", usb_midi_slots[port].device->idVendor(),  usb_midi_slots[port].device->idProduct(), packed_id);
      if (usb_midi_slots[port].packed_id != packed_id) {
        // device at this port has changed since we last saw it -- ie, disconnection or connection
        // unassign the midi_xxx helper pointers if appropriate
        //usb_midi_slots[port].behaviour = nullptr;
        Serial_printf(F("update_usb_midi_device_connections: device at port %i is %08X which differs from current %08X!\n"), port, packed_id, usb_midi_slots[port].packed_id);
        // call setup_usb_midi_device() to assign device to port and set handlers
        setup_usb_midi_device(port, packed_id);
        Serial_println(F("-----"));
      }
    }
    //Serial_println("finished loop in update_usb_midi_device_connections");
  }
  //Serial_println("returning from update_usb_midi_device_connections");
}

//#define SINGLE_FRAME_READ_ONCE
#define SINGLE_FRAME_READ_ALL

/*void read_midi_usb_devices() {
  #ifdef SINGLE_FRAME_READ_ALL
    for (unsigned int i = 0 ; i < NUM_USB_MIDI_DEVICES ; i++) {
      while(usb_midi_slots[i].device!=nullptr && usb_midi_slots[i].device->read()); //device->read());
    }
  #else
    #ifdef SINGLE_FRAME_READ_ONCE
      //static int counter;
      for (unsigned int i = 0 ; i < NUM_USB_MIDI_DEVICES ; i++) {
        //while(usb_midi_device[i]->read());
        if (usb_midi_slots[i].device!=nullptr && usb_midi_slots[i].device->read()) {
          //usb_midi_device[counter%NUM_USB_MIDI_DEVICES]->sendNoteOn(random(0,127),random(0,127),random(1,16));
          //Serial.printf("%i: read data from %04x:%04x\n", counter, usb_midi_device[i]->idVendor(), usb_midi_device[i]->idProduct());
        }
        //counter++;
      }
    #else
      static int counter;
      // only all messages from one device per loop
      if (counter>=NUM_USB_MIDI_DEVICES)
        counter = 0;
      while(usb_midi_slots[counter].read());
      counter++;
    #endif
  #endif
}*/


//FLASHMEM 
void setup_multi_usb() { // error: void setup_multi_usb() causes a section type conflict with virtual void DeviceBehaviourUltimateBase::setup_callbacks()
  Serial.print(F("Arduino initialising usb/midi...")); Serial_flush();

  Usb.begin();
  Serial.println(F("Usb.begin() returned")); Serial_flush();
  for (unsigned int i = 0 ; i < 5 ; i++) {
    Serial.printf(F("%i/5: Waiting 500ms for USB to settle down.."), i+1); Serial_flush();
    Usb.Task();
    tft_print(".");
    delay(500);
  }
  tft_print("done.\n");
  Serial.println(F("setup_multi_usb() finishing.")); Serial_flush();
}

#endif