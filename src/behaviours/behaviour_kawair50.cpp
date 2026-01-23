#include "Config.h"

#ifdef ENABLE_KAWAI_R50
    #include "behaviours/behaviour_kawair50.h"

    LinkedList<MenuItem*> *DeviceBehaviour_KawaiR50::make_menu_items() {
        LinkedList<MenuItem*> *items = new LinkedList<MenuItem*>();

        MenuItem *header = new MenuItem("Kawai R50 Settings", false, true);
        items->add(header);

        // drum enable/disable submenu
        /*SubMenuItemBar *drum_enable_menu = new SubMenuItemBar("Drum Enable/Disable", true, true);
        for (uint8_t i = 0; i < MIDI_NUM_NOTES; i++) {
            DrumMapEntry *entry = drum_mapper.drum_map_storage[i];
            if (entry != nullptr) {
                String label = String(entry->name) + " (" + String(entry->incoming_midi_note) + ")";
                drum_enable_menu->add(new ToggleControl<bool>(label.c_str(), &entry->enabled));
            }
        }
        items->add(drum_enable_menu);*/

        ObjectMultiToggleControl *enabled_drums = new ObjectMultiToggleControl("Enable drums", true);
        for (uint8_t i = 0; i < MIDI_NUM_NOTES; i++) {
            DrumMapEntry *entry = drum_mapper.drum_map_storage[i];
            if (entry != nullptr) {
                String *label = new String(entry->name);
                enabled_drums->addItem(
                    new MultiToggleItemLambda(
                        label->c_str(),
                        [=](bool v) { entry->enabled = v; },
                        [=]() -> bool { return entry->enabled; }
                    )
                );
            }
        }
        items->add(enabled_drums);

        return items;
    }

    DeviceBehaviour_KawaiR50 *behaviour_kawair50 = nullptr; 
#endif