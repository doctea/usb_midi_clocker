#include "behaviours/behaviour_clocked.h"

#include "arrangement/arrangement.h"

ArrangementTrackBase *ClockedBehaviour::create_arrangement_track() {
    ArrangementSingleTrack *track = new ArrangementSingleTrack(this->get_label(), this->colour);
    return track;
    //return nullptr;
}

/*void ClockedBehaviour::send_clock(uint32_t ticks) {
    if (!is_connected()) return;
    this->sendRealTime(midi::Clock);
    this->sendNow();
}

void ClockedBehaviour::on_bar(int bar_number) {
    if (this->restart_on_bar) {
        //Serial.printf(F("%s:\tClockedBehaviour#on_bar and restart_on_bar set!\n"), this->get_label());
        this->restart_on_bar = false;
        this->on_restart();
    }
}
bool ClockedBehaviour::is_set_restart_on_bar() {
    return this->restart_on_bar;
}
void ClockedBehaviour::set_restart_on_bar(bool v = true) {
    this->restart_on_bar = v;
}
const char *ClockedBehaviour::get_restart_on_bar_status_label(bool value) {
    if (value) 
        return "Restarting on bar..";
    else 
        return "Trigger restart on bar";
}*/

void ClockedBehaviour::on_restart() {
    if (!is_connected()) return;

    if (this->clock_enabled) {
        this->sendRealTime(midi::Stop);
        this->sendRealTime(midi::Start);
        this->sendNow();
        this->started = true;
    }
}

#ifdef ENABLE_SCREEN
    #include "menu.h"

    #include "submenuitem_bar.h"
    //#include "mymenu/menu_delayticks.h"
    #include "menuitems.h"
    #include "menuitems_selector.h"
    #include "menuitems_lambda.h"
    #include "menuitems_lambda_selector.h"

    FLASHMEM LinkedList<MenuItem*> *ClockedBehaviour::make_menu_items() {
        LinkedList<MenuItem*> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();
        if (this->should_show_restart_option()) {
            String restart_label = String(F("Restart ") + String(this->get_label()) + F(" on bar"));

            LambdaActionItem *restart_action = new LambdaActionItem(
                restart_label.c_str(),
                [=]() -> void { this->set_restart_on_bar(true); },
                [=]() -> bool { return this->is_set_restart_on_bar(); },
                "Restarting.."
            );

            menuitems->add(restart_action);
        }

        return menuitems;
    }

    LinkedList<LambdaSelectorControl<int32_t>::option> *delay_ticks_control_available_values = nullptr;

    void add_option_delay_ticks_control(LinkedList<LambdaSelectorControl<int32_t>::option> *list, int32_t value, const char *label) { 
        String *l = new String(label);
        list->add(LambdaSelectorControl<int32_t>::option { .value = value, .label = l->c_str() });
    }

    LinkedList<MenuItem*> *DividedClockedBehaviour::make_menu_items() {
        //Serial.println(F("\tDividedClockedBehaviour calling DeviceBehaviourUltimateBase::make_menu_items()")); Serial_flush();
        DeviceBehaviourUltimateBase::make_menu_items();

        String bar_label = String(this->get_label()) + String(F(" Clock"));
        SubMenuItemBar *bar = new SubMenuItemBar(bar_label.c_str());

        //Serial.println(F("\tDividedClockedBehaviour creating divisor_control")); Serial_flush();
        LambdaNumberControl<uint32_t> *divisor_control = new LambdaNumberControl<uint32_t>(
            "Divider",
            //"Subclocker div", 
            [=](uint32_t v) -> void { this->set_divisor(v); },
            [=]() -> uint32_t { return this->get_divisor(); },
            nullptr, // change callback on_subclocker_divisor_changed
            1,  //min
            48  //max
        );
        divisor_control->go_back_on_select = true;

        #define ENABLE_DELAY_TICKS_CONTROL
        #define ENABLE_AUTO_RESTART_CONTROL
        #define ENABLE_PAUSE_DURING_DELAY_CONTROL

        #ifdef ENABLE_DELAY_TICKS_CONTROL
            // use a global delay_ticks_control list of available values to save (hopefully) code space and RAM
            if (delay_ticks_control_available_values==nullptr) {
                delay_ticks_control_available_values = new LinkedList<LambdaSelectorControl<int32_t>::option>();
                add_option_delay_ticks_control(delay_ticks_control_available_values, 0,                "None");
                add_option_delay_ticks_control(delay_ticks_control_available_values,PPQN/4,            "1/4");
                add_option_delay_ticks_control(delay_ticks_control_available_values,PPQN/2,            "1/2");
                add_option_delay_ticks_control(delay_ticks_control_available_values,PPQN,              "1");
                add_option_delay_ticks_control(delay_ticks_control_available_values,PPQN+(PPQN/2),     "1.5");
                add_option_delay_ticks_control(delay_ticks_control_available_values,PPQN*2,            "2");
                add_option_delay_ticks_control(delay_ticks_control_available_values,(PPQN*2)+(PPQN/2), "2.5");
                add_option_delay_ticks_control(delay_ticks_control_available_values,PPQN*3,            "3");
                add_option_delay_ticks_control(delay_ticks_control_available_values,(PPQN*3)+(PPQN/2), "3.5");
                add_option_delay_ticks_control(delay_ticks_control_available_values,PPQN*4,            "4");
                add_option_delay_ticks_control(delay_ticks_control_available_values,(PPQN*4)+(PPQN/2), "4.5");
                add_option_delay_ticks_control(delay_ticks_control_available_values,PPQN*5,            "5");
                add_option_delay_ticks_control(delay_ticks_control_available_values,(PPQN*5)+(PPQN/2), "5.5");
                add_option_delay_ticks_control(delay_ticks_control_available_values,PPQN*6,            "6");
                add_option_delay_ticks_control(delay_ticks_control_available_values,(PPQN*6)+(PPQN/2), "6.5");
                add_option_delay_ticks_control(delay_ticks_control_available_values,PPQN*7,            "7");
                add_option_delay_ticks_control(delay_ticks_control_available_values,(PPQN*7)+(PPQN/2), "7.5");
                add_option_delay_ticks_control(delay_ticks_control_available_values,PPQN*8,            "8");
                add_option_delay_ticks_control(delay_ticks_control_available_values,(PPQN*8)+(PPQN/2), "8.5");
                add_option_delay_ticks_control(delay_ticks_control_available_values,PPQN*9,            "9");
                add_option_delay_ticks_control(delay_ticks_control_available_values,(PPQN*9)+(PPQN/2), "9.5");
                add_option_delay_ticks_control(delay_ticks_control_available_values,PPQN*10,            "10");
                add_option_delay_ticks_control(delay_ticks_control_available_values,(PPQN*10)+(PPQN/2), "10.5");
                add_option_delay_ticks_control(delay_ticks_control_available_values,PPQN*11,            "11");
                add_option_delay_ticks_control(delay_ticks_control_available_values,(PPQN*11)+(PPQN/2), "11.5");
                add_option_delay_ticks_control(delay_ticks_control_available_values,PPQN*12,            "12");
                add_option_delay_ticks_control(delay_ticks_control_available_values,(PPQN*12)+(PPQN/2), "12.5");
                add_option_delay_ticks_control(delay_ticks_control_available_values,PPQN*13,            "13");
                add_option_delay_ticks_control(delay_ticks_control_available_values,(PPQN*13)+(PPQN/2), "13.5");
                add_option_delay_ticks_control(delay_ticks_control_available_values,PPQN*14,            "14");
                add_option_delay_ticks_control(delay_ticks_control_available_values,(PPQN*14)+(PPQN/2), "14.5");
                add_option_delay_ticks_control(delay_ticks_control_available_values,PPQN*15,            "15");
                add_option_delay_ticks_control(delay_ticks_control_available_values,(PPQN*15)+(PPQN/2), "15.5");
            }

            //Serial.println(F("\tDividedClockedBehaviour creating delay_ticks_control")); Serial_flush();
            LambdaSelectorControl<int32_t> *delay_ticks_control = new LambdaSelectorControl<int32_t>(
                "Delay",
                /*this,
                &DividedClockedBehaviour::set_delay_ticks,
                &DividedClockedBehaviour::get_delay_ticks,*/
                [=](int32_t v) -> void { this->set_delay_ticks(v); },
                [=]() -> int32_t { return this->get_delay_ticks(); },
                nullptr
            );       
            delay_ticks_control->set_available_values(delay_ticks_control_available_values);

            delay_ticks_control->go_back_on_select = true;
        #endif

        #ifdef ENABLE_AUTO_RESTART_CONTROL
            //Serial.println(F("\tDividedClockedBehaviour creating auto_restart_control")); Serial_flush();
            LambdaToggleControl *auto_restart_control = new LambdaToggleControl(
                "Restart",
                [=](bool v) -> void { this->set_auto_restart_on_change(v); },
                [=]() -> bool { return this->should_auto_restart_on_change(); },
                nullptr
            );
            //auto_restart_control->debug = true;
        #endif

        #ifdef ENABLE_PAUSE_DURING_DELAY_CONTROL
            //Serial.println(F("\tDividedClockedBehaviour creating pause_during_delay_control")); Serial_flush();
            LambdaSelectorControl<int8_t> *pause_during_delay_control = new LambdaSelectorControl<int8_t>(
                "Pause",
                /*this,
                &DividedClockedBehaviour::set_pause_during_delay,
                &DividedClockedBehaviour::get_pause_during_delay,*/
                [=](int8_t v) -> void { this->set_pause_during_delay(v); },
                [=]() -> int8_t { return this->get_pause_during_delay(); },
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



void DividedClockedBehaviour::send_clock(unsigned long ticks) {
    //if (this->debug) Serial.print("/");
    // if we've been waiting to be on-beat before changing divisor, do so now
    if (is_bpm_on_beat(ticks) && this->queued_clock_divisor!=this->clock_divisor) {
        this->set_divisor(this->queued_clock_divisor);
        this->clock_divisor = queued_clock_divisor;
    }
    //if (this->clock_delay_ticks==PPQN*4) this->debug = true; else this->debug = false;

    int32_t period_length = this->get_period_length();
    uint32_t tick_of_period = (ticks%period_length); //*BARS_PER_PHRASE));

    real_ticks = (int32_t)tick_of_period - clock_delay_ticks;
    if (this->waiting && ((int32_t)tick_of_period) < clock_delay_ticks) { //(real_ticks) < clock_delay_ticks) {
        //if (this->debug) Serial.printf(F("DividedClockBehaviour with global ticks %i, not sending because waiting %i && (tick_of_period %i < clock_delay_ticks of %i\n"), ticks, waiting, tick_of_period, clock_delay_ticks);
        //Serial.printf("%s#sendClock() not sending due to tick %% clock_divisor test failed", this->get_label());
        return;
    }
    if (waiting) {
        //if (this->debug) Serial.printf(F("%s: DividedClockBehaviour with real_ticks %i and clock_delay_ticks %i was waiting\n"), this->get_label(), real_ticks, clock_delay_ticks);
        this->started = true;
        this->sendRealTime((uint8_t)(midi::Stop)); //sendStart();
        this->sendRealTime((uint8_t)(midi::Start)); //sendStart();
        //if (this->debug) Serial.printf(F("%s:\tunsetting waiting!\n"), this->get_label());
        waiting = false;
    }

    if (is_bpm_on_bar(real_ticks)) { //}, clock_delay_ticks)) {
        //if (this->should_auto_restart_on_change())    // force resync restart on every bar, however, no good if target device (eg beatstep) pattern is longer than a bar
        //    this->set_restart_on_bar();
        //if (this->debug) Serial.printf(F("%s: DividedClockBehaviour with real_ticks %i and clock_delay_ticks %i confirmed yes for is_bpm_on_bar, called ClockedBehaviour::on_bar\n"), this->get_label(), real_ticks, clock_delay_ticks);
        ClockedBehaviour::on_bar(BPM_CURRENT_BAR_OF_PHRASE);
        // todo: should we handle calling on_phrase here?  maybe thats where we should force re-sync?
    }

    if (ticks % clock_divisor == 0) {
        //if (this->debug) Serial.print("\\");
        ClockedBehaviour::send_clock(ticks - clock_delay_ticks);
    } else {
        //if (this->debug) Serial.printf("%s#sendClock() not sending due to tick %% clock_divisor test failed", this->get_label());
    }
}