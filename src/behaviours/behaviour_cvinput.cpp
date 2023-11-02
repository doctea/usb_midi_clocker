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
    #include "menuitems_lambda_selector.h"
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
                parameter_manager->get_available_pitch_inputs(),
                this->pitch_input
        );
        bar->add(pitch_parameter_selector);

        this->velocity_parameter_selector 
            = new ParameterInputSelectorControl<DeviceBehaviour_CVInput> (
                "Velocity Input",
                this,
                &DeviceBehaviour_CVInput::set_selected_velocity_input,
                parameter_manager->available_inputs,
                this->pitch_input
        );
        bar->add(velocity_parameter_selector);
        menuitems->add(bar);

        #ifdef DEBUG_VELOCITY
            DirectNumberControl<int8_t> *velocity_control = new DirectNumberControl<int8_t>("Velocity", &this->velocity, 127, 0, 127);
            menuitems->add(velocity_control);
        #endif

        //Serial.println(F("DeviceBehaviour_CVInput::make_menu_items() setting up HarmonyStatus")); Serial_flush();
        HarmonyStatus *harmony = new HarmonyStatus("CV->MIDI pitch", 
            &this->last_note, 
            &this->current_note,
            &this->current_raw_note
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
        bar = new SubMenuItemBar("Trigger/durations");
        //Serial.println(F("about to create length_ticks_control ObjectSelectorControl..")); Serial_flush();
        LambdaSelectorControl<int32_t> *length_ticks_control 
            = new LambdaSelectorControl<int32_t>(
                "Note length",
                [=](int32_t v) -> void { this->set_note_length(v); },
                [=]() -> int32_t { return this->get_note_length(); },
                nullptr,
                true
        );
        //Serial.println(F("about to add values..")); Serial_flush();
        length_ticks_control->add_available_value(0,                 "None");
        length_ticks_control->add_available_value(PPQN/PPQN,         "-");
        length_ticks_control->add_available_value(PPQN/8,            "32nd");
        length_ticks_control->add_available_value(PPQN/4,            "16th");
        length_ticks_control->add_available_value(PPQN/3,            "12th");
        length_ticks_control->add_available_value(PPQN/2,            "8th");
        length_ticks_control->add_available_value(PPQN,              "Beat");
        length_ticks_control->add_available_value(PPQN*2,            "2xBeat");
        length_ticks_control->add_available_value(PPQN*4,            "Bar");
        //Serial.println(F("about to add to menuitems list..")); Serial_flush();
        bar->add(length_ticks_control);

        //Serial.println(F("about to create length_ticks_control ObjectSelectorControl..")); Serial_flush();
        LambdaSelectorControl<int32_t> *trigger_ticks_control 
            = new LambdaSelectorControl<int32_t>(
                "Trigger each",
                [=](int32_t v) -> void { this->set_trigger_on_ticks(v); },
                [=]() -> int32_t { return this->get_trigger_on_ticks(); },
                nullptr,
                true
        );
        trigger_ticks_control->add_available_value(0,                 "Change");
        //trigger_ticks_control->add_available_value(PPQN/PPQN,         "-");
        trigger_ticks_control->add_available_value(PPQN/8,            "32nd");
        trigger_ticks_control->add_available_value(PPQN/4,            "16th");
        trigger_ticks_control->add_available_value(PPQN/3,            "12th");
        trigger_ticks_control->add_available_value(PPQN/2,            "8th");
        trigger_ticks_control->add_available_value(PPQN,              "Beat");
        trigger_ticks_control->add_available_value(PPQN*2,            "2xBeat");
        trigger_ticks_control->add_available_value(PPQN*4,            "Bar");
        //Serial.println(F("about to add to menuitems list..")); Serial_flush();
        bar->add(trigger_ticks_control);

        LambdaSelectorControl<int32_t> *trigger_delay_ticks_control 
            = new LambdaSelectorControl<int32_t>(
                "Delay",
                [=](int32_t v) -> void { this->set_trigger_delay_ticks(v); },
                [=]() -> int32_t { return this->get_trigger_delay_ticks(); },
                nullptr,
                true
        );
        //trigger_ticks_control->add_available_value(0,                 "Change");
        //trigger_ticks_control->add_available_value(PPQN/PPQN,         "-");
        trigger_delay_ticks_control->add_available_value(0,                 "None");
        trigger_delay_ticks_control->add_available_value(PPQN/8,            "32nd");
        trigger_delay_ticks_control->add_available_value(PPQN/4,            "16th");
        trigger_delay_ticks_control->add_available_value(PPQN/3,            "12th");
        trigger_delay_ticks_control->add_available_value(PPQN/2,            "8th");
        trigger_delay_ticks_control->add_available_value(PPQN,              "Beat");
        trigger_delay_ticks_control->add_available_value(PPQN*2,            "2xBeat");
        //trigger_ticks_control->add_available_value(PPQN*4,            "Bar");
        bar->add(trigger_delay_ticks_control);

        menuitems->add(bar);

        #ifdef CVINPUT_CONFIGURABLE_CHANNEL
            menuitems->add(new LambdaNumberControl<byte>("Channel", [=](byte v) -> void { this->set_channel(v); }, [=]() -> byte { return this->get_channel(); }));
        #endif

        //menuitems->add(new ToggleControl<bool>("Debug", &this->debug));

        // TODO: move scale/key and quantise items to a dedicated class that can be re-used
        // TODO: make mono, fake-poly, and true-poly versions of class:-
        //          mono forces one note at a time, and doesn't offer auto-chord function?
        //          fake-poly offers auto-chord functions
        //          true-poly doesn't offer auto-chord functions
        //          all versions offer quantisation to scale
        // TODO: allow all pitched behaviours to use a 'global scale' setting (-1?)
        menuitems->add(
            new LambdaScaleMenuItemBar(
                "Scale / Key", 
                [=](SCALE scale) -> void { this->set_scale(scale); }, 
                [=]() -> SCALE { return this->get_scale(); },
                [=](int8_t scale_root) -> void { this->set_scale_root(scale_root); },
                [=]() -> int8_t { return this->get_scale_root(); },
                true
                //, false
            )
        );

        bar = new SubMenuItemBar("Quantise / chords");
        bar->add(new LambdaToggleControl("Quantise",    
            [=](bool v) -> void { this->set_quantise(v); },
            [=]() -> bool { return this->is_quantise(); }
        ));
        bar->add(new LambdaToggleControl("Play chords", 
            [=](bool v) -> void { this->set_play_chords(v); },
            [=]() -> bool { return this->is_play_chords(); }
        ));

        LambdaSelectorControl<CHORD::Type> *selected_chord_control = new LambdaSelectorControl<CHORD::Type>(
            "Chord", 
            [=](CHORD::Type chord_type) -> void { this->set_selected_chord(chord_type); }, 
            [=]() -> CHORD::Type { return this->get_selected_chord(); },
            nullptr, true
        );
        for (size_t i = 0 ; i < NUMBER_CHORDS ; i++) {
            selected_chord_control->add_available_value(i, chords[i].label);
        }

        bar->add(new LambdaNumberControl<int8_t>("Inversion", 
            //this, &DeviceBehaviour_CVInput::set_inversion, &DeviceBehaviour_CVInput::get_inversion, 
            [=](int8_t v) -> void { this->set_inversion(v); }, 
            [=]() -> int8_t { return this->get_inversion(); },
            nullptr, 0, 4, true
        ));
        bar->add(selected_chord_control);

        menuitems->add(bar);

        menuitems->add(new ChordMenuItem("Current chord",   &this->current_chord_data));
        menuitems->add(new ChordMenuItem("Last chord",      &this->last_chord_data));

        menuitems->add(new ToggleControl<bool>("Debug", &this->debug));

        //Serial.println(F("returning..")); Serial_flush();
        return menuitems;
    }
#endif

    DeviceBehaviour_CVInput *behaviour_cvinput = new DeviceBehaviour_CVInput(); //(midi::MidiInterface<midi::SerialMIDI<HardwareSerial>>*)nullptr, &ENABLE_BITBOX);
#endif
