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

MIDIOutputWrapper *pc_usb_1_output = &midi_out_bitbox_wrapper; //&midi_out_bass_wrapper;
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

void setup_pc_usb() {
  usbMIDI.setHandleNoteOn(pc_usb_handle_note_on);
  usbMIDI.setHandleNoteOff(pc_usb_handle_note_off);
}

void read_usb_from_computer() {
  while(usbMIDI.read());
}