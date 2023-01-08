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
 
    void DeviceBehaviour_CVInput::set_selected_parameter_input(BaseParameterInput *input) {
        if (input==nullptr)
            Serial.printf(F("nullptr passed to set_selected_parameter_input(BaseParameterInput *input)\n"));

        Serial.printf(F("set_selected_parameter_input(%s)\n"), input->name);

        if (this->parameter_input_selector!=nullptr && input!=nullptr) {
            Serial.println("set_selected_parameter_input() updating the control..");
            this->parameter_input_selector->update_source(input);
        }
        this->source_input = input;

        //else
        //Serial.printf("WARNING in %s: set_selected_parameter_input() not passed a VoltageParameterInput in '%c'!\n", this->get_label(), input->name);               
    }

    FLASHMEM
    LinkedList<MenuItem *> *DeviceBehaviour_CVInput::make_menu_items() {
        Serial.println(F("DeviceBehaviour_CVInput::make_menu_items() start")); Serial_flush();
        LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();
        //SubMenuItemBar *bar = new SubMenuItemBar((String(this->get_label()) + String(" CV Pitch")).c_str());
        /*
                const char *label, 
                TargetClass *target_object, 
                void(TargetClass::*setter_func)(BaseParameterInput*), 
                BaseParameterInput *initial_parameter_input,
                LinkedList<BaseParameterInput*> *available_parameter_inputs,
        */
        Serial.println(F("DeviceBehaviour_CVInput::make_menu_items() setting up ParameterInputSelectorControl")); Serial_flush();
        //ParameterInputSelectorControl<DeviceBehaviour_CVInput> *parameter_input_selector 
        this->parameter_input_selector 
            = new ParameterInputSelectorControl<DeviceBehaviour_CVInput> (
                "Select Parameter Input",
                this,
                &DeviceBehaviour_CVInput::set_selected_parameter_input,
                parameter_manager->get_available_pitch_inputs(),
                this->source_input
        );
        menuitems->add(parameter_input_selector);

        Serial.println(F("DeviceBehaviour_CVInput::make_menu_items() setting up HarmonyStatus")); Serial_flush();
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

        Serial.println(F("about to create length_ticks_control ObjectSelectorControl..")); Serial_flush();
        ObjectSelectorControl<DeviceBehaviour_CVInput,int32_t> *length_ticks_control 
            = new ObjectSelectorControl<DeviceBehaviour_CVInput,int32_t>(
                "Note length",
                this,
                &DeviceBehaviour_CVInput::set_note_length,
                &DeviceBehaviour_CVInput::get_note_length
        );
        Serial.println(F("about to add values..")); Serial_flush();
        length_ticks_control->add_available_value(0,                 "None");
        length_ticks_control->add_available_value(PPQN/PPQN,         "-");
        length_ticks_control->add_available_value(PPQN/4,            "1/32");
        length_ticks_control->add_available_value(PPQN/3,            "1/12");
        length_ticks_control->add_available_value(PPQN/2,            "1/8");
        length_ticks_control->add_available_value(PPQN,              "1/4");
        length_ticks_control->add_available_value(PPQN*2,            "1/2");
        length_ticks_control->add_available_value(PPQN*4,            "1");
        Serial.println(F("about to add to menuitems list..")); Serial_flush();
        menuitems->add(length_ticks_control);

        Serial.println(F("returning..")); Serial_flush();
        return menuitems;
    }
#endif

DeviceBehaviour_CVInput *behaviour_cvinput = new DeviceBehaviour_CVInput(); //(midi::MidiInterface<midi::SerialMIDI<HardwareSerial>>*)nullptr, &ENABLE_BITBOX);
