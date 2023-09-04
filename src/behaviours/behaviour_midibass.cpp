#include "Config.h"

#include "behaviours/behaviour_midibass.h"

#ifdef ENABLE_SCREEN
    #include "menu.h"
    #include "menuitems_object.h"
    #include "submenuitem_bar.h"

    #include "mymenu/menu_midioutputwrapper.h"

    FLASHMEM
    LinkedList<MenuItem *> *MIDIBassBehaviour::make_menu_items() {
        LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();
        SubMenuItemBar *bar = new SubMenuItemBar((String(this->get_label()) + String(F(" MIDI bass"))).c_str());

        //Serial.printf("############# MIDIBassBehaviour::make_menu_items looking up target_id %i!!!\n", this->target_id);
        // hmmm todo: this doesn't update if the target is changed...? 
        MIDIOutputWrapper *my_wrapper = midi_matrix_manager->get_target_for_id(this->target_id);

        char output_label[40];
        snprintf(output_label, 40, "%s output", this->get_label());

        if (my_wrapper!=nullptr) {
            HarmonyStatus *harmony = new HarmonyStatus(
                output_label, 
                &this->last_transposed_note, 
                &this->current_transposed_note, 
                &this->last_drone_note
            );

            ObjectNumberControl<MIDIBassBehaviour,int> *transpose_control = 
                new ObjectNumberControl<MIDIBassBehaviour,int>(
                    "Octave",
                    this, 
                    &MIDIBassBehaviour::setForceOctave, 
                    &MIDIBassBehaviour::getForceOctave, 
                    nullptr,
                    -1,
                    8
                );

            bar->add(transpose_control);         

            menuitems->add(harmony);
        
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

        }
        NumberControl<int8_t> *machinegun_mode = new NumberControl<int8_t>("Machinegun", &machinegun, machinegun, 0, 4);
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