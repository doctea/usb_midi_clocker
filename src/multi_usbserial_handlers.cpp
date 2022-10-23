#include "Config.h"

#include "bpm.h"
#include "midi/midi_outs.h"

#include "multi_usbserial_handlers.h"
#include "multi_usbserial_wrapper.h"

#include "tft.h"

#include "behaviours/behaviour_manager.h"

/*
usb_midi_device[0] is 1C75:0288 aka Arturia:Arturia KeyStep 32
usb_midi_device[1] is 2886:800B aka The Tyrell Corporation:Bambleweeny57
usb_midi_device[2] is 1C75:0206 aka Arturia:Arturia BeatStep
usb_midi_device[3] is 09E8:0028 aka AKAI PROFESSIONAL,LP:APC MINI       
usb_midi_device[4] is 09E8:006B aka Akai:Akai MPK49
*/

extern USBHost Usb;

USBSerialWrapper userial1(Usb);
USBSerialWrapper userial2(Usb);
USBSerialWrapper userial3(Usb);

midi::MidiInterface<USBSerialWrapper> midiusbserial1(userial1);
midi::MidiInterface<USBSerialWrapper> midiusbserial2(userial2);
midi::MidiInterface<USBSerialWrapper> midiusbserial3(userial3);


/*MIDIDevice_BigBuffer * usb_midi_device[NUM_USB_DEVICES] = {
  &midi01, &midi02, &midi03, &midi04, &midi05, &midi06, &midi07, &midi08,
};*/

usbserial_midi_slot usbserial_midi_slots[NUM_USBSERIAL_DEVICES] = {
  { 0x00, 0x00, 0x0000, &userial1, &midiusbserial1, nullptr },
  { 0x00, 0x00, 0x0000, &userial2, &midiusbserial2, nullptr },
  { 0x00, 0x00, 0x0000, &userial3, &midiusbserial3, nullptr }
};

//uint64_t usb_midi_connected[NUM_USB_DEVICES] = { 0,0,0,0,0,0,0,0 };

// assign device to port and set appropriate handlers
void setup_usbserial_midi_device(uint8_t idx, uint32_t packed_id = 0x0000) {
  uint16_t vid, pid;
  if (packed_id==0) {
    vid = usbserial_midi_slots[idx].usbdevice->idVendor();
    pid = usbserial_midi_slots[idx].usbdevice->idProduct();
    packed_id = (usbserial_midi_slots[idx].usbdevice->idVendor()<<16) | (usbserial_midi_slots[idx].usbdevice->idProduct());
  } else {
    vid = packed_id >> 16;
    pid = 0x0000FFFF & packed_id;
  }
  if ((uint32_t)((usbserial_midi_slots[idx].usbdevice->idVendor()<<16) | (usbserial_midi_slots[idx].usbdevice->idProduct())) != packed_id) {
      Serial.printf("packed_id %08X and newly-generated packed_id %08X don't match already?!", 
        (usbserial_midi_slots[idx].usbdevice->idVendor()<<16) | (usbserial_midi_slots[idx].usbdevice->idProduct()),
        packed_id 
      );
      return;
  }
  Serial.printf("USBSerial Port %d changed from %08X to %08X (now ", idx, usbserial_midi_slots[idx].packed_id, packed_id);
  Serial.printf("'%s' '%s')\n", usbserial_midi_slots[idx].usbdevice->manufacturer(), usbserial_midi_slots[idx].usbdevice->product());
  usbserial_midi_slots[idx].packed_id = packed_id;

  // remove handlers that might already be set on this port -- new ones assigned below thru xxx_init() functions
  if (usbserial_midi_slots[idx].behaviour!=nullptr) {
    Serial.printf("Disconnecting usbserial_midi_slot %i behaviour\n", idx);
    usbserial_midi_slots[idx].behaviour->disconnect_device();
  }

  if (packed_id==0) {
    usbserial_midi_slots[idx].packed_id = 0;
    Serial.printf("Disconnected usbserial device on port %i\n", idx);
    return;
  }

  // attempt to connect this device to a registered behaviour type
  Serial.printf("about to attempt to connect %d w/ %4x\n", idx, packed_id);
  if (behaviour_manager->attempt_usbserial_device_connect(idx, packed_id))
    return;

  //usb_midi_connected[idx] = packed_id;
  usbserial_midi_slots[idx].packed_id = packed_id;

  Serial.print(F("Detected unknown (or disabled) device vid="));
  Serial.print(vid);
  Serial.print(F(", pid="));
  Serial.println(pid);
}


void update_usbserial_device_connections() {
  for (int port = 0 ; port < NUM_USBSERIAL_DEVICES ; port++) {
    uint32_t packed_id = (usbserial_midi_slots[port].usbdevice->idVendor()<<16) | (usbserial_midi_slots[port].usbdevice->idProduct());
    //Serial.printf("packed %04X and %04X to %08X\n", usb_midi_slots[port].device->idVendor(),  usb_midi_slots[port].device->idProduct(), packed_id);
    if (usbserial_midi_slots[port].packed_id != packed_id) {
      // device at this port has changed since we last saw it -- ie, disconnection or connection
      // unassign the midi_xxx helper pointers if appropriate
      usbserial_midi_slots[port].behaviour = nullptr;
      Serial.printf(F("update_usbserial_device_connections: device at port %i is %08X which differs from current %08X!\n"), port, packed_id, usbserial_midi_slots[port].packed_id);
      // call setup_usb_midi_device() to assign device to port and set handlers
      setup_usbserial_midi_device(port, packed_id);
      Serial.println(F("-----"));
    }
  }
}

void setup_multi_usbserial() {
    // nothing to be done...
    /*Serial.print(F("Arduino initialising usb/midi...")); Serial.flush();

    Usb.begin();
    Serial.println(F("Usb.begin() returned")); Serial.flush();
    for (int i = 0 ; i < 5 ; i++) {
    //digitalWrite(LED_BUILTIN, HIGH);
    Serial.printf(F("%i/5: Waiting 500ms for USB to settle down.."), i+1); Serial.flush();
    tft_print((char*)".");
    delay(500);
    //digitalWrite(LED_BUILTIN, LOW);
    }
    tft_print((char*)"done.\n");
    Serial.println(F("setup_multi_usb() finishing.")); Serial.flush();*/
}
