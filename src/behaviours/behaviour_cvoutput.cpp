#ifdef ENABLE_CV_OUTPUT

#include "behaviours/behaviour_cvoutput.h"
#include "mymenu_items/ParameterMenuItems_lowmemory.h"

DeviceBehaviour_CVOutput<DAC8574> *behaviour_cvoutput_1 = nullptr; //new DeviceBehaviour_CVOutput<DAC8574>("CV Pitch Output 1", ENABLE_CV_OUTPUT);

/*
template<class DACClass>
LinkedList<MenuItem*> *DeviceBehaviour_CVOutput<DACClass>::make_menu_items() {
    LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();

    return menuitems;
}
*/

#endif