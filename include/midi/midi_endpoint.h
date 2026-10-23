#pragma once

#include <stdint.h>
#include <string.h>

// Another owner tied to this one, eg the behaviour driving a MIDI port's output jack.
struct MIDIEndpointOwnerLink {
    const char *role = nullptr;            // eg "In", "Out", "In+Out"
    class MIDIEndpointOwner *owner = nullptr;
};

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
        // Routes from one of an owner's sources to one of its own targets are
        // refused unless this returns true: for a single device or behaviour they
        // are feedback loops (or recursion, for proxies that re-emit their input).
        virtual bool allows_self_routing() { return false; }
        // Related owners shown in this owner's routing panel so the user can jump to them,
        // eg a MIDI port lists the behaviours occupying its jacks.
        virtual uint8_t get_linked_owner_count() { return 0; }
        virtual MIDIEndpointOwnerLink get_linked_owner(uint8_t) { return MIDIEndpointOwnerLink(); }
};

// Lightweight owner for built-in endpoints that have no behaviour.
class MIDIPortOwner : public MIDIEndpointOwner {
    public:
        static const uint8_t MAX_LABEL = 20;
        static const uint8_t MAX_LINKS = 2;
        char label[MAX_LABEL];
        uint16_t colour = 0xFFFF;
        MIDIEndpointOwnerLink links[MAX_LINKS];
        uint8_t links_count = 0;

        MIDIPortOwner(const char *label, uint16_t colour = 0xFFFF) : colour(colour) {
            strncpy(this->label, label, MAX_LABEL - 1);
            this->label[MAX_LABEL - 1] = '\0';
        }

        virtual const char *get_endpoint_owner_label() override { return label; }
        virtual uint16_t get_endpoint_owner_colour() override { return colour; }
        // built-in port groupings pair independent jacks (eg port 1 In -> port 1 Out thru)
        virtual bool allows_self_routing() override { return true; }

        // records that a behaviour occupies one of this port's jacks
        bool add_linked_owner(const char *role, MIDIEndpointOwner *owner) {
            if (owner == nullptr || links_count >= MAX_LINKS) return false;
            links[links_count].role = role;
            links[links_count].owner = owner;
            links_count++;
            return true;
        }
        virtual uint8_t get_linked_owner_count() override { return links_count; }
        virtual MIDIEndpointOwnerLink get_linked_owner(uint8_t index) override {
            return index < links_count ? links[index] : MIDIEndpointOwnerLink();
        }
};
