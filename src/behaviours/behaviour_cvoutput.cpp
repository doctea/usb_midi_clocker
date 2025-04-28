#ifdef ENABLE_CV_OUTPUT

#include "Config.h"

#include "behaviours/behaviour_cvoutput.h"
#include "mymenu_items/ParameterMenuItems_lowmemory.h"

#ifdef ENABLE_CV_OUTPUT
    DeviceBehaviour_CVOutput<DAC8574> *behaviour_cvoutput_1 = nullptr; //new DeviceBehaviour_CVOutput<DAC8574>("CV Pitch Output 1", ENABLE_CV_OUTPUT);
#endif
#ifdef ENABLE_CV_OUTPUT_2
    DeviceBehaviour_CVOutput<DAC8574> *behaviour_cvoutput_2 = nullptr; //
#endif
#ifdef ENABLE_CV_OUTPUT_3
    DeviceBehaviour_CVOutput<DAC8574> *behaviour_cvoutput_3 = nullptr; //
#endif


/*template<class DACClass>
LinkedList<MenuItem*> *DeviceBehaviour_CVOutput<DACClass>::make_menu_items() {
    LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();

    PolyphonicBehaviour::make_menu_items();

    return menuitems;
}*/


#endif