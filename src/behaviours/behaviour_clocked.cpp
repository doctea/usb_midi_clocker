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

        ObjectNumberControl<DividedClockedBehaviour,uint32_t> *divisor_control = new ObjectNumberControl<DividedClockedBehaviour,uint32_t>(
            "Divider",
            //"Subclocker div", 
            this, 
            &DividedClockedBehaviour::set_divisor, 
            &DividedClockedBehaviour::get_divisor, 
            nullptr, // change callback on_subclocker_divisor_changed
            1,  //min
            48  //max
        );

        ObjectSelectorControl<DividedClockedBehaviour,uint32_t> *delay_ticks_control = new ObjectSelectorControl<DividedClockedBehaviour,uint32_t>(
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

        //ObjectToggleControl<DividedClockedBehaviour> *pause_during_delay_control = new ObjectToggleControl<DividedClockedBehaviour>(
        ObjectSelectorControl<DividedClockedBehaviour,byte> *pause_during_delay_control = new ObjectSelectorControl<DividedClockedBehaviour,byte>(
            "Start pause",
            this,
            &DividedClockedBehaviour::set_pause_during_delay,
            &DividedClockedBehaviour::get_pause_during_delay,
            nullptr
        );
        pause_during_delay_control->add_available_value(DELAY_PAUSE::OFF,    "Off");
        pause_during_delay_control->add_available_value(DELAY_PAUSE::BAR,    "Bar");
        pause_during_delay_control->add_available_value(DELAY_PAUSE::TWO_BAR,"2Bar");
        pause_during_delay_control->add_available_value(DELAY_PAUSE::PHRASE, "Phrase");
        
        divisor_control->go_back_on_select = delay_ticks_control->go_back_on_select = true; 

        bar->add(divisor_control);
        bar->add(delay_ticks_control);
        bar->add(auto_restart_control);
        bar->add(pause_during_delay_control);

        menuitems->add(bar);

        if (this->debug) {
            SubMenuItemBar *debugbar2 = new SubMenuItemBar("debug bar");

            debugbar2->add(new NumberControl<uint32_t>("ticks", (uint32_t*)&ticks, (uint32_t)ticks, (uint32_t)0, (uint32_t)2*32, nullptr));
            debugbar2->add(new NumberControl<int32_t>("real_ticks", &real_ticks, (int32_t)real_ticks, (int32_t)0, (int32_t)2*31, nullptr));
            debugbar2->add(new NumberControl<bool>("waiting", &waiting, false, false, true, nullptr));
            menuitems->add(debugbar2);
        }

        ClockedBehaviour::make_menu_items();

        return menuitems;
    }
#endif