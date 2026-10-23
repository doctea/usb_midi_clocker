#pragma once

// Per-owner routing panel: lists and edits the matrix routes into an owner's
// targets and out of its sources. Shown at the top of behaviour pages and, for
// built-in ports, on the 'MIDI Ports' page.

#include "menuitems_actionlist.h"
#include "midi/midi_routing_lists.h"

class MIDIRoutingPanel : public ActionListMenuItem {
    public:
        MIDIRoutingPanel(const char *label, MIDIMatrixManager *matrix, MIDIEndpointOwner *owner)
            : ActionListMenuItem(label, nullptr), routes(matrix, owner), matrix(matrix) {
            set_root(&routes);
        }

        MIDIEndpointOwner *get_owner() const { return routes.get_owner(); }

    protected:
        MIDIOwnerRoutesSource routes;
        MIDIMatrixManager *matrix;

        // closed: one line per direction listing the far ends, eg "<- Prog.Bass, ||TM Ch2"
        virtual int display_compact(Coord pos, bool selected, bool opened) override {
            tft->setCursor(pos.x, pos.y);
            pos.y = header(label, pos, selected, opened);
            tft->setCursor(pos.x, pos.y);
            begin_lines(pos.y);

            const uint8_t columns = text_columns();
            uint16_t colour = default_colour();
            if (!get_owner()->is_endpoint_owner_online()) colour = tft->halfbright_565(colour);
            char line[MENU_C_MAX + 1];
            if (routes.has_inputs()) {
                midi_routing_format_owner_summary(*matrix, get_owner(), MIDIRouteDirection::INBOUND, line, sizeof(line));
                colours(false, colour, BLACK);
                print_line(line, columns, pos.x);
            }
            if (routes.has_outputs()) {
                midi_routing_format_owner_summary(*matrix, get_owner(), MIDIRouteDirection::OUTBOUND, line, sizeof(line));
                colours(false, colour, BLACK);
                print_line(line, columns, pos.x);
            }
            for (uint8_t i = 0; i < routes.link_count(); ++i) {
                const MIDIEndpointOwnerLink link = get_owner()->get_linked_owner(i);
                midi_routing_format_link(link, line, sizeof(line));
                uint16_t link_colour = link.owner != nullptr ? link.owner->get_endpoint_owner_colour() : default_colour();
                if (link.owner != nullptr && !link.owner->is_endpoint_owner_online()) link_colour = tft->halfbright_565(link_colour);
                colours(false, link_colour, BLACK);
                print_line(line, columns, pos.x);
            }
            colours(false, default_colour(), BLACK);
            return end_lines();
        }
};

// nullptr when the matrix is not ready or the owner has no endpoints
MIDIRoutingPanel *make_midi_routing_panel(MIDIEndpointOwner *owner, const char *label = "MIDI routing");

// 'MIDI Ports' page: a routing panel for each built-in port owner
void setup_menu_midi_ports();
