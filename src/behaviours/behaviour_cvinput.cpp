#include "Config.h"
#ifdef ENABLE_CV_INPUT_PITCH

#include "behaviours/behaviour_cvinput.h"

#include "bpm.h"

#include "scales.h"

extern bool debug_flag;

#include "ParameterManager.h"

    extern ParameterManager *parameter_manager;
 
    void DeviceBehaviour_CVInput::set_selected_parameter_input(BaseParameterInput *input) {
        /*if (input==nullptr)
            Serial.printf(F("nullptr passed to set_selected_parameter_input(BaseParameterInput *input)\n"));
        else
            Serial.printf(F("set_selected_parameter_input(%s)\n"), input->name);*/

        /*if (this->pitch_parameter_selector!=nullptr) {
            //Serial.println(F("set_selected_parameter_input() updating the control.."));
            this->pitch_parameter_selector->update_source(input);
        }*/
        this->pitch_input = input;

        #ifdef ENABLE_SCALES
            if (is_valid_note(this->chord_player.current_note)) {
                chord_player.trigger_off_for_pitch_because_changed(this->chord_player.current_note);
            }
        #endif
        //Serial.println(F("finished in set_selected_paramter_input"));
        //else
        //Serial.printf(F("WARNING in %s: set_selected_parameter_input() not passed a VoltageParameterInput in '%c'!\n"), this->get_label(), input->name);
    }
    void DeviceBehaviour_CVInput::set_selected_velocity_input(BaseParameterInput *input) {
        /*if (input==nullptr)
            Serial.printf(F("nullptr passed to set_selected_parameter_input(BaseParameterInput *input)\n"));
        else
            Serial.printf(F("set_selected_parameter_input(%s)\n"), input->name);*/

        /*if (this->velocity_parameter_selector!=nullptr) {
            //Serial.println(F("set_selected_parameter_input() updating the control.."));
            this->velocity_parameter_selector->update_source(input);
        } */
        this->velocity_input = input;

        /*if (is_valid_note(this->current_note)) {
            trigger_off_for_pitch_because_changed(this->current_note);
        }*/
        //Serial.println(F("finished in set_selected_paramter_input"));
        //else
        //Serial.printf(F("WARNING in %s: set_selected_parameter_input() not passed a VoltageParameterInput in '%c'!\n"), this->get_label(), input->name);
    }

    
#ifdef ENABLE_SCREEN
    #include "menu.h"
    #include "menuitems.h"
    #include "menuitems_object.h"
    #include "menuitems_lambda_selector.h"
    #include "submenuitem_bar.h"
    #include "mymenu_items/ParameterInputMenuItems.h"
    #include "mymenu/menuitems_harmony.h"
    //FLASHMEM
    LinkedList<MenuItem *> *DeviceBehaviour_CVInput::make_menu_items() {
        //Serial.println(F("DeviceBehaviour_CVInput::make_menu_items() start")); Serial_flush();
        LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();
        //SubMenuItemBar *bar = new SubMenuItemBar((String(this->get_label()) + String(" CV Pitch")).c_str());
        /*
                const char *label, 
                TargetClass *target_object, 
                void(TargetClass::*setter_func)(BaseParameterInput*), 
                BaseParameterInput *initial_parameter_input,
                LinkedList<BaseParameterInput*> *available_parameter_inputs,
        */
        //Serial.println(F("DeviceBehaviour_CVInput::make_menu_items() setting up ParameterInputSelectorControl")); Serial_flush();
        //ParameterInputSelectorControl<DeviceBehaviour_CVInput> *pitch_parameter_selector 

        SubMenuItemBar *bar = new SubMenuItemBar("Inputs");

        this->pitch_parameter_selector 
            = new ParameterInputSelectorControl<DeviceBehaviour_CVInput> (
                "1v/oct Input",
                this,
                &DeviceBehaviour_CVInput::set_selected_parameter_input,
                &DeviceBehaviour_CVInput::get_selected_parameter_input,
                parameter_manager->get_available_pitch_inputs(),
                this->pitch_input
        );
        bar->add(pitch_parameter_selector);

        this->velocity_parameter_selector 
            = new ParameterInputSelectorControl<DeviceBehaviour_CVInput> (
                "Velocity Input",
                this,
                &DeviceBehaviour_CVInput::set_selected_velocity_input,
                &DeviceBehaviour_CVInput::get_selected_velocity_input,
                parameter_manager->available_inputs,
                this->velocity_input
        );
        bar->add(velocity_parameter_selector);

        menuitems->add(bar);

        #ifdef ENABLE_SCALES
            this->chord_player.make_menu_items(menuitems);
        #endif

        //Serial.println(F("returning..")); Serial_flush();
        return menuitems;
    }
#endif

    DeviceBehaviour_CVInput *behaviour_cvinput_1 = new DeviceBehaviour_CVInput("CV Pitch Input 1");
    DeviceBehaviour_CVInput *behaviour_cvinput_2 = new DeviceBehaviour_CVInput("CV Pitch Input 2");
    DeviceBehaviour_CVInput *behaviour_cvinput_3 = new DeviceBehaviour_CVInput("CV Pitch Input 3");
#endif
