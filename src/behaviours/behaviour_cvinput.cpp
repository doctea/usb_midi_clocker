#include "Config.h"

#include "behaviours/behaviour_cvinput.h"

#ifdef ENABLE_SCREEN
    #include "bpm.h"
    #include "menu.h"
    #include "menuitems.h"
    #include "menuitems_object.h"
    #include "submenuitem_bar.h"
    #include "ParameterManager.h"
    #include "mymenu_items/ParameterInputMenuItems.h"

    extern ParameterManager *parameter_manager;

    LinkedList<MenuItem *> *DeviceBehaviour_CVInput::make_menu_items() {
        LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();
        //SubMenuItemBar *bar = new SubMenuItemBar((String(this->get_label()) + String(" CV Pitch")).c_str());
        /*
                const char *label, 
                TargetClass *target_object, 
                void(TargetClass::*setter_func)(BaseParameterInput*), 
                BaseParameterInput *initial_parameter_input,
                LinkedList<BaseParameterInput*> *available_parameter_inputs,
        */
        ParameterInputSelectorControl<DeviceBehaviour_CVInput> *parameter_input_selector 
            = new ParameterInputSelectorControl<DeviceBehaviour_CVInput> (
                "Select Parameter Input",
                this,
                &DeviceBehaviour_CVInput::set_selected_parameter_input,
                parameter_manager->available_inputs
        );
        menuitems->add(parameter_input_selector);

        HarmonyStatus *harmony = new HarmonyStatus("CV->MIDI pitch", 
            &this->last_note, 
            &this->current_note
        );
        menuitems->add(harmony);

        /*ObjectNumberControl<DeviceBehaviour_CVInput,int32_t> *length_ticks_control 
        = new ObjectNumberControl<DeviceBehaviour_CVInput,int32_t> (
                "Note length",
                this,
                &DeviceBehaviour_CVInput::set_note_length,
                &DeviceBehaviour_CVInput::get_note_length
            );
        menuitems->add(length_ticks_control);*/

        Serial.println("about to create length_ticks_control.."); Serial.flush();
        ObjectSelectorControl<DeviceBehaviour_CVInput,int32_t> *length_ticks_control 
            = new ObjectSelectorControl<DeviceBehaviour_CVInput,int32_t>(
                "Note length",
                this,
                &DeviceBehaviour_CVInput::set_note_length,
                &DeviceBehaviour_CVInput::get_note_length
        );
        Serial.println(F("about to add values..")); Serial.flush();
        length_ticks_control->add_available_value(0,                 "None");
        length_ticks_control->add_available_value(PPQN/PPQN,         "-");
        length_ticks_control->add_available_value(PPQN/4,            "1/32");
        length_ticks_control->add_available_value(PPQN/3,            "1/12");
        length_ticks_control->add_available_value(PPQN/2,            "1/8");
        length_ticks_control->add_available_value(PPQN,              "1/4");
        length_ticks_control->add_available_value(PPQN*2,            "1/2");
        length_ticks_control->add_available_value(PPQN*4,            "1");
        Serial.println(F("about to add to menuitems list..")); Serial.flush();
        menuitems->add(length_ticks_control);

        return menuitems;
    }
#endif

DeviceBehaviour_CVInput *behaviour_cvinput = new DeviceBehaviour_CVInput(); //(midi::MidiInterface<midi::SerialMIDI<HardwareSerial>>*)nullptr, &ENABLE_BITBOX);
