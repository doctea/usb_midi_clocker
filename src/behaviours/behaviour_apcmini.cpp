#include "behaviours/behaviour_apcmini.h"

#ifdef ENABLE_APCMINI

DeviceBehaviour_APCMini *behaviour_apcmini;

void apcmini_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    if (behaviour_apcmini!=nullptr) behaviour_apcmini->receive_control_change(inChannel, inNumber, inValue);
}

void apcmini_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_apcmini!=nullptr) behaviour_apcmini->receive_note_on(inChannel, inNumber, inVelocity);
}

void apcmini_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_apcmini!=nullptr) behaviour_apcmini->receive_note_off(inChannel, inNumber, inVelocity);
}

#ifdef ENABLE_SCREEN
    #include "menuitems_lambda_selector.h"

    //FLASHMEM // causes a section type conflict with virtual void DeviceBehaviour_APCMini::setup_callbacks()
    LinkedList<MenuItem*> *DeviceBehaviour_APCMini::make_menu_items() {
        LinkedList<MenuItem*> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();

        LambdaSelectorControl<uint8_t> *tempo_selector_control = new LambdaSelectorControl<uint8_t>(
            "Tempo Control",
            [=] (int8_t v) -> void { this->tempo_fader_cc = v; },
            [=] (void) -> int8_t { return this->tempo_fader_cc; },
            nullptr,
            true,
            false
        );
        tempo_selector_control->add_available_value(-1, "None");
        tempo_selector_control->add_available_value(APCMINI_FADER_CC_MASTER, "Master");
        tempo_selector_control->add_available_value(APCMINI_FADER_CC_1, "Fader 1");
        tempo_selector_control->add_available_value(APCMINI_FADER_CC_2, "Fader 2");
        tempo_selector_control->add_available_value(APCMINI_FADER_CC_3, "Fader 3");
        tempo_selector_control->add_available_value(APCMINI_FADER_CC_4, "Fader 4");
        tempo_selector_control->add_available_value(APCMINI_FADER_CC_5, "Fader 5");
        tempo_selector_control->add_available_value(APCMINI_FADER_CC_6, "Fader 6");
        tempo_selector_control->add_available_value(APCMINI_FADER_CC_7, "Fader 7");
        tempo_selector_control->add_available_value(APCMINI_FADER_CC_8, "Fader 8");

        menuitems->add(tempo_selector_control);

        return menuitems;
    }

#endif

#ifdef ENABLE_APCMINI_PROGRESSIONS
    #include "behaviours/behaviour_progression.h"

    bool DeviceBehaviour_APCMini::process_note_on_progressions_page(byte inChannel, byte inNumber, byte inVelocity) {
        if (inNumber>=0 && inNumber < NUM_SEQUENCES * APCMINI_DISPLAY_WIDTH) {
            return behaviour_progression->apcmini_press(inNumber, apcmini_shift_held);
        }
        return false;
    }

    bool DeviceBehaviour_APCMini::process_note_off_progressions_page(byte inChannel, byte inNumber, byte inVelocity) {
        if (inNumber>=0 && inNumber < NUM_SEQUENCES * APCMINI_DISPLAY_WIDTH) {
            return behaviour_progression->apcmini_release(inNumber, apcmini_shift_held);
        }
        return false;
    }
#endif

#endif