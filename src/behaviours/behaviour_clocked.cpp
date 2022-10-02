#include "behaviours/behaviour_clocked.h"

#ifdef ENABLE_SCREEN
    #include "menu.h"

    LinkedList<MenuItem*> *ClockedBehaviour::make_menu_items() {
        DeviceBehaviourUltimateBase::make_menu_items();
        if (this->should_show_restart_option()) {
            String restart_label = String("Restart " + String(this->get_label()) + " on bar");

            ObjectActionItem<ClockedBehaviour> *restart_action = new ObjectActionItem<ClockedBehaviour>(
                restart_label.c_str(),
                this,
                &ClockedBehaviour::set_restart_on_bar,
                &ClockedBehaviour::is_set_restart_on_bar,
                "Restarting.."
            );

            menuitems->add(restart_action);
        }

        return menuitems;
    }

    #include "submenuitem_bar.h"
    //#include "mymenu/menu_delayticks.h"
    #include "menuitems.h"
    #include "menuitems_selector.h"

    LinkedList<MenuItem*> *DividedClockedBehaviour::make_menu_items() {
        DeviceBehaviourUltimateBase::make_menu_items();

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

        ObjectSelectorControl<DividedClockedBehaviour,int> *delay_ticks_control = new ObjectSelectorControl<DividedClockedBehaviour,int>(
            "Delay (beats)",
            this,
            &DividedClockedBehaviour::set_delay_ticks,
            &DividedClockedBehaviour::get_delay_ticks,
            nullptr
        );       
        delay_ticks_control->add_available_value(0,     "None");
        delay_ticks_control->add_available_value(PPQN/4,"1/4");
        delay_ticks_control->add_available_value(PPQN/2,"1/2");
        delay_ticks_control->add_available_value(PPQN,  "1");
        delay_ticks_control->add_available_value(PPQN*2,"2");
        delay_ticks_control->add_available_value(PPQN*3,"3");
        delay_ticks_control->add_available_value(PPQN*4,"4");
        delay_ticks_control->add_available_value(PPQN*5,"5");
        delay_ticks_control->add_available_value(PPQN*6,"6");
        delay_ticks_control->add_available_value(PPQN*7,"7");
        delay_ticks_control->add_available_value(PPQN*8,"8");

        ObjectToggleControl<DividedClockedBehaviour> *auto_restart_control = new ObjectToggleControl<DividedClockedBehaviour>(
            "Auto-restart",
            this,
            &DividedClockedBehaviour::set_auto_restart_on_change,
            &DividedClockedBehaviour::should_auto_restart_on_change,
            nullptr
        );

        divisor_control->go_back_on_select = delay_ticks_control->go_back_on_select = true; 

        bar->add(divisor_control);
        bar->add(delay_ticks_control);
        bar->add(auto_restart_control);
        menuitems->add(bar);

        ClockedBehaviour::make_menu_items();

        return menuitems;
    }
#endif