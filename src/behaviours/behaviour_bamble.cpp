#include "behaviours/behaviour_bamble.h"

DeviceBehaviour_Bamble *behaviour_bamble = new DeviceBehaviour_Bamble(); //nullptr;

void bamble_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    if (behaviour_bamble!=nullptr) behaviour_bamble->receive_control_change(inChannel, inNumber, inValue);
}

void bamble_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_bamble!=nullptr) behaviour_bamble->receive_note_on(inChannel, inNumber, inVelocity);
}

void bamble_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_bamble!=nullptr) behaviour_bamble->receive_note_off(inChannel, inNumber, inVelocity);
}

#ifdef ENABLE_SCREEN

#include "submenuitem_bar.h"
#include "midi/Drums.h"

//FLASHMEM //virtual LinkedList<MenuItem*>* DeviceBehaviour_Bamble::make_menu_items() causes a section type conflict with virtual void DeviceBehaviour_Bamble::setup_callbacks()
LinkedList<MenuItem*> *DeviceBehaviour_Bamble::make_menu_items() {
    //Serial.println(F("\tDividedClockedBehaviour calling DeviceBehaviourUltimateBase::make_menu_items()")); Serial.flush();
    DeviceBehaviourUltimateBase::make_menu_items();

    //String bar_label = String(this->get_label()) + String(F(" Clock"));
    SubMenuItemBar *bar = new SubMenuItemBar("Euclidian settings");

    ObjectSelectorControl<DeviceBehaviour_Bamble,int8_t> *euclidian_mode_control = new ObjectSelectorControl<DeviceBehaviour_Bamble,int8_t>(
        "Mode",
        this,
        &DeviceBehaviour_Bamble::setDemoMode,
        &DeviceBehaviour_Bamble::getDemoMode
    );
    euclidian_mode_control->go_back_on_select = true;
    
    euclidian_mode_control->add_available_value(0, "None");
    euclidian_mode_control->add_available_value(1, "Euclidian");
    euclidian_mode_control->add_available_value(2, "Mutate");
    euclidian_mode_control->add_available_value(3, "Long");
    euclidian_mode_control->add_available_value(4, "??");
    euclidian_mode_control->add_available_value(5, "Random");

    ObjectToggleControl<DeviceBehaviour_Bamble> *fills_control = new ObjectToggleControl<DeviceBehaviour_Bamble>(
        "Fills",
        this,
        &DeviceBehaviour_Bamble::setFillsMode,
        &DeviceBehaviour_Bamble::getFillsMode
    );
    
    ObjectNumberControl<DeviceBehaviour_Bamble,float> *euclidian_density_control = new ObjectNumberControl<DeviceBehaviour_Bamble,float>(
        "Density",
        this,
        &DeviceBehaviour_Bamble::setDensity,
        &DeviceBehaviour_Bamble::getDensity,
        nullptr,
        (float)0.0,
        (float)1.0
    );
    euclidian_density_control->go_back_on_select = true;

    // mutate pattern range 
    SubMenuItemBar *mutate_range = new SubMenuItemBar("Mutate pattern range");

    ObjectSelectorControl<DeviceBehaviour_Bamble,int8_t> *minimum_pattern = new ObjectSelectorControl<DeviceBehaviour_Bamble,int8_t>(
        "Low Mutate",
        this,
        &DeviceBehaviour_Bamble::setMinimumPattern,
        &DeviceBehaviour_Bamble::getMinimumPattern
    );
    minimum_pattern->add_available_value(TRIGGER_KICK,      "Kick");
    minimum_pattern->add_available_value(TRIGGER_SIDESTICK, "Sidestick");
    minimum_pattern->add_available_value(TRIGGER_CLAP,      "Clap");
    minimum_pattern->add_available_value(TRIGGER_SNARE,     "Snare");
    minimum_pattern->add_available_value(TRIGGER_CRASH_1,   "Crash 1");
    minimum_pattern->add_available_value(TRIGGER_TAMB,      "Tamb");
    minimum_pattern->add_available_value(TRIGGER_HITOM,     "HiTom");
    minimum_pattern->add_available_value(TRIGGER_LOTOM,     "LoTom");
    minimum_pattern->add_available_value(TRIGGER_PEDALHAT,  "Pedal");
    minimum_pattern->add_available_value(TRIGGER_OPENHAT,   "OHH");
    minimum_pattern->add_available_value(TRIGGER_CLOSEDHAT, "CHH");
    minimum_pattern->add_available_value(TRIGGER_CRASH_2,   "Crash 2");
    minimum_pattern->add_available_value(TRIGGER_SPLASH,    "Splash");
    minimum_pattern->add_available_value(TRIGGER_VIBRA,     "Vibra");
    minimum_pattern->add_available_value(TRIGGER_RIDE_BELL, "Ride Bell");
    minimum_pattern->add_available_value(TRIGGER_RIDE_CYM,  "Ride Cym");
    minimum_pattern->go_back_on_select = true;
    mutate_range->add(minimum_pattern);

    ObjectSelectorControl<DeviceBehaviour_Bamble,int8_t> *maximum_pattern = new ObjectSelectorControl<DeviceBehaviour_Bamble,int8_t>(
        "High Mutate",
        this,
        &DeviceBehaviour_Bamble::setMaximumPattern,
        &DeviceBehaviour_Bamble::getMaximumPattern
    );
    maximum_pattern->add_available_value(TRIGGER_KICK,      "Kick");
    maximum_pattern->add_available_value(TRIGGER_SIDESTICK, "Sidestick");
    maximum_pattern->add_available_value(TRIGGER_CLAP,      "Clap");
    maximum_pattern->add_available_value(TRIGGER_SNARE,     "Snare");
    maximum_pattern->add_available_value(TRIGGER_CRASH_1,   "Crash 1");
    maximum_pattern->add_available_value(TRIGGER_TAMB,      "Tamb");
    maximum_pattern->add_available_value(TRIGGER_HITOM,     "HiTom");
    maximum_pattern->add_available_value(TRIGGER_LOTOM,     "LoTom");
    maximum_pattern->add_available_value(TRIGGER_PEDALHAT,  "Pedal");
    maximum_pattern->add_available_value(TRIGGER_OPENHAT,   "OHH");
    maximum_pattern->add_available_value(TRIGGER_CLOSEDHAT, "CHH");
    maximum_pattern->add_available_value(TRIGGER_CRASH_2,   "Crash 2");
    maximum_pattern->add_available_value(TRIGGER_SPLASH,    "Splash");
    maximum_pattern->add_available_value(TRIGGER_VIBRA,     "Vibra");
    maximum_pattern->add_available_value(TRIGGER_RIDE_BELL, "Ride Bell");
    maximum_pattern->add_available_value(TRIGGER_RIDE_CYM,  "Ride Cym");
    maximum_pattern->go_back_on_select = true;
    mutate_range->add(maximum_pattern);

    bar->add(euclidian_mode_control);
    bar->add(fills_control);
    bar->add(euclidian_density_control);

    menuitems->add(bar);
    menuitems->add(mutate_range);

    DividedClockedBehaviour::make_menu_items();

    return menuitems;

}

#endif