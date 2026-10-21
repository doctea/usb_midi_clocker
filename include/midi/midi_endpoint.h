#pragma once

#include <stdint.h>
#include <string.h>

// Anything that owns MIDI matrix endpoints (sources and/or targets): behaviours,
// and built-in ports (serial MIDI, PC USB, loopers) via MIDIPortOwner.
// Used to group endpoints for display and to find the page that represents them.
class MIDIEndpointOwner {
    public:
        int16_t routing_page_index = -1;   // index into Menu::pages; -1 = none

        virtual ~MIDIEndpointOwner() = default;

        virtual const char *get_endpoint_owner_label() = 0;
        virtual uint16_t get_endpoint_owner_colour() { return 0xFFFF; }
        // false when the owner represents an absent device (eg unplugged USB device)
        virtual bool is_endpoint_owner_online() { return true; }
        // true when the owner derives its source pitches from the current harmony and re-issues
        // them itself on harmony change (eg chord generators); the matrix then leaves their held
        // notes alone rather than re-quantising them per connection
        virtual bool regenerates_notes_on_harmony_change() { return false; }
};

// Lightweight owner for built-in endpoints that have no behaviour.
class MIDIPortOwner : public MIDIEndpointOwner {
    public:
        static const uint8_t MAX_LABEL = 20;
        char label[MAX_LABEL];
        uint16_t colour = 0xFFFF;

        MIDIPortOwner(const char *label, uint16_t colour = 0xFFFF) : colour(colour) {
            strncpy(this->label, label, MAX_LABEL - 1);
            this->label[MAX_LABEL - 1] = '\0';
        }

        virtual const char *get_endpoint_owner_label() override { return label; }
        virtual uint16_t get_endpoint_owner_colour() override { return colour; }
};
