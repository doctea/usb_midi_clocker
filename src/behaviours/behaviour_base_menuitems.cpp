#ifdef ENABLE_SCREEN

    #include "behaviours/behaviour_base.h"
    #include "menu.h"
    #include "menuitems.h"

    //FLASHMEM 
    LinkedList<MenuItem*> *DeviceBehaviourUltimateBase::make_menu_items() {
        if (this->menuitems == nullptr) {
            this->menuitems = new LinkedList<MenuItem*>();
            //this->menuitems->add(new SeparatorMenuItem((char*)this->get_label(), C_WHITE));
            //this->menuitems->add(new MenuItem((char*)this->get_label()));
        }

        if (this->has_output()) {
            SubMenuItemBar *bar = new SubMenuItemBar((String(this->get_label()) + String(F(" Note limits"))).c_str());

            char output_label[MENU_C_MAX];
            snprintf(output_label, MENU_C_MAX, "%s output", this->get_label());

            // todo: move this and transpose_control into base Behaviour...?
            HarmonyStatus *harmony = new HarmonyStatus(
                output_label, 
                &this->last_transposed_note, 
                &this->current_transposed_note, 
                &this->last_drone_note
            );
            menuitems->add(harmony);
        
            SubMenuItemBar *transposition_bar = new SubMenuItemBar("Transpose");
            ObjectScaleNoteMenuItem<DeviceBehaviourUltimateBase,int8_t> *lowest_note_control = new ObjectScaleNoteMenuItem<DeviceBehaviourUltimateBase,int8_t>(
                "Lowest Note",
                this,
                &DeviceBehaviourUltimateBase::setLowestNote,
                &DeviceBehaviourUltimateBase::getLowestNote,
                nullptr,
                (int8_t)0,
                (int8_t)127,
                true,
                true
            );
            transposition_bar->add(lowest_note_control);

            ObjectScaleNoteMenuItem<DeviceBehaviourUltimateBase,int8_t> *highest_note_control = new ObjectScaleNoteMenuItem<DeviceBehaviourUltimateBase,int8_t>(
                "Highest Note",
                this,
                &DeviceBehaviourUltimateBase::setHighestNote,
                &DeviceBehaviourUltimateBase::getHighestNote,
                nullptr,
                (int8_t)0,
                (int8_t)127,
                true,
                true
            );
            transposition_bar->add(highest_note_control);

            ObjectSelectorControl<DeviceBehaviourUltimateBase,int8_t> *lowest_note_mode_control = new ObjectSelectorControl<DeviceBehaviourUltimateBase,int8_t>(
                "Low Mode",
                this,
                &DeviceBehaviourUltimateBase::setLowestNoteMode,
                &DeviceBehaviourUltimateBase::getLowestNoteMode,
                nullptr,
                true
            );
            lowest_note_mode_control->add_available_value(NOTE_MODE::IGNORE, "Ignore");
            lowest_note_mode_control->add_available_value(NOTE_MODE::TRANSPOSE, "Transpose");
            transposition_bar->add(lowest_note_mode_control);

            ObjectSelectorControl<DeviceBehaviourUltimateBase,int8_t> *highest_note_mode_control = new ObjectSelectorControl<DeviceBehaviourUltimateBase,int8_t>(
                "High Mode",
                this,
                &DeviceBehaviourUltimateBase::setHighestNoteMode,
                &DeviceBehaviourUltimateBase::getHighestNoteMode,
                nullptr,
                true
            );
            highest_note_mode_control->set_available_values(lowest_note_mode_control->available_values);
            transposition_bar->add(highest_note_mode_control);

            menuitems->add(transposition_bar);
        }

        return this->menuitems;
    }
#endif