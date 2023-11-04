#ifdef ENABLE_SCREEN

    #include "behaviours/behaviour_base.h"
    #include "menu.h"
    #include "menuitems.h"

    #include "menuitems_lambda_selector.h"

    #include "submenuitem_bar.h"
    #include "mymenu/menuitems_scale.h"

    //FLASHMEM 
    LinkedList<MenuItem*> *DeviceBehaviourUltimateBase::make_menu_items() {
        if (this->menuitems == nullptr) {
            this->menuitems = new LinkedList<MenuItem*>();
            //this->menuitems->add(new SeparatorMenuItem((char*)this->get_label(), C_WHITE));
            //this->menuitems->add(new MenuItem((char*)this->get_label()));

            // create the controls for the underlying device type first
            this->make_menu_items_device();
            
            // create controls for all devices that can transmit midi notes (ie send out from device)
            if (this->transmits_midi_notes()) {
                char output_label[MAX_LABEL_LENGTH];
                snprintf(output_label, MAX_LABEL_LENGTH, "%s output", this->get_label()); //(post-transpose)

                // todo: move this and transpose_control into base Behaviour...?
                output_harmony_status = new HarmonyStatus(
                    output_label, 
                    &this->last_transposed_note, 
                    &this->current_transposed_note
                    /*,  &this->last_drone_note*/
                );
                menuitems->add(output_harmony_status);
            
                SubMenuItemBar *transposition_bar = new SubMenuItemBar("Transpose");
                LambdaScaleNoteMenuItem<int8_t> *lowest_note_control = new LambdaScaleNoteMenuItem<int8_t>(
                    "Lowest",
                    [=](int8_t v) -> void { this->setLowestNote(v); },
                    [=]() -> int8_t { return this->getLowestNote(); },
                    nullptr,
                    (int8_t)MIDI_MIN_NOTE,
                    (int8_t)MIDI_MAX_NOTE,
                    true,
                    true
                );
                transposition_bar->add(lowest_note_control);

                LambdaScaleNoteMenuItem<int8_t> *highest_note_control = new LambdaScaleNoteMenuItem<int8_t>(
                    "Highest",
                    [=](int8_t v) -> void { this->setHighestNote(v); },
                    [=]() -> int8_t { return this->getHighestNote(); },
                    nullptr,
                    (int8_t)MIDI_MIN_NOTE,
                    (int8_t)MIDI_MAX_NOTE,
                    true,
                    true
                );
                transposition_bar->add(highest_note_control);

                LambdaSelectorControl<int8_t> *lowest_note_mode_control = new LambdaSelectorControl<int8_t>(
                    "Low Mode",
                    [=](int8_t v) -> void { this->setLowestNoteMode(v); },
                    [=]() -> int8_t { return this->getLowestNoteMode(); },
                    nullptr,
                    true
                );
                lowest_note_mode_control->add_available_value(NOTE_MODE::IGNORE, "Ignore");
                lowest_note_mode_control->add_available_value(NOTE_MODE::TRANSPOSE, "Transpose");
                transposition_bar->add(lowest_note_mode_control);

                LambdaSelectorControl<int8_t> *highest_note_mode_control = new LambdaSelectorControl<int8_t>(
                    "High Mode",
                    [=](int8_t v) -> void { this->setHighestNoteMode(v); },
                    [=]() -> int8_t { return this->getHighestNoteMode(); },
                    nullptr,
                    true
                );
                highest_note_mode_control->set_available_values(lowest_note_mode_control->available_values);
                transposition_bar->add(highest_note_mode_control);

                menuitems->add(transposition_bar);
            }
        }

        return this->menuitems;
    }
   
            
#endif