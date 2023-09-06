#include "Config.h"

#include "behaviours/behaviour_midibass.h"

#ifdef ENABLE_SCREEN
    #include "menu.h"
    #include "menuitems_object.h"
    #include "submenuitem_bar.h"

    //#include "mymenu/menu_midioutputwrapper.h"

    FLASHMEM
    LinkedList<MenuItem *> *MIDIBassBehaviour::make_menu_items() {
        LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();

        SubMenuItemBar *bar = new SubMenuItemBar((String(this->get_label()) + String(F(" Note limits"))).c_str());

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