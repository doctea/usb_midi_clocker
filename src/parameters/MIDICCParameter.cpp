#include "parameters/MIDICCParameter.h"

#include "menu.h"

#include "mymenu_items/ParameterMenuItems.h"
#include "mymenu_items/ParameterInputMenuItems.h"

LinkedList<MenuItem *> *MIDICCParameter::makeControls() {
    LinkedList<MenuItem *> *controls = new LinkedList<MenuItem *>();
    
    Serial.printf("MIDICCParameter#makeControls for %s\n", this->label);
    // first set up the submenu to hold the values
    ParameterMenuItem *fullmenuitem = new ParameterMenuItem(this->label, this);
    controls->add(fullmenuitem);

    while(!Serial);
    Serial.printf("in makeControls() in %s, parameter_manager is @%p and available_inputs is @%p\n", this->label, parameter_manager, parameter_manager->available_inputs);

    SubMenuItemBar *input_selectors_bar = new SubMenuItemBar("Inputs");
    input_selectors_bar->debug = true;  // TODO: remove this !
    input_selectors_bar->add(new ParameterInputSelectorControl<MIDICCParameter>(
        "Input 1", 
        this,
        &MIDICCParameter::set_slot_0_input,
        parameter_manager->available_inputs
    ));
    input_selectors_bar->add(new ParameterInputSelectorControl<MIDICCParameter>(
        "Input 2", 
        this,
        &MIDICCParameter::set_slot_1_input,
        parameter_manager->available_inputs
    ));
    input_selectors_bar->add(new ParameterInputSelectorControl<MIDICCParameter>(
        "Input 3", 
        this,
        &MIDICCParameter::set_slot_2_input,
        parameter_manager->available_inputs
    ));
    controls->add(input_selectors_bar);

    return controls;
}