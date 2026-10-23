#include "Config.h"

#ifdef ENABLE_SCREEN

#include "mymenu.h"
#include "mymenu/menu_midi_routing.h"

extern Menu *menu;

void midi_routing_ui_message(const char *text, bool error) {
    menu_set_last_message(text, error ? RED : GREEN);
}

void midi_routing_ui_jump_to_page(int16_t page_index) {
    if (menu != nullptr) menu->jump_to_page_with_return(page_index);
}

MIDIRoutingPanel *make_midi_routing_panel(MIDIEndpointOwner *owner, const char *label) {
    if (midi_matrix_manager == nullptr || owner == nullptr) return nullptr;
    if (!midi_matrix_manager->owns_endpoints(owner) && owner->get_linked_owner_count() == 0) return nullptr;
    return new MIDIRoutingPanel(label, midi_matrix_manager, owner);
}

void setup_menu_midi_ports() {
    if (midi_matrix_manager == nullptr || midi_matrix_manager->port_owners_count == 0) return;

    const int page_index = menu->add_page("MIDI Ports", C_WHITE, true, "Settings");
    for (uint8_t i = 0; i < midi_matrix_manager->port_owners_count; ++i) {
        MIDIEndpointOwner *owner = midi_matrix_manager->port_owners[i];
        MIDIRoutingPanel *panel = make_midi_routing_panel(owner, owner->get_endpoint_owner_label());
        if (panel == nullptr) continue;
        if (owner->routing_page_index < 0) owner->routing_page_index = page_index;
        menu->add(panel, owner->get_endpoint_owner_colour());
    }
}

#endif
