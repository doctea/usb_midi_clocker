#include "Config.h"
#ifdef ENABLE_CV_INPUT_PITCH

#include "behaviours/behaviour_cvinput.h"

#include "bpm.h"

#include "scales.h"
#include "mymenu/menuitems_scale.h"

extern bool debug_flag;

#ifdef ENABLE_SCREEN
    #include "menu.h"
    #include "menuitems.h"
    #include "menuitems_object.h"
    #include "submenuitem_bar.h"
    #include "ParameterManager.h"
    #include "mymenu_items/ParameterInputMenuItems.h"

    extern ParameterManager *parameter_manager;
 
    void DeviceBehaviour_CVInput::set_selected_parameter_input(BaseParameterInput *input) {
        /*if (input==nullptr)
            Serial.printf(F("nullptr passed to set_selected_parameter_input(BaseParameterInput *input)\n"));
        else
            Serial.printf(F("set_selected_parameter_input(%s)\n"), input->name);*/

        if (this->pitch_parameter_selector!=nullptr/* && input!=nullptr*/) {
            //Serial.println(F("set_selected_parameter_input() updating the control.."));
            this->pitch_parameter_selector->update_source(input);
        } 
        this->pitch_input = input;

        if (is_valid_note(this->current_note)) {
            trigger_off_for_pitch_because_changed(this->current_note);
        }
        //Serial.println(F("finished in set_selected_paramter_input"));
        //else
        //Serial.printf(F("WARNING in %s: set_selected_parameter_input() not passed a VoltageParameterInput in '%c'!\n"), this->get_label(), input->name);
    }
    void DeviceBehaviour_CVInput::set_selected_velocity_input(BaseParameterInput *input) {
        /*if (input==nullptr)
            Serial.printf(F("nullptr passed to set_selected_parameter_input(BaseParameterInput *input)\n"));
        else
            Serial.printf(F("set_selected_parameter_input(%s)\n"), input->name);*/

        if (this->velocity_parameter_selector!=nullptr/* && input!=nullptr*/) {
            //Serial.println(F("set_selected_parameter_input() updating the control.."));
            this->velocity_parameter_selector->update_source(input);
        } 
        this->velocity_input = input;

        /*if (is_valid_note(this->current_note)) {
            trigger_off_for_pitch_because_changed(this->current_note);
        }*/
        //Serial.println(F("finished in set_selected_paramter_input"));
        //else
        //Serial.printf(F("WARNING in %s: set_selected_parameter_input() not passed a VoltageParameterInput in '%c'!\n"), this->get_label(), input->name);
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
        //ParameterInputSelectorControl<DeviceBehaviour_CVInput> *pitch_parameter_selector 
        this->pitch_parameter_selector 
            = new ParameterInputSelectorControl<DeviceBehaviour_CVInput> (
                "1v/oct Input",
                this,
                &DeviceBehaviour_CVInput::set_selected_parameter_input,
                parameter_manager->get_available_pitch_inputs(),
                this->pitch_input
        );
        menuitems->add(pitch_parameter_selector);

        this->velocity_parameter_selector 
            = new ParameterInputSelectorControl<DeviceBehaviour_CVInput> (
                "Velocity Input",
                this,
                &DeviceBehaviour_CVInput::set_selected_velocity_input,
                parameter_manager->available_inputs,
                this->pitch_input
        );
        menuitems->add(velocity_parameter_selector);

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
        length_ticks_control->add_available_value(PPQN/8,            "32nd note");
        length_ticks_control->add_available_value(PPQN/4,            "16th note");
        length_ticks_control->add_available_value(PPQN/3,            "1/12");
        length_ticks_control->add_available_value(PPQN/2,            "8th note");
        length_ticks_control->add_available_value(PPQN,              "Beat");
        length_ticks_control->add_available_value(PPQN*2,            "1/2 bar");
        length_ticks_control->add_available_value(PPQN*4,            "Bar");
        Serial.println(F("about to add to menuitems list..")); Serial_flush();
        menuitems->add(length_ticks_control);

        Serial.println(F("about to create length_ticks_control ObjectSelectorControl..")); Serial_flush();
        ObjectSelectorControl<DeviceBehaviour_CVInput,int32_t> *trigger_ticks_control 
            = new ObjectSelectorControl<DeviceBehaviour_CVInput,int32_t>(
                "Trigger each",
                this,
                &DeviceBehaviour_CVInput::set_trigger_on_ticks,
                &DeviceBehaviour_CVInput::get_trigger_on_ticks
        );
        trigger_ticks_control->add_available_value(0,                 "Change");
        //trigger_ticks_control->add_available_value(PPQN/PPQN,         "-");
        trigger_ticks_control->add_available_value(PPQN/8,            "32nd note");
        trigger_ticks_control->add_available_value(PPQN/4,            "16th note");
        trigger_ticks_control->add_available_value(PPQN/3,            "1/12");
        trigger_ticks_control->add_available_value(PPQN/2,            "8th note");
        trigger_ticks_control->add_available_value(PPQN,              "Beat");
        trigger_ticks_control->add_available_value(PPQN*2,            "1/2 bar");
        trigger_ticks_control->add_available_value(PPQN*4,            "Bar");
        Serial.println(F("about to add to menuitems list..")); Serial_flush();
        menuitems->add(trigger_ticks_control);

        #ifdef CVINPUT_CONFIGURABLE_CHANNEL
            menuitems->add(new ObjectNumberControl<DeviceBehaviour_CVInput,byte>("Channel", this, &DeviceBehaviour_CVInput::set_channel, &DeviceBehaviour_CVInput::get_channel));
        #endif

        //menuitems->add(new ToggleControl<bool>("Debug", &this->debug));

        menuitems->add(new ObjectToggleControl<DeviceBehaviour_CVInput>("Quantise to scale", this, &DeviceBehaviour_CVInput::set_quantise, &DeviceBehaviour_CVInput::is_quantise));
        menuitems->add(new ObjectToggleControl<DeviceBehaviour_CVInput>("Play chords", this, &DeviceBehaviour_CVInput::set_play_chords, &DeviceBehaviour_CVInput::is_play_chords));

        ObjectSelectorControl<DeviceBehaviour_CVInput,CHORD> *selected_chord_control = new ObjectSelectorControl<DeviceBehaviour_CVInput,CHORD>("Chord selection", this, &DeviceBehaviour_CVInput::set_selected_chord, &DeviceBehaviour_CVInput::get_selected_chord);
        selected_chord_control->add_available_value(CHORD::TRIAD, chords[CHORD::TRIAD].label);
        selected_chord_control->add_available_value(CHORD::SUS2, chords[CHORD::SUS2].label);
        selected_chord_control->add_available_value(CHORD::SUS4, chords[CHORD::SUS4].label);
        selected_chord_control->add_available_value(CHORD::SEVENTH, chords[CHORD::SEVENTH].label);
        menuitems->add(selected_chord_control);

        menuitems->add(new ObjectScaleMenuItem<DeviceBehaviour_CVInput>("Scale", this, &DeviceBehaviour_CVInput::set_scale, &DeviceBehaviour_CVInput::get_scale, &DeviceBehaviour_CVInput::set_scale_root, &DeviceBehaviour_CVInput::get_scale_root, false));

        menuitems->add(new ToggleControl<bool>("Debug", &this->debug));

        Serial.println(F("returning..")); Serial_flush();
        return menuitems;
    }
#endif

    DeviceBehaviour_CVInput *behaviour_cvinput = new DeviceBehaviour_CVInput(); //(midi::MidiInterface<midi::SerialMIDI<HardwareSerial>>*)nullptr, &ENABLE_BITBOX);
#endif
