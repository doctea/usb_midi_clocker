#include "midi/midi_out_wrapper.h"

#include "midi/midi_looper.h"
#include "bpm.h"

/*
MIDIOutputWrapper::MIDIOutputWrapper(const char *label, MIDITrack *looper, int8_t channel) : MIDIOutputWrapper(label, channel) {
    output_looper = looper;
}

void MIDIOutputWrapper::actual_sendNoteOn(int8_t pitch, int8_t velocity, int8_t channel) {
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

void MIDIOutputWrapper::actual_sendNoteOff(int8_t pitch, int8_t velocity, int8_t channel) {
    if (output_serialmidi!=nullptr)     output_serialmidi->sendNoteOff(pitch, velocity, channel);
    if (output_usb!=nullptr)            output_usb->sendNoteOff(pitch, velocity, channel);
    if (output_usb_pointer!=nullptr && (*output_usb_pointer)!=nullptr)
                                        (*output_usb_pointer)->sendNoteOff(pitch, velocity, channel);
    if (output_looper!=nullptr)         output_looper->in_event(ticks, midi::NoteOff, pitch, velocity);
}

void MIDIOutputWrapper::actual_sendControlChange(int8_t pitch, int8_t velocity, int8_t channel) {
    if (channel==0) channel = default_channel;
    if (output_serialmidi!=nullptr)     output_serialmidi->sendControlChange(pitch, velocity, channel);
    if (output_usb!=nullptr)            output_usb->sendControlChange(pitch, velocity, channel);
    if (output_usb_pointer!=nullptr && (*output_usb_pointer)!=nullptr)
                                        (*output_usb_pointer)->sendControlChange(pitch, velocity, channel);
    if (output_looper!=nullptr) {
        output_looper->in_event(ticks, midi::ControlChange, pitch, velocity);
    }
}*/

#ifdef ENABLE_LOOPER
void MIDIOutputWrapper_LoopTrack::actual_sendNoteOn(int8_t pitch, int8_t velocity, int8_t channel) {
    output->in_event(ticks, midi::NoteOn, pitch, velocity); //, channel);
}

void MIDIOutputWrapper_LoopTrack::actual_sendNoteOff(int8_t pitch, int8_t velocity, int8_t channel) {  
    output->in_event(ticks, midi::NoteOff, pitch, velocity); //, channel);
}

void MIDIOutputWrapper_LoopTrack::actual_sendControlChange(int8_t number, int8_t value, int8_t channel) {
    //TODO: implement loop track CC recording
    //output->sendControlChange(pitch, velocity, channel);
    output->in_event(ticks, midi::ControlChange, number, value); //, channel);
}
#endif


#ifdef ENABLE_LOOPER
FLASHMEM MIDIOutputWrapper *make_midioutputwrapper(const char *label, MIDITrack *output, int8_t channel) {
    return new MIDIOutputWrapper_LoopTrack(label, output, channel);
}
#endif
FLASHMEM MIDIOutputWrapper *make_midioutputwrapper(const char *label, MIDIDeviceBase *output, int8_t channel) {
    return new MIDIOutputWrapper_MIDIUSB(label, output, channel);
}
FLASHMEM MIDIOutputWrapper *make_midioutputwrapper(const char *label, midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output, int8_t channel) {
    Debug_printf("before making a new serial midi wrapper, free ram: %i\n", freeRam());
    return new MIDIOutputWrapper_MIDISerial(label, output, channel);
}
FLASHMEM MIDIOutputWrapper *make_midioutputwrapper_pcusb(const char *label, int8_t cable_number, int8_t channel) {
    return new MIDIOutputWrapper_PC(label, cable_number, channel);
}
FLASHMEM MIDIOutputWrapper *make_midioutputwrapper(const char *label, DeviceBehaviourUltimateBase *behaviour, int8_t channel) {
    /*MIDIOutputWrapper_Behaviour * v = new MIDIOutputWrapper_Behaviour(label, behaviour, channel);
    behaviour->wrapper = v;*/
    return new MIDIOutputWrapper_Behaviour(label, behaviour, channel);
}

MIDIOutputWrapper::~MIDIOutputWrapper() {}