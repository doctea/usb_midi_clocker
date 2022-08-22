#include "midi_out_wrapper.h"

#include "midi_looper.h"
#include "bpm.h"

MIDIOutputWrapper::MIDIOutputWrapper(const char *label, MIDITrack *looper, byte channel = 1) {
    strcpy(this->label, label);
    output_looper = looper;
    default_channel = channel;
}

void MIDIOutputWrapper::sendNoteOn(byte pitch, byte velocity, byte channel = 0) {
    if (this->debug) Serial.printf("sendNoteOn(p=%i, v=%i, c=%i) in %s...\n", pitch, velocity, channel, label); Serial.flush();
    if (channel==0) channel = default_channel;
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
    if (output_looper!=nullptr) {
        output_looper->in_event(ticks, midi::NoteOn, pitch, velocity);
    }
    //Serial.println("sent NoteOn");
    if (playing_notes[pitch]<8) {
        playing_notes[pitch]++;
    } else {
        if (this->debug) Serial.printf("\talready playing %i notes at pitch %i, so not counting a new one\n", playing_notes[pitch], pitch);
    }
}

void MIDIOutputWrapper::sendNoteOff(byte pitch, byte velocity, byte channel = 0) {
    if (channel==0) channel = default_channel;
    if (output_serialmidi!=nullptr)     output_serialmidi->sendNoteOff(pitch, velocity, channel);
    if (output_usb!=nullptr)            output_usb->sendNoteOff(pitch, velocity, channel);
    if (output_usb_pointer!=nullptr && (*output_usb_pointer)!=nullptr)
                                        (*output_usb_pointer)->sendNoteOff(pitch, velocity, channel);
    if (output_looper!=nullptr) {
        output_looper->in_event(ticks, midi::NoteOff, pitch, velocity);
    }

    if (playing_notes[pitch]>0) playing_notes[pitch]--;
}

void MIDIOutputWrapper::sendControlChange(byte pitch, byte velocity, byte channel = 0) {
    if (channel==0) channel = default_channel;
    if (output_serialmidi!=nullptr)     output_serialmidi->sendControlChange(pitch, velocity, channel);
    if (output_usb!=nullptr)            output_usb->sendControlChange(pitch, velocity, channel);
    if (output_usb_pointer!=nullptr && (*output_usb_pointer)!=nullptr)
                                        (*output_usb_pointer)->sendControlChange(pitch, velocity, channel);
    if (output_looper!=nullptr) {
        output_looper->in_event(ticks, midi::ControlChange, pitch, velocity);
    }
}
