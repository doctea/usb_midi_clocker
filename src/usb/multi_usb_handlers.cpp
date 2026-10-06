#include "Config.h"

#include "clock.h"
#include "bpm.h"
#include "debug.h"
#include "behaviours/behaviour_manager.h"
#include "interfaces/interfaces.h"
#include "midi/midi_mapper_matrix_manager.h"
#include "system_settings.h"

static void release_generated_outputs() {
  behaviour_manager->release_outputs();
  if (midi_matrix_manager != nullptr)
    midi_matrix_manager->stop_all_notes();
  if (gate_manager != nullptr)
    gate_manager->stop_all_gates();
}

// call this when global clock should be reset
// todo: should probably move this elsewhere..?
void global_on_restart() {
  release_generated_outputs();
  set_restart_on_next_bar(false);

  Serial_println(F("on_restart()==>"));

  clock_reset();
  //interrupts();
  last_processed_tick = -1;
  
  //send_midi_serial_stop_start();

  behaviour_manager->on_restart();

  Serial.println(F("<==on_restart()"));
}

void global_on_stop() {
  if (system_settings.isReleaseOutputsOnMidiStop())
    release_generated_outputs();
}

void global_on_external_clock_stall(bool stalled) {
  if (stalled && system_settings.isReleaseOutputsOnExternalClockLoss())
    release_generated_outputs();
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
// use_MIDIDevice_BigBuffer midi13(Usb);
// use_MIDIDevice_BigBuffer midi14(Usb);
// use_MIDIDevice_BigBuffer midi15(Usb);
// use_MIDIDevice_BigBuffer midi16(Usb);

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
  // { 0x00, 0x00, 0x0000, &midi13, nullptr },
  // { 0x00, 0x00, 0x0000, &midi14, nullptr },
  // { 0x00, 0x00, 0x0000, &midi15, nullptr },
  // { 0x00, 0x00, 0x0000, &midi16, nullptr }
};

//uint64_t usb_midi_connected[NUM_USB_MIDI_DEVICES] = { 0,0,0,0,0,0,0,0 };

void dump_usb_diagnostics() {
  uint32_t devices, pipes, transfers, strings;
  USBHost::countFree(devices, pipes, transfers, strings);
  if (devices == UINT32_MAX || pipes == UINT32_MAX || transfers == UINT32_MAX || strings == UINT32_MAX) {
    Serial.printf("USB host: %s; free pools CORRUPT (counts unavailable)\n",
                  USBHost::hadSystemError() ? "HALTED" : "no system error");
  } else {
    Serial.printf("USB host: %s; free pools devices=%lu pipes=%lu transfers=%lu strings=%lu\n",
                  USBHost::hadSystemError() ? "HALTED" : "no system error",
                  (unsigned long)devices, (unsigned long)pipes,
                  (unsigned long)transfers, (unsigned long)strings);
  }
  const auto controller = USBHost::getControllerDiagnosticInfo();
  Serial.printf("USB controller command=%08lX status=%08lX intr=%08lX frame=%lu async_hw=%08lX async_sw=%08lX port=%08lX enumeration_busy=%u\n",
                (unsigned long)controller.command, (unsigned long)controller.status,
                (unsigned long)controller.interrupt_enable, (unsigned long)controller.frame_index,
                (unsigned long)controller.async_address, (unsigned long)controller.async_software_head,
                (unsigned long)controller.port_status, (unsigned int)controller.enumerating);
  Serial.printf("USB schedules run=%u halted=%u async_enable=%u async_running=%u periodic_enable=%u periodic_running=%u\n",
                (unsigned int)((controller.command >> 0) & 1), (unsigned int)((controller.status >> 12) & 1),
                (unsigned int)((controller.command >> 5) & 1), (unsigned int)((controller.status >> 15) & 1),
                (unsigned int)((controller.command >> 4) & 1), (unsigned int)((controller.status >> 14) & 1));
  constexpr uint32_t pipe_capacity = 32;
  USBHost::PipeDiagnosticInfo pipe_info[pipe_capacity];
  const uint32_t pipe_count = USBHost::getAsyncPipeDiagnostics(pipe_info, pipe_capacity);
  Serial.printf("USB async pipes=%lu limit=%lu\n", (unsigned long)pipe_count, (unsigned long)pipe_capacity);
  for (uint32_t index = 0; index < pipe_count; ++index) {
    const auto &pipe = pipe_info[index];
    Serial.printf("USB async pipe=%08lX link=%08lX head=%u id=%04X:%04X address=%u hub=%u port=%u speed=%u type=%u direction=%u ep=%u cap=%08lX:%08lX current=%08lX next=%08lX token=%08lX dummy=%08lX\n",
                  (unsigned long)pipe.address, (unsigned long)pipe.horizontal_link,
                  (unsigned int)((pipe.capabilities1 >> 15) & 1), pipe.vid, pipe.pid,
                  (unsigned int)pipe.device_address, (unsigned int)pipe.hub, (unsigned int)pipe.port,
                  (unsigned int)pipe.speed, (unsigned int)pipe.type, (unsigned int)pipe.direction,
                  (unsigned int)((pipe.capabilities1 >> 8) & 15),
                  (unsigned long)pipe.capabilities1, (unsigned long)pipe.capabilities2,
                  (unsigned long)pipe.current, (unsigned long)pipe.next,
                  (unsigned long)pipe.token, (unsigned long)pipe.dummy);
      Serial.printf("USB async work pipe=%08lX submitted=%lu completed=%lu errors=%lu pending=%lu active=%lu first=%08lX first_token=%08lX first_next=%08lX next_pending=%u truncated=%u\n",
              (unsigned long)pipe.address, (unsigned long)pipe.transfer_submissions,
              (unsigned long)pipe.transfer_completions, (unsigned long)pipe.transfer_errors,
              (unsigned long)pipe.pending_count, (unsigned long)pipe.active_count,
              (unsigned long)pipe.first_pending, (unsigned long)pipe.first_pending_token,
              (unsigned long)pipe.first_pending_next, (unsigned int)pipe.next_is_pending,
              (unsigned int)pipe.followup_truncated);
  }
  const auto info = USBHost::getDiagnosticInfo();
  Serial.printf("USB error records=%lu last=%s (%s) at=%lu ms state=%u detail=%08lX\n",
                (unsigned long)info.count, USBHost::diagnosticErrorName(info.error),
                info.previous_boot ? "previous boot" : "this boot",
                (unsigned long)info.at_ms, info.state, (unsigned long)info.detail);
  if (info.count > 0) {
    Serial.printf("USB error device=%04X:%04X address=%u hub=%u port=%u\n",
                  info.vid, info.pid, info.address, info.hub, info.port);
  } else {
    Serial.println("USB error device unavailable: no retained error event");
  }
  const auto progress = USBHost::getPreviousDiagnosticProgress();
  Serial.printf("USB previous boot phase=%s at=%lu ms\n",
                USBHost::diagnosticPhaseName(progress.phase), (unsigned long)progress.at_ms);
  USBHub *hubs[] = { &hub1, &hub2, &hub3, &hub4 };
  for (unsigned int hub_index = 0; hub_index < sizeof(hubs) / sizeof(hubs[0]); ++hub_index) {
    USBHub *hub = hubs[hub_index];
    const auto hub_info = hub->getPortDiagnostics();
    const uint16_t vid = hub->idVendor();
    if (!vid && !hub_info.port_count && !hub_info.change_pipe_present) continue;
    Serial.printf("USB hub slot=%u id=%04X:%04X ports=%u change_pipe=%u change_bits=%08lX reset_port=%u\n",
                  hub_index, vid, hub->idProduct(), (unsigned int)hub_info.port_count,
                  (unsigned int)hub_info.change_pipe_present, (unsigned long)hub_info.change_bits,
                  (unsigned int)hub_info.reset_port);
    for (uint8_t port_index = 0; port_index < hub_info.port_count; ++port_index) {
      const auto &port = hub_info.ports[port_index];
      Serial.printf("USB hub port=%u:%u status=%08lX state=%u child=%u id=%04X:%04X address=%u enum=%u\n",
                    hub_index, (unsigned int)(port_index + 1), (unsigned long)port.status,
                    (unsigned int)port.state, (unsigned int)port.has_device, port.vid, port.pid,
                    (unsigned int)port.address, (unsigned int)port.enumeration_state);
    }
  }
  for (unsigned int port = 0; port < NUM_USB_MIDI_DEVICES; ++port) {
    auto *device = usb_midi_slots[port].device;
    if (device == nullptr) continue;
    const auto tx = device->getTransmitDiagnosticInfo();
    Serial.printf("USB MIDI port=%u id=%04X:%04X ready=%u pending=%u realtime=%u dropped=%lu realtime_dropped=%lu\n",
                  port, device->idVendor(), device->idProduct(),
                  (unsigned int)device->hasTransmitQueueStorage(),
                  (unsigned int)device->getTransmitPendingCount(),
                  (unsigned int)device->getRealtimeTransmitPendingCount(),
                  (unsigned long)device->getTransmitDropCount(),
                  (unsigned long)device->getRealtimeTransmitDropCount());
    Serial.printf("USB MIDI tx port=%u buffers=%u/%u capacity=%u timer=%lu submitted=%lu completed=%lu errors=%lu\n",
                  port, (unsigned int)tx.buffer1_count, (unsigned int)tx.buffer2_count,
                  (unsigned int)tx.packet_capacity, (unsigned long)tx.timer_callbacks,
                  (unsigned long)tx.submissions, (unsigned long)tx.completions,
                  (unsigned long)tx.errors);
  }
}

static void disconnect_usb_midi_slot(uint8_t idx) {
  DeviceBehaviourUSBBase *behaviour = usb_midi_slots[idx].behaviour;
  usb_midi_slots[idx].behaviour = nullptr;
  if (behaviour != nullptr && behaviour->device == usb_midi_slots[idx].device) {
    Serial.printf(F("Disconnecting usb_midi_slot %i behaviour\n"), idx);
    behaviour->disconnect_device();
  }
}

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
  disconnect_usb_midi_slot(idx);

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
  uint32_t packed_ids[NUM_USB_MIDI_DEVICES];
  // only the ID snapshot is done with IRQs off; connect/disconnect work needs the USB ISR running
  #ifdef IRQ_PROTECT_USB_CHANGES
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  #endif
  {
    for (int port = 0 ; port < NUM_USB_MIDI_DEVICES ; port++) {
      packed_ids[port] = (usb_midi_slots[port].device->idVendor()<<16) | (usb_midi_slots[port].device->idProduct());
    }
  }
  for (int port = 0 ; port < NUM_USB_MIDI_DEVICES ; port++) {
    if (usb_midi_slots[port].packed_id != packed_ids[port]) {
      disconnect_usb_midi_slot(port);
    }
  }
  for (int port = 0 ; port < NUM_USB_MIDI_DEVICES ; port++) {
      //Serial_printf("update_usb_midi_device_connections() checking port %i/%i\n", port+1, NUM_USB_MIDI_DEVICES);
      uint32_t packed_id = packed_ids[port];
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

  for (unsigned int port = 0; port < NUM_USB_MIDI_DEVICES; port++) {
    if (!usb_midi_slots[port].device->beginTransmitQueues()) {
      Serial.printf("USB MIDI port %u: transmit queue allocation failed; congestion protection unavailable\n", port);
    }
  }

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