#include "Config.h"
#ifdef ENABLE_BEATSTEP

#include "behaviours/behaviour_beatstep.h"

DeviceBehaviour_Beatstep *behaviour_beatstep = new DeviceBehaviour_Beatstep();
/*void beatstep_handle_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    if (behaviour_beatstep!=nullptr) behaviour_beatstep->receive_control_change(inChannel, inNumber, inValue);
}*/

void beatstep_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_beatstep!=nullptr) behaviour_beatstep->receive_note_on(inChannel, inNumber, inVelocity);
}

void beatstep_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
    if (behaviour_beatstep!=nullptr) behaviour_beatstep->receive_note_off(inChannel, inNumber, inVelocity);
}

#ifdef ENABLE_SCREEN
    #include "mymenu/menu_looper.h"
    //FLASHMEM //DeviceBehaviour_Beatstep::make_menu_items() causes a section type conflict with virtual void DeviceBehaviour_Beatstep::setup_callbacks()
    LinkedList<MenuItem*> *DeviceBehaviour_Beatstep::make_menu_items() {
        DeviceBehaviourUltimateBase::make_menu_items();
        this->menuitems->add(
            new HarmonyStatus("Beatstep harmony",   &this->last_note,          &this->current_note)
        );
        DividedClockedBehaviour::make_menu_items();
        #ifdef ENABLE_BEATSTEP_SYSEX
            ObjectToggleControl<DeviceBehaviour_Beatstep> beatstep_auto_advance = new ObjectToggleControl<DeviceBehaviour_Beatstep> (
                "Beatstep auto-advance",
                this,
                &DeviceBehaviour_Beatstep::set_auto_advance_pattern,
                &DeviceBehaviour_Beatstep::is_auto_advance_pattern,
                nullptr
            );
            this->menuitems->add(beatstep_auto_advance);
        #endif
        return this->menuitems;
    }
#endif

#endif