// a behaviour that will display the current held notes, for helping with debugging midi input and output.#pragma once

#pragma once

#include <Arduino.h>
#include "Config.h"

#include "behaviours/behaviour_base.h"

#include "midi/midi_mapper_matrix_manager.h"

#include "mymenu/menu_midioutputwrapper.h"

#if defined(ENABLE_SCALES)
    #include "mymenu/menuitems_scale.h"
    #include "mymenu/menuitems_notedisplay.h"
    #include "chord_player.h"
#endif

extern MIDIMatrixManager *midi_matrix_manager;

class VirtualBehaviour_Displayer : virtual public VirtualBehaviourBase {
    public:   
    
    VirtualBehaviour_Displayer() : DeviceBehaviourUltimateBase() {
    }
        
    /*
    target_id_t target_id_2 = -1;
    NoteTracker note_tracker[MIDI_MAX_CHANNEL];
    virtual bool note_tracker_foreach_note(NoteTracker::foreach_func_def func) {
        bool ret = false;
        for (int i = 0 ; i < MIDI_MAX_CHANNEL; i++) {
            if (note_tracker[i].count_held() > 0) {
                if (note_tracker[i].foreach_note(func))
                    ret = true;
            }
        }
        return ret;
    }
    virtual int8_t note_tracker_get_transposed_note_for(int8_t note) {
        for (int i = 0 ; i < MIDI_MAX_CHANNEL; i++) {
            if (note_tracker[i].is_note_held(note)) {
                return note_tracker[i].get_transposed_note_for(note);
            }
        }
        return NOTE_OFF;
    }
    virtual bool is_note_held(int8_t note, int8_t channel) {
        return note_tracker[channel].is_note_held(note);
    }
    virtual bool note_tracker_held_note_on(int8_t note, int8_t transposed_note, int8_t channel) {
        return note_tracker[channel].held_note_on(note, transposed_note);
    }
    virtual bool note_tracker_held_note_off(int8_t note, int8_t channel) {
        return note_tracker[channel].held_note_off(note);
    }
    virtual int8_t note_tracker_count_held() {
        int count = 0;
        for (int i = 0; i < MIDI_MAX_CHANNEL; i++) {
            count += note_tracker[i].count_held();
        }
        return count;
    }
    virtual const char *note_tracker_get_held_notes_c() {
        // todo: hmm what should we do here?  list all?
        // what else?  aggregate and count?
        // list sequentially?
        // for now, just return the first non-empty one?
        for (int i = 0; i < MIDI_MAX_CHANNEL; i++) {
            if (note_tracker[i].count_held() > 0) {
                return note_tracker[i].get_held_notes_c();
            }
        }
        return "nothing";
    }*/

    virtual char *get_label() override {
        return (char*)"Displayer";
    }

    #ifdef ENABLE_SCREEN
    virtual LinkedList<MenuItem*> *make_menu_items() override {
        LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();

        menuitems->add(new NoteDisplay("Notes", &this->note_tracker));
        menuitems->add(new NoteHarmonyDisplay(
            (const char*)"Harmony", 
            &midi_matrix_manager->global_scale_identity.scale_number, 
            &midi_matrix_manager->global_scale_identity.root_note, 
            &this->note_tracker,
            &midi_matrix_manager->global_quantise_on
        ));

        /*
        menuitems->add(new LambdaToggleControl("Debug", 
            [=](bool v) -> void {
                Serial_printf("VirtualBehaviour_Displayer#Debug settings, looking for wrapper with target_id of %i\n", this->target_id);
                Serial_flush();
                MIDIOutputWrapper *wrapper = midi_matrix_manager->get_target_for_id(this->target_id);
                Serial_flush();
                if (wrapper) {
                    Serial_printf("Found wrapper '%s' for target_id %i, setting log_message_mode to %i\n", wrapper->label, this->target_id, v);
                    Serial_flush();
                    wrapper->set_log_message_mode(v);
                    wrapper->debug = v;
                    Serial_printf("Done setting log_message_mode for wrapper '%s'\n", wrapper->label);
                    Serial_flush();
                } else {
                    Serial_printf("WARNING: tried to set log_message_mode for target_id %i, but no wrapper found!\n", this->target_id);
                }
                Serial_flush();
                this->debug = v; 
            },
            [=]() -> bool { return this->debug; }
        ));
        */

        menuitems->add(new MIDIOutputWrapperDebugMenuItem(
            (const char*)"MIDI Output Debug",
            midi_matrix_manager->get_target_for_id(this->target_id)
        ));

        return menuitems;
    }


    #endif
};

extern VirtualBehaviour_Displayer *behaviour_displayer;
