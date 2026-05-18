#ifdef ENABLE_SCREEN

    #include "behaviours/behaviour_base.h"
    #include "menu.h"
    #include "menuitems.h"

    #include "menuitems_lambda_selector.h"

    #include "submenuitem_bar.h"
    #include "mymenu/menuitems_scale.h"
    #include "mymenu/menuitems_harmony.h"

    #include "mymenu/menuitem_notelimit.h"

    FLASHMEM 
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

                output_harmony_status = new HarmonyStatus(
                    output_label, 
                    &this->last_transposed_note, 
                    &this->current_transposed_note
                    /*,  &this->last_drone_note*/
                );
                menuitems->add(output_harmony_status);
            
                // todo: make this a custom combined type that we can share with seqlib 
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

                transposition_bar->add(new NoteLimitModeControl<>(
                    "Low Mode",
                    [=](NOTE_LIMIT_MODE v) -> void { this->setLowestNoteMode(v); },
                    [=]() -> NOTE_LIMIT_MODE { return this->getLowestNoteMode(); },
                    nullptr,
                    true,
                    true
                ));

                transposition_bar->add(new NoteLimitModeControl<>(
                    "High Mode",
                    [=](NOTE_LIMIT_MODE v) -> void { this->setHighestNoteMode(v); },
                    [=]() -> NOTE_LIMIT_MODE { return this->getHighestNoteMode(); },
                    nullptr,
                    true,
                    true
                ));

                menuitems->add(transposition_bar);
            }
        }

        return this->menuitems;
    }
   
            
#endif