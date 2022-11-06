#include "behaviours/behaviour_bamble.h"

DeviceBehaviour_Bamble *behaviour_bamble = new DeviceBehaviour_Bamble(); //nullptr;

#ifdef ENABLE_BAMBLE_INPUT
    void bamble_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
        if (behaviour_bamble!=nullptr) behaviour_bamble->receive_control_change(inChannel, inNumber, inValue);
    }

    void bamble_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
        if (behaviour_bamble!=nullptr) behaviour_bamble->receive_note_on(inChannel, inNumber, inVelocity);
    }

    void bamble_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
        if (behaviour_bamble!=nullptr) behaviour_bamble->receive_note_off(inChannel, inNumber, inVelocity);
    }
#endif

#ifdef ENABLE_SCREEN

#include "submenuitem_bar.h"
#include "midi/Drums.h"

const char *LABEL_KICK      = "Kick";
const char *LABEL_SIDESTICK = "Sidestick";
const char *LABEL_CLAP      = "Clap";
const char *LABEL_SNARE     = "Snare";
const char *LABEL_CRASH_1   = "Crash 1";
const char *LABEL_TAMB      = "Tamb";
const char *LABEL_HITOM     = "HiTom";
const char *LABEL_LOTOM     = "LoTom";
const char *LABEL_PEDALHAT  = "Pedal";
const char *LABEL_OPENHAT   = "OHH";
const char *LABEL_CLOSEDHAT = "CHH";
const char *LABEL_CRASH_2   = "Crash 2";
const char *LABEL_SPLASH    = "Splash";
const char *LABEL_VIBRA     = "Vibra";
const char *LABEL_RIDE_BELL = "Ride Bell";
const char *LABEL_RIDE_CYM  = "Ride Cym";

//FLASHMEM //virtual LinkedList<MenuItem*>* DeviceBehaviour_Bamble::make_menu_items() causes a section type conflict with virtual void DeviceBehaviour_Bamble::setup_callbacks()
//FLASHMEM
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
    minimum_pattern->add_available_value(TRIGGER_KICK,      LABEL_KICK);
    minimum_pattern->add_available_value(TRIGGER_SIDESTICK, LABEL_SIDESTICK);
    minimum_pattern->add_available_value(TRIGGER_CLAP,      LABEL_CLAP);
    minimum_pattern->add_available_value(TRIGGER_SNARE,     LABEL_SNARE);
    minimum_pattern->add_available_value(TRIGGER_CRASH_1,   LABEL_CRASH_1);
    minimum_pattern->add_available_value(TRIGGER_TAMB,      LABEL_TAMB);
    minimum_pattern->add_available_value(TRIGGER_HITOM,     LABEL_HITOM);
    minimum_pattern->add_available_value(TRIGGER_LOTOM,     LABEL_LOTOM);
    minimum_pattern->add_available_value(TRIGGER_PEDALHAT,  LABEL_PEDALHAT);
    minimum_pattern->add_available_value(TRIGGER_OPENHAT,   LABEL_OPENHAT);
    minimum_pattern->add_available_value(TRIGGER_CLOSEDHAT, LABEL_CLOSEDHAT);
    minimum_pattern->add_available_value(TRIGGER_CRASH_2,   LABEL_CRASH_2);
    minimum_pattern->add_available_value(TRIGGER_SPLASH,    LABEL_SPLASH);
    minimum_pattern->add_available_value(TRIGGER_VIBRA,     LABEL_VIBRA);
    minimum_pattern->add_available_value(TRIGGER_RIDE_BELL, LABEL_RIDE_BELL);
    minimum_pattern->add_available_value(TRIGGER_RIDE_CYM,  LABEL_RIDE_CYM);
    minimum_pattern->go_back_on_select = true;
    mutate_range->add(minimum_pattern);

    ObjectSelectorControl<DeviceBehaviour_Bamble,int8_t> *maximum_pattern = new ObjectSelectorControl<DeviceBehaviour_Bamble,int8_t>(
        "High Mutate",
        this,
        &DeviceBehaviour_Bamble::setMaximumPattern,
        &DeviceBehaviour_Bamble::getMaximumPattern
    );
    maximum_pattern->add_available_value(TRIGGER_KICK,      LABEL_KICK);
    maximum_pattern->add_available_value(TRIGGER_SIDESTICK, LABEL_SIDESTICK);
    maximum_pattern->add_available_value(TRIGGER_CLAP,      LABEL_CLAP);
    maximum_pattern->add_available_value(TRIGGER_SNARE,     LABEL_SNARE);
    maximum_pattern->add_available_value(TRIGGER_CRASH_1,   LABEL_CRASH_1);
    maximum_pattern->add_available_value(TRIGGER_TAMB,      LABEL_TAMB);
    maximum_pattern->add_available_value(TRIGGER_HITOM,     LABEL_HITOM);
    maximum_pattern->add_available_value(TRIGGER_LOTOM,     LABEL_LOTOM);
    maximum_pattern->add_available_value(TRIGGER_PEDALHAT,  LABEL_PEDALHAT);
    maximum_pattern->add_available_value(TRIGGER_OPENHAT,   LABEL_OPENHAT);
    maximum_pattern->add_available_value(TRIGGER_CLOSEDHAT, LABEL_CLOSEDHAT);
    maximum_pattern->add_available_value(TRIGGER_CRASH_2,   LABEL_CRASH_2);
    maximum_pattern->add_available_value(TRIGGER_SPLASH,    LABEL_SPLASH);
    maximum_pattern->add_available_value(TRIGGER_VIBRA,     LABEL_VIBRA);
    maximum_pattern->add_available_value(TRIGGER_RIDE_BELL, LABEL_RIDE_BELL);
    maximum_pattern->add_available_value(TRIGGER_RIDE_CYM,  LABEL_RIDE_CYM);
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