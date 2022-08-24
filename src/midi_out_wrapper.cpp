#include "midi_out_wrapper.h"

#include "midi_looper.h"
#include "bpm.h"

MIDIOutputWrapper::MIDIOutputWrapper(const char *label, MIDITrack *looper, byte channel) : MIDIOutputWrapper(label, channel) {
    output_looper = looper;
}

void MIDIOutputWrapper::sendNoteOn(byte in_pitch, byte velocity, byte channel) {
    if (this->debug) 
        Serial.printf("sendNoteOn(p=%i, v=%i, c=%i) in %s...\n", in_pitch, velocity, channel, label); Serial.flush();

    current_note = in_pitch;
    int pitch = recalculate_pitch(in_pitch);
    if (pitch<0 || pitch>127) return;

    if (playing_notes[pitch]<8) {
        if (this->debug) Serial.printf("\tplaying_notes[%i] is already %i -- increasing by 1\n", pitch, playing_notes[pitch]);
        playing_notes[pitch]++;
    } else {
        if (this->debug) 
            Serial.printf("\talready playing %i notes at pitch %i, so not counting a new one\n", playing_notes[pitch], pitch);
    }

    current_transposed_note = pitch;

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
    if (output_looper!=nullptr)         output_looper->in_event(ticks, midi::NoteOn, pitch, velocity);
    //Serial.println("sent NoteOn");

}

void MIDIOutputWrapper::sendNoteOff(byte in_pitch, byte velocity, byte channel) {

    this->last_note = in_pitch;
    if (this->current_note==in_pitch) 
        current_note = -1;

    int pitch = recalculate_pitch(in_pitch);

    if (this->debug) Serial.printf("MIDIOutputWrapper:sendNoteOff(%i, %i, %i) current count is %i\n", pitch, velocity, channel, playing_notes[pitch]);

    if (pitch<0 || pitch>127) return;
    if (playing_notes[pitch]>0) playing_notes[pitch]--;
    if (playing_notes[pitch]!=0) return;

    this->last_transposed_note = pitch;
    if (this->current_transposed_note==pitch)
        current_transposed_note = -1;

    if (channel==0) channel = default_channel;  
    if (output_serialmidi!=nullptr)     output_serialmidi->sendNoteOff(pitch, velocity, channel);
    if (output_usb!=nullptr)            output_usb->sendNoteOff(pitch, velocity, channel);
    if (output_usb_pointer!=nullptr && (*output_usb_pointer)!=nullptr)
                                        (*output_usb_pointer)->sendNoteOff(pitch, velocity, channel);
    if (output_looper!=nullptr)         output_looper->in_event(ticks, midi::NoteOff, pitch, velocity);
}

void MIDIOutputWrapper::sendControlChange(byte pitch, byte velocity, byte channel) {
    if (channel==0) channel = default_channel;
    if (output_serialmidi!=nullptr)     output_serialmidi->sendControlChange(pitch, velocity, channel);
    if (output_usb!=nullptr)            output_usb->sendControlChange(pitch, velocity, channel);
    if (output_usb_pointer!=nullptr && (*output_usb_pointer)!=nullptr)
                                        (*output_usb_pointer)->sendControlChange(pitch, velocity, channel);
    if (output_looper!=nullptr) {
        output_looper->in_event(ticks, midi::ControlChange, pitch, velocity);
    }
}
