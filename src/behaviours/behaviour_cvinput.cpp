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

    extern ParameterManager parameter_manager;

    FLASHMEM LinkedList<MenuItem *> *DeviceBehaviour_CVInput::make_menu_items() {
        LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();
        #ifdef ENABLE_BASS_TRANSPOSE
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
                    &parameter_manager.available_inputs
            );
            HarmonyStatus *harmony = new HarmonyStatus("CV->MIDI pitch", 
                &this->last_note, 
                &this->current_note
            );

            menuitems->add(parameter_input_selector);
            menuitems->add(harmony);
        #endif
        return menuitems;
    }
#endif

DeviceBehaviour_CVInput *behaviour_cvinput = new DeviceBehaviour_CVInput(); //(midi::MidiInterface<midi::SerialMIDI<HardwareSerial>>*)nullptr, &ENABLE_BITBOX);
