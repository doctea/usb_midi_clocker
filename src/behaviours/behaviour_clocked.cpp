#include "behaviours/behaviour_clocked.h"

#include "arrangement/arrangement.h"

ArrangementTrackBase *ClockedBehaviour::create_arrangement_track() {
    ArrangementSingleTrack *track = new ArrangementSingleTrack(this->get_label(), this->colour);
    return track;
    //return nullptr;
}

#ifdef ENABLE_SCREEN
    #include "menu.h"

    FLASHMEM LinkedList<MenuItem*> *ClockedBehaviour::make_menu_items() {
        LinkedList<MenuItem*> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();
        if (this->should_show_restart_option()) {
            String restart_label = String(F("Restart ") + String(this->get_label()) + F(" on bar"));

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
        //Serial.println(F("\tDividedClockedBehaviour calling DeviceBehaviourUltimateBase::make_menu_items()")); Serial_flush();
        DeviceBehaviourUltimateBase::make_menu_items();

        String bar_label = String(this->get_label()) + String(F(" Clock"));
        SubMenuItemBar *bar = new SubMenuItemBar(bar_label.c_str());

        //Serial.println(F("\tDividedClockedBehaviour creating divisor_control")); Serial_flush();

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
        divisor_control->go_back_on_select = true;

        #define ENABLE_DELAY_TICKS_CONTROL
        #define ENABLE_AUTO_RESTART_CONTROL
        #define ENABLE_PAUSE_DURING_DELAY_CONTROL

        #ifdef ENABLE_DELAY_TICKS_CONTROL
            //Serial.println(F("\tDividedClockedBehaviour creating delay_ticks_control")); Serial_flush();
            ObjectSelectorControl<DividedClockedBehaviour,int32_t> *delay_ticks_control = new ObjectSelectorControl<DividedClockedBehaviour,int32_t>(
                "Delay",
                this,
                &DividedClockedBehaviour::set_delay_ticks,
                &DividedClockedBehaviour::get_delay_ticks,
                nullptr
            );       
            delay_ticks_control->add_available_value(0,                 "None");
            delay_ticks_control->add_available_value(PPQN/4,            "1/4");
            delay_ticks_control->add_available_value(PPQN/2,            "1/2");
            delay_ticks_control->add_available_value(PPQN,              "1");
            delay_ticks_control->add_available_value(PPQN+(PPQN/2),     "1.5");
            delay_ticks_control->add_available_value(PPQN*2,            "2");
            delay_ticks_control->add_available_value((PPQN*2)+(PPQN/2), "2.5");
            delay_ticks_control->add_available_value(PPQN*3,            "3");
            delay_ticks_control->add_available_value((PPQN*3)+(PPQN/2), "3.5");
            delay_ticks_control->add_available_value(PPQN*4,            "4");
            delay_ticks_control->add_available_value((PPQN*4)+(PPQN/2), "4.5");
            delay_ticks_control->add_available_value(PPQN*5,            "5");
            delay_ticks_control->add_available_value((PPQN*5)+(PPQN/2), "5.5");
            delay_ticks_control->add_available_value(PPQN*6,            "6");
            delay_ticks_control->add_available_value((PPQN*6)+(PPQN/2), "6.5");
            delay_ticks_control->add_available_value(PPQN*7,            "7");
            delay_ticks_control->add_available_value((PPQN*7)+(PPQN/2), "7.5");
            delay_ticks_control->add_available_value(PPQN*8,            "8");
            delay_ticks_control->add_available_value((PPQN*8)+(PPQN/2), "8.5");
            delay_ticks_control->add_available_value(PPQN*9,            "9");
            delay_ticks_control->add_available_value((PPQN*9)+(PPQN/2), "9.5");
            delay_ticks_control->add_available_value(PPQN*10,            "10");
            delay_ticks_control->add_available_value((PPQN*10)+(PPQN/2), "10.5");
            delay_ticks_control->add_available_value(PPQN*11,            "11");
            delay_ticks_control->add_available_value((PPQN*11)+(PPQN/2), "11.5");
            delay_ticks_control->add_available_value(PPQN*12,            "12");
            delay_ticks_control->add_available_value((PPQN*12)+(PPQN/2), "12.5");
            delay_ticks_control->add_available_value(PPQN*13,            "13");
            delay_ticks_control->add_available_value((PPQN*13)+(PPQN/2), "13.5");
            delay_ticks_control->add_available_value(PPQN*14,            "14");
            delay_ticks_control->add_available_value((PPQN*4)+(PPQN/2), "14.5");
            delay_ticks_control->add_available_value(PPQN*15,            "15");
            delay_ticks_control->add_available_value((PPQN*15)+(PPQN/2), "15.5");
            //delay_ticks_control->add_available_value(PPQN*16,            "16");
            //delay_ticks_control->add_available_value((PPQN*16)+(PPQN/2), "16.5");

            delay_ticks_control->go_back_on_select = true;
        #endif

        #ifdef ENABLE_AUTO_RESTART_CONTROL
            //Serial.println(F("\tDividedClockedBehaviour creating auto_restart_control")); Serial_flush();
            ObjectToggleControl<DividedClockedBehaviour> *auto_restart_control = new ObjectToggleControl<DividedClockedBehaviour>(
                "Restart",
                this,
                &DividedClockedBehaviour::set_auto_restart_on_change,
                &DividedClockedBehaviour::should_auto_restart_on_change,
                nullptr
            );
            //auto_restart_control->debug = true;
        #endif

        #ifdef ENABLE_PAUSE_DURING_DELAY_CONTROL
            //Serial.println(F("\tDividedClockedBehaviour creating pause_during_delay_control")); Serial_flush();
            ObjectSelectorControl<DividedClockedBehaviour,int8_t> *pause_during_delay_control = new ObjectSelectorControl<DividedClockedBehaviour,int8_t>(
                "Pause",
                this,
                &DividedClockedBehaviour::set_pause_during_delay,
                &DividedClockedBehaviour::get_pause_during_delay,
                nullptr
            );
            pause_during_delay_control->add_available_value(DELAY_PAUSE::PAUSE_OFF,    "Off");
            pause_during_delay_control->add_available_value(DELAY_PAUSE::PAUSE_BAR,    "Bar");
            pause_during_delay_control->add_available_value(DELAY_PAUSE::PAUSE_TWO_BAR,"2Bar");
            pause_during_delay_control->add_available_value(DELAY_PAUSE::PAUSE_PHRASE, "Phrs");
            pause_during_delay_control->add_available_value(DELAY_PAUSE::PAUSE_FINAL_PHRASE, "FiPh");
            pause_during_delay_control->go_back_on_select = true;
        #endif
        
        bar->add(divisor_control);
        #ifdef ENABLE_DELAY_TICKS_CONTROL
            bar->add(delay_ticks_control);
        #endif
        #ifdef ENABLE_AUTO_RESTART_CONTROL
            bar->add(auto_restart_control);
        #endif
        #ifdef ENABLE_PAUSE_DURING_DELAY_CONTROL
            bar->add(pause_during_delay_control);
        #endif

        menuitems->add(bar);

        /*if (this->debug) {
            SubMenuItemBar *debugbar2 = new SubMenuItemBar("debug bar");
            //debugbar2->add(new NumberControl<uint32_t>("ticks", (uint32_t*)&ticks, (uint32_t)ticks, (uint32_t)0, (uint32_t)2*32, nullptr));
            //debugbar2->add(new NumberControl<int32_t>("real_ticks", &real_ticks, (int32_t)real_ticks, (int32_t)0, (int32_t)2*31, nullptr));
            // ^^ adding this sends memory usage over the limit, wtf?
            debugbar2->add(new NumberControl<bool>("waiting", &waiting, false, false, true, nullptr));
            menuitems->add(debugbar2);
        }*/

        //NumberControl(const char* label, DataType *target_variable, DataType start_value, DataType min_value, DataType max_value, void (*on_change_handler)(DataType last_value, DataType new_value)) 

        /*if (this->debug) {
            menuitems->add(new NumberControl<bool>("Waiting?", &this->waiting, false, false, true, nullptr));
            menuitems->add(new NumberControl<bool>("Started?", &this->started, false, false, true, nullptr));
        }*/

        /*menuitems->add(
            new NumberControl<uint32_t>
                ("Queued divisor", &this->queued_clock_divisor, this->queued_clock_divisor, (uint32_t)0, (uint32_t)16, nullptr)
        );*/

        //Serial.println(F("\tDividedClockedBehaviour calling ClockedBehaviour::make_menu_items()")); Serial_flush();        
        ClockedBehaviour::make_menu_items();

        return menuitems;
    }
#endif