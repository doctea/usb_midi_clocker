#include "behaviours/behaviour_base.h"
#include "midi/midi_mapper_matrix_manager.h"

// called when a receive_note_on message is received from the device
void DeviceBehaviourUltimateBase::receive_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    midi_matrix_manager->processNoteOn(this->source_id, note, velocity); //, channel);
}

// called when a note_off message is received from the device
void DeviceBehaviourUltimateBase::receive_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    midi_matrix_manager->processNoteOff(this->source_id, note, velocity); //, channel);
}

// called when a receive_control_change message is received from the device
void DeviceBehaviourUltimateBase::receive_control_change (uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    midi_matrix_manager->processControlChange(this->source_id, inNumber, inValue); //, inChannel);
}


#ifdef ENABLE_SCREEN
    #include "menu.h"

    LinkedList<MenuItem*> ClockedBehaviour::make_menu_items() {
        LinkedList<MenuItem*> menuitems = DeviceBehaviourUltimateBase::make_menu_items();
        if (this->should_show_restart_option()) {
            String restart_label = String("Restart " + String(this->get_label()) + " on bar");

            ObjectActionItem<ClockedBehaviour> *restart_action = new ObjectActionItem<ClockedBehaviour>(
                restart_label.c_str(),
                this,
                &ClockedBehaviour::set_restart_on_bar,
                &ClockedBehaviour::is_set_restart_on_bar,
                "Restarting.."
            );

            menuitems.add(restart_action);
        }

        return menuitems;
    }

    #include "submenuitem_bar.h"

    LinkedList<MenuItem*> DividedClockedBehaviour::make_menu_items() {
        LinkedList<MenuItem*> menuitems = ClockedBehaviour::make_menu_items();
        String bar_label = String(this->get_label()) + String(" Clock");
        SubMenuItemBar *bar = new SubMenuItemBar(bar_label.c_str());

        ObjectNumberControl<DividedClockedBehaviour,int> *divisor_control = new ObjectNumberControl<DividedClockedBehaviour,int>(
            "Divider",
            //"Subclocker div", 
            this, 
            &DividedClockedBehaviour::set_divisor, 
            &DividedClockedBehaviour::get_divisor, 
            nullptr, // change callback on_subclocker_divisor_changed
            1,  //min
            48  //max
        );
        ObjectNumberControl<DividedClockedBehaviour,int> *delay_ticks_control = new ObjectNumberControl<DividedClockedBehaviour,int>(
            "Delay",
            this,
            &DividedClockedBehaviour::set_delay_ticks,
            &DividedClockedBehaviour::get_delay_ticks,
            nullptr,
            0,
            PPQN * BEATS_PER_BAR * BARS_PER_PHRASE
        );

        divisor_control->go_back_on_select = delay_ticks_control->go_back_on_select = true; 

        bar->add(divisor_control);
        bar->add(delay_ticks_control);
        menuitems.add(bar);

        return menuitems;
    }
#endif