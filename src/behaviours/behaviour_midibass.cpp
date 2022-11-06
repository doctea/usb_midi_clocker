#include "Config.h"

#include "behaviours/behaviour_midibass.h"

#ifdef ENABLE_SCREEN
    #include "menu.h"
    #include "menuitems_object.h"
    #include "submenuitem_bar.h"

    FLASHMEM
    LinkedList<MenuItem *> *MIDIBassBehaviour::make_menu_items() {
        LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();
        SubMenuItemBar *bar = new SubMenuItemBar((String(this->get_label()) + String(F(" MIDI bass"))).c_str());

        //Serial.printf("############# MIDIBassBehaviour::make_menu_items looking up target_id %i!!!\n", this->target_id);
        MIDIOutputWrapper *my_wrapper = midi_matrix_manager->get_target_for_id(this->target_id);

        HarmonyStatus *neutron_harmony = new HarmonyStatus(
            "Neutron output", 
            &my_wrapper->last_transposed_note, 
            &my_wrapper->current_transposed_note, 
            &this->last_drone_note
        );

        ObjectNumberControl<MIDIOutputWrapper,int> *neutron_transpose_control = 
            new ObjectNumberControl<MIDIOutputWrapper,int>(
                "Octave",
                my_wrapper, 
                &MIDIOutputWrapper::setForceOctave, 
                &MIDIOutputWrapper::getForceOctave, 
                nullptr,
                -1,
                8
            );
        
        //TODO: see commented-out section in DeviceBehaviour_Neutron
        ObjectToggleControl<MIDIBassBehaviour> *neutron_drone_bass = 
            new ObjectToggleControl<MIDIBassBehaviour> (
                "Drone",
                this,
                &MIDIBassBehaviour::set_drone,
                &MIDIBassBehaviour::is_drone,
                nullptr
            );

        menuitems->add(neutron_harmony);

        bar->add(neutron_transpose_control);         
        bar->add(neutron_drone_bass);
        menuitems->add(bar);
        return menuitems;
    }
#endif