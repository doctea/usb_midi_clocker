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

        menuitems->add(new LambdaToggleControl("Debug", 
            [=](bool v) -> void { 
                midi_matrix_manager->get_target_for_id(this->target_id)->set_log_message_mode(v);
                this->debug = v; 
            },
            [=]() -> bool { return this->debug; }
        ));

        menuitems->add(new MIDIOutputWrapperDebugMenuItem(
            (const char*)"MIDI Output Debug",
            midi_matrix_manager->get_target_for_id(this->target_id)
        ));

        return menuitems;
    }


    #endif
};

extern VirtualBehaviour_Displayer *behaviour_displayer;
