#include "midi_pc_usb.h"

#include "midi_out_wrapper.h"
#include "MidiMappings.h"

#include "clock.h"

#include "bpm.h"

#include "multi_usb_handlers.h"

#include "behaviour_bamble.h"

#include "midi_mapper_matrix_manager.h"

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
bool usb_midi_clock_ticked = false;
unsigned long last_usb_midi_clock_ticked_at;
void pc_usb_midi_handle_clock() {
  if (clock_mode==CLOCK_EXTERNAL_USB_HOST && usb_midi_clock_ticked) {
    Serial.printf("WARNING: received a usb midi clock tick at %u, but last one from %u was not yet processed (didn't process within gap of %u)!\n", millis(), last_usb_midi_clock_ticked_at, millis()-last_usb_midi_clock_ticked_at);
  }
  /*if (CLOCK_EXTERNAL_USB_HOST) {  // TODO: figure out why tempo estimation isn't working and fix
    tap_tempo_tracker.push_beat();
  }*/
  last_usb_midi_clock_ticked_at = millis();
  usb_midi_clock_ticked = true;
}
bool check_and_unset_pc_usb_midi_clock_ticked() {
  bool v = usb_midi_clock_ticked;
  usb_midi_clock_ticked = false;
  /*if(clock_mode==CLOCK_EXTERNAL_USB_HOST && ticks%PPQN==0) {  // TODO: figure out why this isn't working and fix
    set_bpm(tap_tempo_tracker.bpm_calculate_current());
  }*/
  return v;
}
void pc_usb_midi_handle_start() {
  if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
    //tap_tempo_tracker.reset();
    playing = true;
    global_on_restart();
  }
}
void pc_usb_midi_handle_continue() {
  if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
    playing = true;
  }
}
void pc_usb_midi_handle_stop() {
  if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
    playing = false;
  }
}

void setup_pc_usb() {
  usbMIDI.setHandleNoteOn(pc_usb_handle_note_on);
  usbMIDI.setHandleNoteOff(pc_usb_handle_note_off);
  usbMIDI.setHandleControlChange(pc_usb_handle_control_change);

  usbMIDI.setHandleClock(pc_usb_midi_handle_clock);
  usbMIDI.setHandleStart(pc_usb_midi_handle_start);
  usbMIDI.setHandleStop(pc_usb_midi_handle_stop);
  usbMIDI.setHandleContinue(pc_usb_midi_handle_continue);
}

void read_usb_from_computer() {
  while(usbMIDI.read());
}