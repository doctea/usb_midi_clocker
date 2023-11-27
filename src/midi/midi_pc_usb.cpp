#include "midi/midi_pc_usb.h"

#include "midi/midi_out_wrapper.h"
#include "midi/MidiMappings.h"

#include "clock.h"

#include "bpm.h"

#include "usb/multi_usb_handlers.h"

#include "behaviours/behaviour_bamble.h"

#include "midi/midi_mapper_matrix_manager.h"

// TODO: turn this into a behaviour instead

source_id_t pc_usb_sources[NUM_PC_SOURCES];

void pc_usb_handle_note_on(byte channel, byte note, byte velocity) { //, byte cable) {
  //Serial.printf("pc_usb_handle_note_on (%i, %i, %i, %i)!\n", channel, note, velocity, cable);
  byte cable = usbMIDI.getCable();

  midi_matrix_manager->processNoteOn(pc_usb_sources[cable], note, velocity, channel = 0);
}
void pc_usb_handle_note_off(byte channel, byte note, byte velocity) { //, byte cable) {
  //Serial.printf("pc_usb_handle_note_off(%i, %i, %i, %i)!\n", channel, note, velocity, cable);
  byte cable = usbMIDI.getCable();

  midi_matrix_manager->processNoteOff(pc_usb_sources[cable], note, velocity, channel = 0);
}
void pc_usb_handle_control_change(byte channel, byte cc, byte value) { //, byte cable) {
  //Serial.printf("pc_usb_handle_note_off(%i, %i, %i, %i)!\n", channel, note, velocity, cable);
  byte cable = usbMIDI.getCable();
  
  midi_matrix_manager->processControlChange(pc_usb_sources[cable], cc, value, channel = 0);
}

// for handling external midi clock from host's usb

FLASHMEM
void setup_pc_usb() {
  usbMIDI.setHandleNoteOn(pc_usb_handle_note_on);
  usbMIDI.setHandleNoteOff(pc_usb_handle_note_off);
  usbMIDI.setHandleControlChange(pc_usb_handle_control_change);

  usbMIDI.setHandleClock(pc_usb_midi_handle_clock);
  usbMIDI.setHandleStart(pc_usb_midi_handle_start);
  usbMIDI.setHandleStop(pc_usb_midi_handle_stop);
  usbMIDI.setHandleContinue(pc_usb_midi_handle_continue);

  set_global_restart_callback(&global_on_restart);
}

void read_usb_from_computer() {
  while(usbMIDI.read());
}