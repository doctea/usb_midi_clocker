#include "usb.h"

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

MIDIDevice_BigBuffer * usb_midi_device[8] = {
  &midi01, &midi02, &midi03, &midi04, &midi05, &midi06, &midi07, &midi08,
};

uint64_t usb_midi_connected[8] = { 0,0,0,0,0,0,0,0 };

#include "midi_out_wrapper.h"
#include "MidiMappings.h"

#include "clock.h"

// set the incoming midi from the USB host (ie computer) to go out to first Bamble pitch channel
MIDIOutputWrapper midi_bamble_ch1_wrapper = MIDIOutputWrapper((char*)"USB : Bamble : ch 1", &midi_bamble, 1);
MIDIOutputWrapper *pc_usb_1_output = &midi_bamble_ch1_wrapper; //midi_out_bitbox_wrapper; //&midi_out_bass_wrapper;
void pc_usb_1_setOutputWrapper(MIDIOutputWrapper *wrapper) {
  pc_usb_1_output->stop_all_notes();
  pc_usb_1_output = wrapper;    
}

MIDIOutputWrapper *pc_usb_2_output = &midi_out_bitbox_wrapper; //&midi_out_bass_wrapper;
void pc_usb_2_setOutputWrapper(MIDIOutputWrapper *wrapper) {
  pc_usb_2_output->stop_all_notes();
  pc_usb_2_output = wrapper;    
}

void pc_usb_handle_note_on(byte channel, byte note, byte velocity) { //, byte cable) {
  byte cable = usbMIDI.getCable();
  //Serial.printf("pc_usb_handle_note_on (%i, %i, %i, %i)!\n", channel, note, velocity, cable);
  if (cable==1)
    pc_usb_1_output->sendNoteOn(note, velocity); //, channel);
  else
    pc_usb_2_output->sendNoteOn(note, velocity); //, channel);
}
void pc_usb_handle_note_off(byte channel, byte note, byte velocity) { //, byte cable) {
  byte cable = usbMIDI.getCable();
  //Serial.printf("pc_usb_handle_note_off(%i, %i, %i, %i)!\n", channel, note, velocity, cable);
  if (cable==1)
    pc_usb_1_output->sendNoteOff(note, velocity); //, channel);
  else
    pc_usb_2_output->sendNoteOff(note, velocity); //, channel);
}

// for handling external midi clock from host's usb
bool usb_midi_clock_ticked = false;
unsigned long last_usb_midi_clock_ticked_at;
void usb_midi_handle_clock() {
  if (usb_midi_clock_ticked) {
    Serial.printf("WARNING: received a usb midi clock tick at %u, but last one from %u was not yet processed (didn't process within gap of %u)!", millis(), last_usb_midi_clock_ticked_at, millis()-last_usb_midi_clock_ticked_at);
  }
  last_usb_midi_clock_ticked_at = millis();
  usb_midi_clock_ticked = true;
}
bool check_and_unset_usb_midi_clock_ticked() {
  bool v = usb_midi_clock_ticked;
  usb_midi_clock_ticked = false;
  return v;
}
void usb_midi_handle_start() {
  if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
    playing = true;
    on_restart();
  }
}
void usb_midi_handle_continue() {
  if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
    playing = true;
  }
}
void usb_midi_handle_stop() {
  if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
    playing = false;
  }
}

void setup_pc_usb() {
  usbMIDI.setHandleNoteOn(pc_usb_handle_note_on);
  usbMIDI.setHandleNoteOff(pc_usb_handle_note_off);

  usbMIDI.setHandleClock(usb_midi_handle_clock);
  usbMIDI.setHandleStart(usb_midi_handle_start);
  usbMIDI.setHandleStop(usb_midi_handle_stop);
  usbMIDI.setHandleContinue(usb_midi_handle_continue);
}

void read_usb_from_computer() {
  while(usbMIDI.read());
}