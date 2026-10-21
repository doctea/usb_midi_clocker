#pragma once

#include <stdint.h>

#ifdef ARDUINO
    #include <util/atomic.h>
    #define MIDI_NOTE_DIAGNOSTIC_ATOMIC ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
#else
    #define MIDI_NOTE_DIAGNOSTIC_ATOMIC if (true)
#endif

enum class MIDINoteTraceKind : uint8_t {
    SOURCE_ON, SOURCE_OFF, PAIRED_ON, PAIRED_OFF, UNMATCHED_OFF, LEDGER_FULL,
    PAUSED_ON, PAUSED_OFF, MIGRATE, TARGET_MIGRATE, MIGRATE_RESULT, HELD,
    DISPATCH_ON, DISPATCH_OFF, SEND_ON, SEND_OFF, SUPPRESSED_OFF, REFCOUNT_LIMIT,
    POLICY, CONNECT, DISCONNECT, RESET, PAUSE_RELEASE, CLEANUP, PANIC_DISCARD, PANIC_CHANNEL
};

inline const char *midi_note_trace_kind_name(MIDINoteTraceKind kind) {
    switch (kind) {
#define MIDI_TRACE_NAME(name) case MIDINoteTraceKind::name: return #name
        MIDI_TRACE_NAME(SOURCE_ON); MIDI_TRACE_NAME(SOURCE_OFF);
        MIDI_TRACE_NAME(PAIRED_ON); MIDI_TRACE_NAME(PAIRED_OFF);
        MIDI_TRACE_NAME(UNMATCHED_OFF); MIDI_TRACE_NAME(LEDGER_FULL);
        MIDI_TRACE_NAME(PAUSED_ON); MIDI_TRACE_NAME(PAUSED_OFF);
        MIDI_TRACE_NAME(MIGRATE); MIDI_TRACE_NAME(TARGET_MIGRATE);
        MIDI_TRACE_NAME(MIGRATE_RESULT); MIDI_TRACE_NAME(HELD);
        MIDI_TRACE_NAME(DISPATCH_ON); MIDI_TRACE_NAME(DISPATCH_OFF);
        MIDI_TRACE_NAME(SEND_ON); MIDI_TRACE_NAME(SEND_OFF);
        MIDI_TRACE_NAME(SUPPRESSED_OFF); MIDI_TRACE_NAME(REFCOUNT_LIMIT);
        MIDI_TRACE_NAME(POLICY); MIDI_TRACE_NAME(CONNECT);
        MIDI_TRACE_NAME(DISCONNECT); MIDI_TRACE_NAME(RESET);
        MIDI_TRACE_NAME(PAUSE_RELEASE); MIDI_TRACE_NAME(CLEANUP);
        MIDI_TRACE_NAME(PANIC_DISCARD); MIDI_TRACE_NAME(PANIC_CHANNEL);
#undef MIDI_TRACE_NAME
    }
    return "UNKNOWN";
}

struct MIDINoteTraceEvent {
    uint32_t time_ms = 0;
    uint32_t started_ms = 0;
    int8_t source = -1, target = -1;
    int8_t input_pitch = -1, output_pitch = -1;
    uint8_t input_channel = 0, output_channel = 0, velocity = 0;
    MIDINoteTraceKind kind = MIDINoteTraceKind::SOURCE_ON;
    int8_t refcount = -1;
    uint8_t fixed_channel = 0, quantise = 0, note_map = 0, global_quantise = 0;
    bool paused = false, emitted = false;
};

struct MIDINoteTargetDebugState {
    bool behaviour = false;
    int8_t tuning = 0, low = 0, high = 127;
    uint8_t low_mode = 0, high_mode = 0;
};

// Producers include clock interrupts. Only the bounded copy/update is atomic;
// allocation, formatting, transport calls and serial output must stay outside.
class MIDINoteTrace {
public:
    static constexpr uint16_t CAPACITY = 128;
    volatile bool enabled = false;
    int source_filter = -1, target_filter = -1;
    uint32_t overwritten = 0;

    bool accepts(int source, int target) const {
        return enabled && ((source == -1 && target == -1)
            || ((source_filter < 0 || source == source_filter)
            && (target_filter < 0 || target == target_filter)));
    }
    void record(const MIDINoteTraceEvent &event) {
        MIDI_NOTE_DIAGNOSTIC_ATOMIC {
            if (!accepts(event.source, event.target)) return;
            entries[next] = event;
            next = (next + 1) % CAPACITY;
            if (used < CAPACITY) ++used;
            else ++overwritten;
        }
    }
    void configure(int source, int target) {
        MIDI_NOTE_DIAGNOSTIC_ATOMIC {
            source_filter = source;
            target_filter = target;
            enabled = true;
        }
    }
    uint16_t count() const { return used; }
    const MIDINoteTraceEvent &at(uint16_t index) const {
        return entries[(next + CAPACITY - used + index) % CAPACITY];
    }
    void clear() {
        MIDI_NOTE_DIAGNOSTIC_ATOMIC { next = used = 0; overwritten = 0; }
    }

private:
    MIDINoteTraceEvent entries[CAPACITY];
    uint16_t next = 0, used = 0;
};

struct MIDINoteTraceContext {
    MIDINoteTrace *trace;
    MIDINoteTraceEvent event;

    void record(MIDINoteTraceKind kind, uint8_t pitch, uint8_t channel, int8_t refcount, uint32_t now_ms) {
        event.time_ms = now_ms;
        event.kind = kind;
        event.output_pitch = pitch;
        event.output_channel = channel;
        event.refcount = refcount;
        trace->record(event);
    }
};

class MIDINoteTraceScope {
public:
    MIDINoteTraceScope(MIDINoteTraceContext *&slot, MIDINoteTraceContext *context)
        : slot(slot), previous(slot) { slot = context; }
    ~MIDINoteTraceScope() { slot = previous; }
    MIDINoteTraceScope(const MIDINoteTraceScope &) = delete;
    MIDINoteTraceScope &operator=(const MIDINoteTraceScope &) = delete;

private:
    MIDINoteTraceContext *&slot;
    MIDINoteTraceContext *previous;
};
