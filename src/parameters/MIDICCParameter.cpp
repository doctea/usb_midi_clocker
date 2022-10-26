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

    //while(!Serial);
    Serial.printf("in makeControls() in %s, parameter_manager is @%p and available_inputs is @%p\n", this->label, parameter_manager, parameter_manager->available_inputs);

    SubMenuItemBar *input_selectors_bar = new SubMenuItemBar("Inputs");
    //input_selectors_bar->debug = true;  // TODO: remove this !
    input_selectors_bar->add(new ParameterInputSelectorControl<MIDICCParameter>(
        "Input 1", 
        this,
        &MIDICCParameter::set_slot_0_input,
        parameter_manager->available_inputs,
        parameter_manager->getInputForName(this->get_input_name_for_slot(0)),
        fullmenuitem->items->get(1)     // second item of ParameterMenuItem is first slot
    ));
    input_selectors_bar->add(new ParameterInputSelectorControl<MIDICCParameter>(
        "Input 2", 
        this,
        &MIDICCParameter::set_slot_1_input,
        parameter_manager->available_inputs,
        parameter_manager->getInputForName(this->get_input_name_for_slot(1)),
        fullmenuitem->items->get(2)     // third item of ParameterMenuItem is second slot
    ));
    input_selectors_bar->add(new ParameterInputSelectorControl<MIDICCParameter>(
        "Input 3", 
        this,
        &MIDICCParameter::set_slot_2_input,
        parameter_manager->available_inputs,
        parameter_manager->getInputForName(this->get_input_name_for_slot(2)),
        fullmenuitem->items->get(3)     // fourth item of ParameterMenuItem is third slot
    ));
    controls->add(input_selectors_bar);

    return controls;
}