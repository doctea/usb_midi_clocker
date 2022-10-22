//#include <midi.h>
#include <MIDI.h>

#include "bpm.h"
#include "midi/midi_looper.h"

#include "Config.h"
#include "ConfigMidi.h"
#include "midi/midi_outs.h"

#include "midi/midi_drums.h"

#include "midi/midi_mapper_matrix_manager.h"

// TODO: rewrite all this to work a lot better, ie:-
//      [DONE    >1 event per tick]
//      record as midi file
//      load as midi file
//      [DONE    make stop_all_notes]
//      able to track multiple devices / channels
//      [DONE] transpose
//      [DONE] quantizing (time)
//      quantizing (scale)

MIDITrack mpk49_loop_track;// = MIDITrack(); // = MIDITrack(&midi_out_bitbox_wrapper); //&MIDIOutputWrapper(midi_out_bitbox, 3));

#ifdef ENABLE_DRUM_LOOPER
    MIDITrack drums_loop_track = MIDITrack(); //&midi_drums_output); //midi_bamble);
#endif

// for sending passthrough or recorded noteOns to actual output
void MIDITrack::sendNoteOn(byte pitch, byte velocity, byte channel) {
    midi_matrix_manager->processNoteOn(this->source_id, pitch, velocity); //, channel);
}
// for sending passthrough or recorded noteOffs to actual output
void MIDITrack::sendNoteOff(byte pitch, byte velocity, byte channel) {
    midi_matrix_manager->processNoteOff(this->source_id, pitch, velocity); //, channel);
}

void MIDITrack::stop_all_notes() {
    midi_matrix_manager->stop_all_notes_for_source(this->source_id);

    if (current_note!=-1) 
        last_note = current_note;
    current_note = -1;
}


#ifdef ENABLE_SCREEN
    #include "mymenu/menu_looper.h"
    LinkedList<MenuItem*> *MIDITrack::make_menu_items() {

        LinkedList<MenuItem*> *menuitems = new LinkedList<MenuItem*>();

        LooperStatus            *mpk49_looper_status =       new LooperStatus("Looper", this);
        LooperQuantizeControl   *quantizer_setting =         new LooperQuantizeControl("Loop quantisation", this);   
        LooperTransposeControl  *looper_transpose_control =  new LooperTransposeControl("Loop transpose", this);

        menuitems->add(new SeparatorMenuItem("Looper"));
        menuitems->add(mpk49_looper_status); 
        menuitems->add(quantizer_setting);       // todo: make this part of the LooperStatus object..? (maybe not as it allows interaction)
        //menu->add(&looper_output_selector);
        menuitems->add(looper_transpose_control);

        return menuitems;
    }
#endif