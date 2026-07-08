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

#ifdef ENABLE_PARAMETERS
  #include "parameter_inputs/MIDIParameterInput.h"

  MIDIPitchBendParameterInput *pb_inputs[NUM_PC_SOURCES];

  __attribute__((noinline))
  void setup_pc_usb_pitchbend_parameter_inputs() {
    for (int i=0; i<NUM_PC_SOURCES; i++) {
      char buf[32];
      snprintf(buf, sizeof(buf), "PCUSB Bend %i", i+1);
      Serial.printf("setup_pc_usb_pitchbend_parameter_inputs %i/%i: creating %s\n", i+1, NUM_PC_SOURCES, buf);
      Serial.flush();
      pb_inputs[i] = new MIDIPitchBendParameterInput(buf, "PC USB", 0);
      Serial.printf("setup_pc_usb_pitchbend_parameter_inputs %i/%i: created %s, adding to parameter_manage at %p..\n", i+1, NUM_PC_SOURCES, buf, parameter_manager);
      Serial.flush();
      if (parameter_manager==nullptr) {
        Serial.println("setup_pc_usb_pitchbend_parameter_inputs: parameter_manager is null!"); Serial.flush();
      } else {
        Serial.printf("setup_pc_usb_pitchbend_parameter_inputs: parameter_manager is at %p\n", parameter_manager); Serial.flush();
        parameter_manager->addInput(pb_inputs[i]);
        Serial.println("ADDED!"); Serial.flush();
      }
    }
    Serial.println("setup_pc_usb_pitchbend_parameter_inputs done!"); Serial.flush();
  }
#endif


void pc_usb_handle_note_on(byte channel, byte note, byte velocity) { //, byte cable) {
  byte cable = usbMIDI.getCable();
  //Serial.printf("pc_usb_handle_note_on (%i, %i, %i, %i)!\n", channel, note, velocity, cable);

  midi_matrix_manager->processNoteOn(pc_usb_sources[cable], note, velocity, channel);
}
void pc_usb_handle_note_off(byte channel, byte note, byte velocity) { //, byte cable) {
  byte cable = usbMIDI.getCable();
  //Serial.printf("pc_usb_handle_note_off(%i, %i, %i, %i)!\n", channel, note, velocity, cable);

  midi_matrix_manager->processNoteOff(pc_usb_sources[cable], note, velocity, channel);
}
void pc_usb_handle_control_change(byte channel, byte cc, byte value) { //, byte cable) {
  //Serial.printf("pc_usb_handle_note_off(%i, %i, %i, %i)!\n", channel, note, velocity, cable);
  byte cable = usbMIDI.getCable();
  
  midi_matrix_manager->processControlChange(pc_usb_sources[cable], cc, value, channel);
}
void pc_usb_handle_pitch_bend(byte channel, int bend) {
  byte cable = usbMIDI.getCable();
  // Serial.printf("pc_usb_handle_pitch_bend(%i, %i, %i)\n", channel, bend, cable);

  if (!pb_inputs[cable])
    return;

  midi_matrix_manager->processPitchBend(pc_usb_sources[cable], bend, channel);

  #ifdef ENABLE_PARAMETERS
    if (pb_inputs[cable]->responds_to_pitch_bend(channel))
      pb_inputs[cable]->receive_pitch_bend(bend, channel);
  #endif
}

void read_usb_from_computer() {
  while(usbMIDI.read());
}

FLASHMEM
void setup_pc_usb() {
  usbMIDI.setHandleNoteOn(pc_usb_handle_note_on);
  usbMIDI.setHandleNoteOff(pc_usb_handle_note_off);
  usbMIDI.setHandleControlChange(pc_usb_handle_control_change);
  usbMIDI.setHandlePitchChange(pc_usb_handle_pitch_bend);

  usbMIDI.setHandleClock(pc_usb_midi_handle_clock);
  usbMIDI.setHandleStart(pc_usb_midi_handle_start);
  usbMIDI.setHandleStop(pc_usb_midi_handle_stop);
  usbMIDI.setHandleContinue(pc_usb_midi_handle_continue);

  set_global_restart_callback(&global_on_restart);
}
