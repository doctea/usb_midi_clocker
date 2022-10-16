#include "behaviours/behaviour_base.h"
//#include "menuitems.h"
#include "menu.h"
#include "menuitems.h"

LinkedList<MenuItem*> *DeviceBehaviourUltimateBase::make_menu_items() {
    if (this->menuitems == nullptr) {
        this->menuitems = new LinkedList<MenuItem*>();
        this->menuitems->add(new SeparatorMenuItem((char*)this->get_label(), C_WHITE));
        //this->menuitems->add(new MenuItem("test")); ///*(char*)this->get_label()*/, C_WHITE));
    }

    return this->menuitems;
}
