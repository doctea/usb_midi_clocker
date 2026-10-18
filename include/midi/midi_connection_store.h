#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

enum class MIDIConnectionQuantiseMode : uint8_t {
    INHERIT_BEHAVIOUR = 0,
    FORCE_OFF = 1,
    FORCE_SCALE = 2,
    FORCE_CHORD = 3,
};

enum class MIDIConnectionNoteMapMode : uint8_t {
    NONE = 0,
    RESERVED = 1,
};

struct MIDIConnectionPolicy {
    uint8_t fixed_channel = 0;
    MIDIConnectionQuantiseMode quantise_mode = MIDIConnectionQuantiseMode::INHERIT_BEHAVIOUR;
    MIDIConnectionNoteMapMode note_map_mode = MIDIConnectionNoteMapMode::NONE;
    bool paused = false;
};

inline bool midi_connection_policy_valid(const MIDIConnectionPolicy &policy) {
    return policy.fixed_channel <= 16
        && policy.quantise_mode <= MIDIConnectionQuantiseMode::FORCE_CHORD
        && policy.note_map_mode <= MIDIConnectionNoteMapMode::RESERVED;
}

// Named fields allow future additions; unknown fields are ignored, but malformed
// known fields and unsupported versions must not silently change a saved route.
inline bool midi_parse_connection_policy(const char *text, MIDIConnectionPolicy &result) {
    if (text == nullptr) return false;
    MIDIConnectionPolicy policy;
    for (const char *field = text; *field;) {
        const char *end = strchr(field, ';');
        if (end == nullptr) end = field + strlen(field);
        const char *equals = static_cast<const char *>(memchr(field, '=', end - field));
        const char *key = field;
        while (key < end && isspace(static_cast<unsigned char>(*key))) ++key;
        const char *key_end = equals != nullptr ? equals : end;
        while (key_end > key && isspace(static_cast<unsigned char>(key_end[-1]))) --key_end;
        const size_t length = key_end - key;
        const bool version = length == 7 && strncmp(key, "version", length) == 0;
        const bool channel = length == 7 && strncmp(key, "channel", length) == 0;
        const bool quantise = length == 8 && strncmp(key, "quantise", length) == 0;
        const bool note_map = length == 8 && strncmp(key, "note_map", length) == 0;
        const bool paused = length == 6 && strncmp(key, "paused", length) == 0;
        if (version || channel || quantise || note_map || paused) {
            if (equals == nullptr) return false;
            const char *value = equals + 1;
            while (value < end && isspace(static_cast<unsigned char>(*value))) ++value;
            if (value == end || *value < '0' || *value > '9') return false;
            unsigned int number = 0;
            while (value < end && *value >= '0' && *value <= '9') {
                number = number * 10 + (*value++ - '0');
                if (number > 255) return false;
            }
            while (value < end && isspace(static_cast<unsigned char>(*value))) ++value;
            if (value != end) return false;
            if (version) {
                if (number != 1 && number != 2) return false;
            } else if (channel) {
                if (number > 16) return false;
                policy.fixed_channel = number;
            } else if (quantise) {
                if (number > static_cast<unsigned int>(MIDIConnectionQuantiseMode::FORCE_CHORD)) return false;
                policy.quantise_mode = static_cast<MIDIConnectionQuantiseMode>(number);
            } else if (note_map) {
                if (number > static_cast<unsigned int>(MIDIConnectionNoteMapMode::RESERVED)) return false;
                policy.note_map_mode = static_cast<MIDIConnectionNoteMapMode>(number);
            } else {
                if (number > 1) return false;
                policy.paused = number != 0;
            }
        }
        field = *end ? end + 1 : end;
    }
    result = policy;
    return true;
}

inline bool midi_format_connection_policy(char *out, size_t size, const MIDIConnectionPolicy &policy) {
    const int length = snprintf(out, size, "version=2;channel=%u;quantise=%u;note_map=%u;paused=%u",
        static_cast<unsigned int>(policy.fixed_channel),
        static_cast<unsigned int>(policy.quantise_mode),
        static_cast<unsigned int>(policy.note_map_mode), policy.paused ? 1u : 0u);
    return length >= 0 && static_cast<size_t>(length) < size;
}

template <uint8_t SourceLimit, uint8_t TargetLimit, uint16_t Capacity = 128, uint8_t DisallowedCapacity = 16>
class MIDIConnectionStore {
public:
    static_assert(Capacity > 0 && Capacity < 255, "Connection indices must fit in a byte");
    static constexpr uint8_t END = 255;
    struct connection_t {
        uint8_t source = END;
        uint8_t target = END;
        uint8_t next_for_source = END;
        MIDIConnectionPolicy policy;
        uint32_t last_activity_ms = 0;
        bool has_activity = false;
    };

    MIDIConnectionStore() { clear(); }

    connection_t *find(int source, int target) {
        if (!valid_pair(source, target)) return nullptr;
        for (uint8_t index = heads[source]; index != END; index = entries[index].next_for_source)
            if (entries[index].target == target) return &entries[index];
        return nullptr;
    }
    const connection_t *find(int source, int target) const {
        if (!valid_pair(source, target)) return nullptr;
        for (uint8_t index = heads[source]; index != END; index = entries[index].next_for_source)
            if (entries[index].target == target) return &entries[index];
        return nullptr;
    }

    connection_t *connect(int source, int target, const MIDIConnectionPolicy &policy) {
        if (!allowed(source, target) || !midi_connection_policy_valid(policy)) return nullptr;
        if (auto *existing = find(source, target)) return existing;
        for (uint16_t index = 0; index < Capacity; ++index) {
            if (entries[index].source != END) continue;
            connection_t &entry = entries[index];
            entry = connection_t{};
            entry.source = source;
            entry.target = target;
            entry.policy = policy;
            // Preserve the matrix's original ascending-target delivery order.
            uint8_t *link = &heads[source];
            while (*link != END && entries[*link].target < target)
                link = &entries[*link].next_for_source;
            entry.next_for_source = *link;
            *link = static_cast<uint8_t>(index);
            ++used;
            return &entry;
        }
        return nullptr;
    }

    bool disconnect(int source, int target) {
        if (!valid_pair(source, target)) return false;
        uint8_t *link = &heads[source];
        while (*link != END) {
            connection_t &entry = entries[*link];
            if (entry.target == target) {
                *link = entry.next_for_source;
                entry = connection_t{};
                --used;
                return true;
            }
            link = &entry.next_for_source;
        }
        return false;
    }

    template <typename Fn> void for_each_from(int source, const Fn &fn) {
        if (source < 0 || source >= SourceLimit) return;
        for (uint8_t index = heads[source]; index != END;) {
            connection_t &entry = entries[index];
            const uint8_t next = entry.next_for_source;
            fn(entry);
            index = next;
        }
    }
    template <typename Fn> void for_each_to(int target, const Fn &fn) const {
        if (target < 0 || target >= TargetLimit) return;
        for (const auto &entry : entries)
            if (entry.source != END && entry.target == target) fn(entry);
    }

    bool disallow(int source, int target) {
        if (!valid_pair(source, target)) return false;
        for (uint8_t index = 0; index < disallowed_count; ++index)
            if (disallowed[index].source == source && disallowed[index].target == target) return true;
        if (disallowed_count == DisallowedCapacity) {
            disallow_overflow = true; // Fail closed rather than permit an unrecorded feedback loop.
            return false;
        }
        disallowed[disallowed_count++] = {static_cast<uint8_t>(source), static_cast<uint8_t>(target)};
        return true;
    }
    bool allowed(int source, int target) const {
        if (!valid_pair(source, target) || disallow_overflow) return false;
        for (uint8_t index = 0; index < disallowed_count; ++index)
            if (disallowed[index].source == source && disallowed[index].target == target) return false;
        return true;
    }
    uint16_t count() const { return used; }
    void clear() {
        for (auto &entry : entries) entry = connection_t{};
        for (auto &head : heads) head = END;
        used = 0;
    }

private:
    bool valid_pair(int source, int target) const {
        return source >= 0 && source < SourceLimit && target >= 0 && target < TargetLimit;
    }
    connection_t entries[Capacity];
    uint8_t heads[SourceLimit];
    uint16_t used = 0;
    struct pair_t { uint8_t source, target; };
    pair_t disallowed[DisallowedCapacity] = {};
    uint8_t disallowed_count = 0;
    bool disallow_overflow = false;
};
