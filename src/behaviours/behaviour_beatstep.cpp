#include "Config.h"
#ifdef ENABLE_BEATSTEP

#include "behaviours/behaviour_beatstep.h"

DeviceBehaviour_Beatstep    *behaviour_beatstep     = new DeviceBehaviour_Beatstep();
#ifdef ENABLE_BEATSTEP_2
    DeviceBehaviour_Beatstep_2  *behaviour_beatstep_2   = new DeviceBehaviour_Beatstep_2();
#endif
/*void beatstep_handle_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    if (behaviour_beatstep!=nullptr) behaviour_beatstep->receive_control_change(inChannel, inNumber, inValue);
}*/

void beatstep_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_beatstep!=nullptr) behaviour_beatstep->receive_note_on(inChannel, inNumber, inVelocity);
}

void beatstep_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_beatstep!=nullptr) behaviour_beatstep->receive_note_off(inChannel, inNumber, inVelocity);
}

void beatstep_handle_sysex(const uint8_t *data, uint16_t length, bool complete) {
    if (behaviour_beatstep!=nullptr) behaviour_beatstep->handle_sysex(data, length, complete);
}

#ifdef ENABLE_BEATSTEP_2
    void beatstep_2_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
        if (behaviour_beatstep_2!=nullptr) behaviour_beatstep_2->receive_note_on(inChannel, inNumber, inVelocity);
    }

    void beatstep_2_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
        if (behaviour_beatstep_2!=nullptr) behaviour_beatstep_2->receive_note_off(inChannel, inNumber, inVelocity);
    }

    void beatstep_2_handle_sysex(const uint8_t *data, uint16_t length, bool complete) {
        if (behaviour_beatstep_2!=nullptr) behaviour_beatstep_2->handle_sysex(data, length, complete);
    }
#endif


#ifdef ENABLE_SCREEN
    #include "mymenu/menu_looper.h"

    #include "submenuitem_bar.h"
    #include "menuitems_lambda_selector.h"
    #include "menuitems_object_selector.h"
    #include "menuitems_numbers.h"
    #include "menuitems_object_multitoggle.h"
    #include "mymenu/menuitems_harmony.h"

    FLASHMEM //DeviceBehaviour_Beatstep::make_menu_items() causes a section type conflict with virtual void DeviceBehaviour_Beatstep::setup_callbacks()
    LinkedList<MenuItem*> *DeviceBehaviour_Beatstep::make_menu_items() {
        DeviceBehaviourUltimateBase::make_menu_items();

        this->menuitems->add(new HarmonyStatus("Beatstep harmony",   &this->last_note,          &this->current_note));
        DividedClockedBehaviour::make_menu_items();

        SubMenuItemBar *pattern_options = new SubMenuItemBar("Pattern options");
        pattern_options->add(new LambdaNumberControl<int8_t>(
            "Pattern length",   
            //this, 
            //&DeviceBehaviour_Beatstep::setPatternLength,  &DeviceBehaviour_Beatstep::getPatternLength, 
            [=](int8_t length) -> void { this->setPatternLength(length); },
            [=]() -> int8_t { return this->getPatternLength(); },
            nullptr, BEATSTEP_PATTERN_LENGTH_MINIMUM, BEATSTEP_PATTERN_LENGTH_MAXIMUM
        ));

        LambdaSelectorControl<int8_t> *direction = new LambdaSelectorControl<int8_t>(
            "Direction",        
            //this, &DeviceBehaviour_Beatstep::setDirection,      &DeviceBehaviour_Beatstep::getDirection
            [=](int8_t length) -> void { this->setDirection(length); },
            [=]() -> int8_t { return this->getDirection(); }
        );
        direction->add_available_value(0, "Fwd");
        direction->add_available_value(1, "Rev");
        direction->add_available_value(2, "Alt");
        direction->add_available_value(3, "Rnd");
        pattern_options->add(direction);
        menuitems->add(pattern_options);

        SubMenuItemBar *note_options = new SubMenuItemBar("Note options");    
        LambdaNumberControl<int8_t> *swing = new LambdaNumberControl<int8_t>(
            "Swing",
            [=](int8_t length) -> void { this->setSwing(length); },
            [=]() -> int8_t { return this->getSwing(); },
            nullptr,
            0x32, 
            0x4b
        );
        swing->int_unit = '%';
        note_options->add(swing);

        LambdaNumberControl<int8_t> *gate = new LambdaNumberControl<int8_t>(
            "Gate length", 
            [=](int8_t length) -> void { this->setGate(length); },
            [=]() -> int8_t { return this->getGate(); },
            nullptr,
            0x32, 
            0x63
        );
        gate->int_unit = '%';
        note_options->add(gate);

        //NumberControl<int8_t> *legato = new NumberControl<int8_t>("Legato", &this->legato, 0x00, 0x00, 0x02);
        LambdaSelectorControl<int8_t> *legato = new LambdaSelectorControl<int8_t>(
            "Legato",
            [=](int8_t length) -> void { this->setLegato(length); },
            [=]() -> int8_t { return this->getLegato(); }
        );
        legato->add_available_value(0, "Off");
        legato->add_available_value(1, "On");
        legato->add_available_value(2, "Reset");
        note_options->add(legato);
        
        menuitems->add(note_options);

        menuitems->add(new LambdaToggleControl("Auto-fetch",        [=](bool v) -> void { this->setAutoFetch(v); }, [=]() -> bool { return this->getAutoFetch(); } ));
        menuitems->add(new LambdaActionItem("Request everything",   [=]() -> void { this->request_all_sysex_parameters(); } ));

        /* 
        // this is added to the multi-select control, so we don't need it here too
        #ifdef ENABLE_BEATSTEP_SYSEX
            ObjectToggleControl<DeviceBehaviour_Beatstep> *beatstep_auto_advance = new ObjectToggleControl<DeviceBehaviour_Beatstep> (
                "Beatstep auto-advance",
                this,
                &DeviceBehaviour_Beatstep::set_auto_advance_pattern,
                &DeviceBehaviour_Beatstep::is_auto_advance_pattern,
                nullptr
            );
            this->menuitems->add(beatstep_auto_advance);
        #endif*/
        return this->menuitems;
    }
#endif

#endif