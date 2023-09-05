#include "Config.h"

#include "behaviours/behaviour_midibass.h"

#ifdef ENABLE_SCREEN
    #include "menu.h"
    #include "menuitems_object.h"
    #include "submenuitem_bar.h"

    #include "mymenu/menuitems_scale.h"

    //#include "mymenu/menu_midioutputwrapper.h"

    FLASHMEM
    LinkedList<MenuItem *> *MIDIBassBehaviour::make_menu_items() {
        LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();
        SubMenuItemBar *bar = new SubMenuItemBar((String(this->get_label()) + String(F(" MIDI bass"))).c_str());

        char output_label[MENU_C_MAX];
        snprintf(output_label, MENU_C_MAX, "%s output", this->get_label());

        // todo: move this and transpose_control into base Behaviour...?
        HarmonyStatus *harmony = new HarmonyStatus(
            output_label, 
            &this->last_transposed_note, 
            &this->current_transposed_note, 
            &this->last_drone_note
        );
        menuitems->add(harmony);
    
        /*ObjectNumberControl<DeviceBehaviourUltimateBase,int> *transpose_control = 
            new ObjectNumberControl<DeviceBehaviourUltimateBase,int>(
                "Octave",
                this, 
                &DeviceBehaviourUltimateBase::setForceOctave, 
                &DeviceBehaviourUltimateBase::getForceOctave, 
                nullptr,
                -1,
                8
            );
        bar->add(transpose_control);*/
        SubMenuItemBar *transposition_bar = new SubMenuItemBar("Transpose");
        ObjectScaleNoteMenuItem<DeviceBehaviourUltimateBase,int8_t> *lowest_note_control = new ObjectScaleNoteMenuItem<DeviceBehaviourUltimateBase,int8_t>(
            "Lowest Note",
            this,
            &DeviceBehaviourUltimateBase::setLowestNote,
            &DeviceBehaviourUltimateBase::getLowestNote,
            nullptr,
            (int8_t)0,
            (int8_t)127,
            true,
            true
        );
        transposition_bar->add(lowest_note_control);

        ObjectScaleNoteMenuItem<DeviceBehaviourUltimateBase,int8_t> *highest_note_control = new ObjectScaleNoteMenuItem<DeviceBehaviourUltimateBase,int8_t>(
            "Highest Note",
            this,
            &DeviceBehaviourUltimateBase::setHighestNote,
            &DeviceBehaviourUltimateBase::getHighestNote,
            nullptr,
            (int8_t)0,
            (int8_t)127,
            true,
            true
        );
        transposition_bar->add(highest_note_control);

        ObjectSelectorControl<DeviceBehaviourUltimateBase,int8_t> *lowest_note_mode_control = new ObjectSelectorControl<DeviceBehaviourUltimateBase,int8_t>(
            "Low Mode",
            this,
            &DeviceBehaviourUltimateBase::setLowestNoteMode,
            &DeviceBehaviourUltimateBase::getLowestNoteMode,
            nullptr,
            true
        );
        lowest_note_mode_control->add_available_value(NOTE_MODE::IGNORE, "Ignore");
        lowest_note_mode_control->add_available_value(NOTE_MODE::TRANSPOSE, "Transpose");
        transposition_bar->add(lowest_note_mode_control);

        ObjectSelectorControl<DeviceBehaviourUltimateBase,int8_t> *highest_note_mode_control = new ObjectSelectorControl<DeviceBehaviourUltimateBase,int8_t>(
            "High Mode",
            this,
            &DeviceBehaviourUltimateBase::setHighestNoteMode,
            &DeviceBehaviourUltimateBase::getHighestNoteMode,
            nullptr,
            true
        );
        highest_note_mode_control->set_available_values(lowest_note_mode_control->available_values);
        transposition_bar->add(highest_note_mode_control);

        menuitems->add(transposition_bar);

        //TODO: see commented-out section in DeviceBehaviour_Neutron
        ObjectToggleControl<MIDIBassBehaviour> *drone_bass = 
            new ObjectToggleControl<MIDIBassBehaviour> (
                "Drone",
                this,
                &MIDIBassBehaviour::set_drone,
                &MIDIBassBehaviour::is_drone,
                nullptr
            );
        bar->add(drone_bass);

        ObjectNumberControl<MIDIBassBehaviour,int8_t> *machinegun_mode = new ObjectNumberControl<MIDIBassBehaviour,int8_t>(
            "Machinegun", 
            this, 
            &MIDIBassBehaviour::set_machinegun, 
            &MIDIBassBehaviour::get_machinegun, 
            nullptr,
            (int8_t)0, 
            (int8_t)4
        );
        bar->add(machinegun_mode);

        menuitems->add(bar);

        #ifdef DEBUG_MIDIBASS
            menuitems->add(new MIDIOutputWrapperDebugMenuItem(
                "Incoming?",
                my_wrapper
                //midi_matrix_manager->get_source
            ));

            menuitems->add(new MIDIOutputWrapperDebugMenuItem(
                "Outgoing?",
                //midi_matrix_manager->get_target_for_handle("S2 : unused : ch 1")
                test_wrapper
            ));
        #endif

        menuitems->add(new ToggleControl<bool>("Debug", &this->debug));

        return menuitems;
    }
#endif