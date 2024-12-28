#ifdef ENABLE_CV_OUTPUT

#include "behaviours/behaviour_cvoutput.h"
#include "mymenu_items/ParameterMenuItems_lowmemory.h"

DeviceBehaviour_CVOutput<DAC8574> *behaviour_cvoutput_1 = new DeviceBehaviour_CVOutput<DAC8574>("CV Pitch Output 1", ENABLE_CV_OUTPUT);

template<class DACClass>
LinkedList<MenuItem*> *DeviceBehaviour_CVOutput<DACClass>::make_menu_items() {
    LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();

    //menu->add_page("CVO-A");
    /*menuitems->add(new SeparatorMenuItem("CVO-A"));
    create_low_memory_parameter_controls(output_a->label, output_a);
    //menu->add(parameter_manager->makeMenuItemsForParameter(output_a));
    menuitems->add(new SeparatorMenuItem("CVO-B"));
    create_low_memory_parameter_controls(output_b->label, output_b);
    //menu->add(parameter_manager->makeMenuItemsForParameter(output_b));
    menuitems->add(new SeparatorMenuItem("CVO-C"));
    create_low_memory_parameter_controls(output_c->label, output_c);
    //menu->add(parameter_manager->makeMenuItemsForParameter(output_c));
    menuitems->add(new SeparatorMenuItem("CVO-D"));
    create_low_memory_parameter_controls(output_d->label, output_d);*/

    menuitems->add(
        parameter_manager->getModulatableParameterSubMenuItems(menu, "DAC options", this->parameters)
    );

    return menuitems;
}

#endif