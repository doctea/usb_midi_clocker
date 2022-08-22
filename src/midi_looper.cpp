//#include <midi.h>
#include <MIDI.h>

#include "bpm.h"
#include "midi_looper.h"

#include "Config.h"
#include "ConfigMidi.h"
#include "midi_outs.h"

#include "midi_drums.h"

#include "midi_mapper_matrix_manager.h"

// TODO: rewrite all this to work a lot better, ie:-
//      [DONE    >1 event per tick]
//      record as midi file
//      load as midi file
//      [DONE    make stop_all_notes]
//      able to track multiple devices / channels
//      [DONE] transpose
//      [DONE] quantizing (time)
//      quantizing (scale)

/*typedef struct midi_frame {
    //unsigned long time;
    LinkedList<midi_message> messages = LinkedList<midi_message>();
};*/

MIDITrack mpk49_loop_track;// = MIDITrack(); // = MIDITrack(&midi_out_bitbox_wrapper); //&MIDIOutputWrapper(midi_out_bitbox, 3));

//MIDIOutputWrapper midi_drums_output = MIDIOutputWrapper((char*)"Bamble drums", &midi_bamble, 10);
MIDITrack drums_loop_track = MIDITrack(); //&midi_drums_output); //midi_bamble);

/*void stop_all_notes() {
    mpk49_loop_track.stop_all_notes();
}

void clear_recording() {
    mpk49_loop_track.stop_all_notes();
    mpk49_loop_track.clear_all();
}*/

// for sending passthrough or recorded noteOns to actual output
void MIDITrack::sendNoteOn(byte pitch, byte velocity, byte channel = 0) {
    midi_matrix_manager->send_note_on(this->source_id, pitch, velocity, channel);
    /*if (output!=nullptr) {
        if (this->debug) Serial.printf("\tsending to output %s\tpitch=%i,\tvel=%i,\tchan=%i\n", output->label);;
        //output->debug = true;
        output->sendNoteOn(pitch, velocity, channel);
        //output->debug = false;
    } else {
        if (this->debug) Serial.println("\tno output?");
    }*/
}
// for sending passthrough or recorded noteOffs to actual output
void MIDITrack::sendNoteOff(byte pitch, byte velocity, byte channel = 0) {
    midi_matrix_manager->send_note_off(this->source_id, pitch, velocity, channel);
    //Serial.printf("sendNoteOff: output is %p, output_deferred is %p, *output_deferred is %p\n", output, output_deferred, *output_deferred);
    /*if (output!=nullptr) 
        output->sendNoteOff(pitch, velocity, channel);*/
}

void MIDITrack::stop_all_notes() {
    midi_matrix_manager->stop_all_notes(this->source_id);
    /*if (output!=nullptr)
        this->output->stop_all_notes();*/
    if (current_note!=-1) 
        last_note = current_note;
    current_note = -1;
}