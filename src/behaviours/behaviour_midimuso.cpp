#include "Config.h"

#ifdef ENABLE_MIDIMUSO

#include "behaviours/behaviour_midimuso.h"

#ifdef ENABLE_SCREEN
    #include "mymenu.h"
    #include "submenuitem_bar.h"

    #include "menuitems_lambda.h"

    //FLASHMEM //DeviceBehaviour_Beatstep::make_menu_items() causes a section type conflict with virtual void DeviceBehaviour_Beatstep::setup_callbacks()
    LinkedList<MenuItem*> *DeviceBehaviour_MIDIMuso::make_menu_items() {
        DeviceBehaviourUltimateBase::make_menu_items();

        this->menuitems->add(new LambdaActionConfirmItem("Set mode 0B", this, [=]() -> void { this->set_mode_0b; } )); //&DeviceBehaviour_MIDIMuso::set_mode_0b));
        this->menuitems->add(new LambdaActionConfirmItem("Set mode 2B", this, [=]() -> void { this->set_mode_2b; } )); //&DeviceBehaviour_MIDIMuso::set_mode_2b));

        return this->menuitems;
    }
#endif

DeviceBehaviour_MIDIMuso *behaviour_midimuso = new DeviceBehaviour_MIDIMuso();

#endif