#include "parameters/MIDICCParameter.h"

#include "menu.h"

#include "mymenu_items/ParameterMenuItems.h"
#include "mymenu_items/ParameterInputMenuItems.h"

/*
// this now moved to DoubleParameter, where i think it belongs
FLASHMEM LinkedList<MenuItem *> *MIDICCParameter::makeControls() {
    LinkedList<MenuItem *> *controls = new LinkedList<MenuItem *>();
    
    Serial.printf(F("MIDICCParameter#makeControls for %s\n"), this->label);
    // first set up the submenu to hold the values
    ParameterMenuItem *fullmenuitem = new ParameterMenuItem(this->label, this);
    controls->add(fullmenuitem);

    SubMenuItemBar *input_selectors_bar = new SubMenuItemBar("Inputs");
    input_selectors_bar->show_header = false;
    input_selectors_bar->show_sub_headers = false;
    //input_selectors_bar->debug = true;  // TODO: remove this !
    //this->connections[0].amount_control = fullmenuitem->items->get(1);
    //this->connections[1].amount_control = fullmenuitem->items->get(2);
    //this->connections[2].amount_control = fullmenuitem->items->get(3);

    MenuItem *spacer1 = new MenuItem("Inputs");
    MenuItem *spacer2 = new MenuItem("");
    spacer1->selectable = false;
    spacer2->selectable = false;
    input_selectors_bar->add(spacer1);

    ParameterInputSelectorControl<MIDICCParameter> *source_selector_1 = new ParameterInputSelectorControl<MIDICCParameter>(
        "Input 1", 
        this,
        &MIDICCParameter::set_slot_0_input,
        parameter_manager->available_inputs,
        parameter_manager->getInputForName(this->get_input_name_for_slot(0)),
        fullmenuitem->items->get(1)     // second item of ParameterMenuItem is first slot
    );
    ParameterInputSelectorControl<MIDICCParameter> *source_selector_2 = new ParameterInputSelectorControl<MIDICCParameter>(
        "Input 2", 
        this,
        &MIDICCParameter::set_slot_1_input,
        parameter_manager->available_inputs,
        parameter_manager->getInputForName(this->get_input_name_for_slot(1)),
        fullmenuitem->items->get(2)     // third item of ParameterMenuItem is second slot
    );
    ParameterInputSelectorControl<MIDICCParameter> *source_selector_3 = new ParameterInputSelectorControl<MIDICCParameter>(
        "Input 3", 
        this,
        &MIDICCParameter::set_slot_2_input,
        parameter_manager->available_inputs,
        parameter_manager->getInputForName(this->get_input_name_for_slot(2)),
        fullmenuitem->items->get(3)     // fourth item of ParameterMenuItem is third slot
    );
    //this->connections[0].input_control = source_selector_1;
    //this->connections[1].input_control = source_selector_2;
    //this->connections[2].input_control = source_selector_3;

    source_selector_1->go_back_on_select = true;
    source_selector_2->go_back_on_select = true;
    source_selector_3->go_back_on_select = true;

    this->link_parameter_input_controls_to_connections(
        fullmenuitem->items->get(1),
        fullmenuitem->items->get(2),
        fullmenuitem->items->get(3),
        source_selector_1,
        source_selector_2,
        source_selector_3
    );

    input_selectors_bar->add(source_selector_1);
    input_selectors_bar->add(source_selector_2);
    input_selectors_bar->add(source_selector_3);
    input_selectors_bar->add(spacer2);

    controls->add(input_selectors_bar);

    return controls;
}*/