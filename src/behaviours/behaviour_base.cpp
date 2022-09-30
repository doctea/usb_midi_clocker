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
    //#include "menuitems_numbers.h"
    LinkedList<MenuItem*> DividedClockedBehaviour::make_menu_items() {
        LinkedList<MenuItem*> menuitems = ClockedBehaviour::make_menu_items();

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
        ObjectActionItem<DividedClockedBehaviour> *restart_action = new ObjectActionItem<DividedClockedBehaviour>(
            "Restart on bar",
            this,
            &DividedClockedBehaviour::set_restart_on_bar,
            &DividedClockedBehaviour::is_set_restart_on_bar,
            "Restarting.."
        );

        divisor_control->go_back_on_select = delay_ticks_control->go_back_on_select = true; 
        /*restart_action.target_object = 
            divisor_control.target_object = 
                delay_ticks_control.target_object = 
                    behaviour_subclocker;   // because behaviour_subclocker pointer won't be set before now..?*/

        menuitems.add(divisor_control);
        menuitems.add(delay_ticks_control);
        menuitems.add(restart_action);

        return menuitems;
    }
#endif