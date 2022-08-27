#include "midi_out_wrapper.h"

#include "midi_looper.h"
#include "bpm.h"

MIDIOutputWrapper::MIDIOutputWrapper(const char *label, MIDITrack *looper, byte channel) : MIDIOutputWrapper(label, channel) {
    output_looper = looper;
}

void MIDIOutputWrapper::actual_sendNoteOn(byte pitch, byte velocity, byte channel) {
    if (output_serialmidi!=nullptr) {
        if (this->debug) Serial.printf("midi_out_wrapper#sendNoteOn %s\tgot an output_serialmidi\n", this->label);
        output_serialmidi->sendNoteOn(pitch, velocity, channel);
    }
    if (output_usb!=nullptr) {
        if (this->debug) Serial.printf("midi_out_wrapper#sendNoteOn %s\tgot an output_usb\tat %p\t[p=%i,\tv=%i,\tc=%i]\n", this->label, this->output_usb, pitch, velocity, channel);
        output_usb->sendNoteOn(pitch, velocity, channel);
    }
    if (output_usb_pointer!=nullptr && (*output_usb_pointer)!=nullptr) {
        if (this->debug) Serial.printf("midi_out_wrapper#sendNoteOn %s\tgot an output_usb_pointer\tat %p\t[p=%i,\tv=%i,\tc=%i]\n", this->label, this->output_usb_pointer, pitch, velocity, channel);
        (*output_usb_pointer)->sendNoteOn(pitch, velocity, channel);
    }
    if (output_looper!=nullptr)         output_looper->in_event(ticks, midi::NoteOn, pitch, velocity);
}

void MIDIOutputWrapper::actual_sendNoteOff(byte pitch, byte velocity, byte channel) {
    if (output_serialmidi!=nullptr)     output_serialmidi->sendNoteOff(pitch, velocity, channel);
    if (output_usb!=nullptr)            output_usb->sendNoteOff(pitch, velocity, channel);
    if (output_usb_pointer!=nullptr && (*output_usb_pointer)!=nullptr)
                                        (*output_usb_pointer)->sendNoteOff(pitch, velocity, channel);
    if (output_looper!=nullptr)         output_looper->in_event(ticks, midi::NoteOff, pitch, velocity);
}

void MIDIOutputWrapper::actual_sendControlChange(byte pitch, byte velocity, byte channel) {
    if (channel==0) channel = default_channel;
    if (output_serialmidi!=nullptr)     output_serialmidi->sendControlChange(pitch, velocity, channel);
    if (output_usb!=nullptr)            output_usb->sendControlChange(pitch, velocity, channel);
    if (output_usb_pointer!=nullptr && (*output_usb_pointer)!=nullptr)
                                        (*output_usb_pointer)->sendControlChange(pitch, velocity, channel);
    if (output_looper!=nullptr) {
        output_looper->in_event(ticks, midi::ControlChange, pitch, velocity);
    }
}
