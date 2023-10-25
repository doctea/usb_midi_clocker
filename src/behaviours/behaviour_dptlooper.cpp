#include "Config.h"

#ifdef ENABLE_DPT_LOOPER

#include "behaviours/behaviour_dptlooper.h"

DeviceBehaviour_DPTLooper *behaviour_dptlooper = new DeviceBehaviour_DPTLooper();

#ifdef ENABLE_SCREEN
    #include "mymenu.h"
    #include "submenuitem_bar.h"
    //FLASHMEM //DeviceBehaviour_Beatstep::make_menu_items() causes a section type conflict with virtual void DeviceBehaviour_Beatstep::setup_callbacks()
    /*LinkedList<MenuItem*> *DeviceBehaviour_DPTLooper::make_menu_items() {
        DeviceBehaviourUltimateBase::make_menu_items();
        SubMenuItemBar *bar = new SubMenuItemBar("DPT Looper options");

        ObjectSelectorControl<DeviceBehaviour_DPTLooper,int> *loop_type = new ObjectSelectorControl<DeviceBehaviour_DPTLooper,int>(
            "Loop type", this, &DeviceBehaviour_DPTLooper::setLoopType, &DeviceBehaviour_DPTLooper::getLoopType
        );
        loop_type->add_available_value(loop_mode_t::ON_BAR,     "Bar");
        loop_type->add_available_value(loop_mode_t::ON_2BAR,    "2xBar");
        loop_type->add_available_value(loop_mode_t::ON_PHRASE,  "Phrase");
        loop_type->add_available_value(loop_mode_t::ON_2PHRASE, "2xPhrase");

        bar->add(loop_type);

        bar->add(new ObjectActionItem<DeviceBehaviour_DPTLooper>("Start", this, &DeviceBehaviour_DPTLooper::start_dubbing));
        bar->add(new ObjectActionItem<DeviceBehaviour_DPTLooper>("Stop",  this, &DeviceBehaviour_DPTLooper::stop_dubbing));

        this->menuitems->add(bar);
        return this->menuitems;
    }*/
#endif

#endif