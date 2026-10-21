#include "matrix_fixture.h"
#include <iostream>

MIDIMatrixManager *midi_matrix_manager = nullptr;

struct Fixture : MIDIMatrixManager {
    source_entry source_storage[MAX_NUM_SOURCES];
    MIDIOutputWrapper outputs[4];
    Fixture() {
        sources = source_storage;
        midi_matrix_manager = this;
        for (int index = 0; index < 4; ++index) {
            char handle[16];
            snprintf(handle, sizeof(handle), "source%d", index);
            assert(register_source(handle, index == 3 ? 10 : 1) == index);
            snprintf(handle, sizeof(handle), "target%d", index);
            assert(register_target(&outputs[index], handle) == index);
        }
    }
    bool load(const char *source, const char *target, const char *policy, sl_scope_t scope = SL_SCOPE_ALL) {
        char source_buf[80], target_buf[80], field[] = "connection";
        snprintf(source_buf, sizeof(source_buf), "%s", source);
        snprintf(target_buf, sizeof(target_buf), "%s", target);
        char *segments[] = {source_buf, target_buf, field};
        return load_line(segments, 3, policy, scope);
    }
};

static void store_tests() {
    MIDIConnectionStore<4, 4, 3, 1> store;
    MIDIConnectionPolicy policy;
    assert(store.count() == 0);
    assert(!store.find(-1, 0) && !store.find(4, 0) && !store.find(0, 4));
    assert(!store.connect(-2, 0, policy) && !store.allowed(0, -1));
    assert(store.connect(0, 3, policy));
    assert(store.connect(0, 1, policy));
    assert(store.connect(1, 0, policy));
    auto *entry = store.find(0, 1);
    entry->policy.paused = true;
    assert(store.connect(0, 1, policy) == entry && entry->policy.paused);
    assert(!store.connect(2, 2, policy) && store.count() == 3);
    std::vector<int> targets;
    store.for_each_from(0, [&](const decltype(store)::connection_t &route) { targets.push_back(route.target); });
    assert((targets == std::vector<int>{1, 3}));
    int inbound = 0;
    store.for_each_to(0, [&](const decltype(store)::connection_t &) { ++inbound; });
    assert(inbound == 1);
    assert(store.disconnect(0, 1) && store.count() == 2);
    assert(!store.disconnect(0, 1));
    assert(store.connect(0, 2, policy) && store.count() == 3);
    targets.clear();
    store.for_each_from(0, [&](const decltype(store)::connection_t &route) { targets.push_back(route.target); });
    assert((targets == std::vector<int>{2, 3}));
    store.for_each_from(0, [&](const decltype(store)::connection_t &route) {
        assert(store.disconnect(route.source, route.target));
    });
    assert(store.count() == 1);
    assert(store.disallow(0, 0) && store.disallow(0, 0));
    assert(!store.allowed(0, 0) && !store.connect(0, 0, policy));
    store.clear();
    assert(!store.allowed(0, 0) && store.count() == 0);
    assert(!store.disallow(1, 1) && !store.allowed(2, 2));

    MIDIConnectionStore<50, 50> full;
    for (int pair = 0; pair < 128; ++pair)
        assert(full.connect(pair / 50, pair % 50, policy));
    assert(full.count() == 128 && !full.connect(3, 0, policy));
    assert(full.disconnect(0, 20) && full.connect(3, 0, policy));
    assert(full.count() == 128);
}

static void parser_tests() {
    MIDIConnectionPolicy policy;
    policy.paused = true;
    assert(midi_parse_connection_policy("version=1;channel=16;quantise=3;note_map=1", policy));
    assert(!policy.paused && policy.fixed_channel == 16 && policy.quantise_mode == MIDIConnectionQuantiseMode::FORCE_CHORD);
    assert(midi_parse_connection_policy("version=2;channel=2;quantise=1;note_map=0;paused=1", policy));
    assert(policy.paused && policy.fixed_channel == 2);
    char text[96];
    assert(midi_format_connection_policy(text, sizeof(text), policy));
    assert(strcmp(text, "version=2;channel=2;quantise=1;note_map=0;paused=1") == 0);
    MIDIConnectionPolicy restored;
    assert(midi_parse_connection_policy(text, restored));
    assert(restored.paused && restored.fixed_channel == 2 && restored.quantise_mode == policy.quantise_mode);
    assert(!midi_format_connection_policy(text, 10, policy));
    assert(midi_parse_connection_policy(" version = 2 ; channel = 3 ; paused = 1 ;future=thing;", policy));
    assert(policy.paused && policy.fixed_channel == 3);
    for (const char *bad : {"version=3", "channel=17", "channel=-1", "quantise=4",
            "note_map=2", "paused=2", "channel=garbage", "paused=", "channel=1oops",
            "paused", "channel=9999999999999999999999999999999"}) {
        const auto previous = policy;
        assert(!midi_parse_connection_policy(bad, policy));
        assert(policy.paused == previous.paused && policy.fixed_channel == previous.fixed_channel);
    }
    assert(!midi_parse_connection_policy(nullptr, policy));
}

static void pause_and_routing_tests() {
    Fixture matrix;
    now_ms = 0;
    matrix.processControlChange(2, 1, 20, 1);
    assert(matrix.sources[2].has_activity && matrix.sources[2].last_activity_ms == 0);
    assert(!matrix.targets[0].has_activity);
    assert(matrix.connect(0, 0) && matrix.connect(1, 0) && matrix.connect(0, 1));
    assert(matrix.connected_to_source_count(0) == 2 && matrix.connected_to_target_count(0) == 2);
    now_ms = 10;
    matrix.processNoteOn(0, 60, 100, 1);
    matrix.processNoteOn(1, 60, 90, 1);
    assert(matrix.active_connection_notes.count() == 3);
    assert(matrix.held_notes_for_connection(0, 0) == 1);
    assert(matrix.outputs[0].playing_notes[60] == 2);
    assert(matrix.set_paused(0, 0, true));
    assert(matrix.connect(0, 0) && matrix.get_connection_policy(0, 0)->paused);
    assert(matrix.is_connected(0, 0) && matrix.get_connection_policy(0, 0)->paused);
    assert(matrix.connected_to_source_count(0) == 2 && matrix.connected_to_target_count(0) == 2);
    assert(matrix.outputs[0].playing_notes[60] == 1 && matrix.outputs[0].events.size() == 2);
    assert(matrix.active_connection_notes.count() == 2 && matrix.held_notes_for_connection(0, 0) == 0);
    now_ms = 20;
    matrix.processNoteOn(0, 62, 80, 1);
    matrix.processControlChange(0, 7, 88, 1);
    matrix.processPitchBend(0, 250, 1);
    matrix.processNoteOff(0, 60, 0, 1);
    assert(matrix.outputs[0].events.size() == 2);
    assert(matrix.sources[0].last_activity_ms == 20 && matrix.sources[0].has_activity);
    assert(matrix.targets[0].last_activity_ms == 10 && matrix.targets[1].last_activity_ms == 20);
    assert(matrix.find_connection(0, 0)->last_activity_ms == 10);
    matrix.processNoteOff(1, 60, 0, 1);
    assert(matrix.outputs[0].events.back().type == 'F');
    const auto before_resume = matrix.outputs[0].events.size();
    assert(matrix.set_paused(0, 0, false));
    assert(matrix.outputs[0].events.size() == before_resume);
    matrix.processNoteOff(0, 62, 0, 1);
    assert(matrix.outputs[0].events.size() == before_resume);
    matrix.processNoteOn(0, 64, 100, 1);
    assert(matrix.outputs[0].events.size() == before_resume + 1);
    auto policy = *matrix.get_connection_policy(0, 0);
    policy.fixed_channel = 5;
    policy.paused = true;
    assert(matrix.set_connection_policy(0, 0, policy));
    assert(matrix.outputs[0].events.back().type == 'F' && matrix.outputs[0].events.back().channel == 1);
    matrix.set_paused(0, 0, false);
    matrix.processControlChange(0, 10, 22, 1);
    matrix.processPitchBend(0, -500, 1);
    assert(matrix.outputs[0].events.back().channel == 5);
    assert(matrix.outputs[0].events[matrix.outputs[0].events.size() - 2].channel == 5);
    matrix.processNoteOn(0, 65, 100, 1);
    matrix.disconnect(0, 0);
    assert(matrix.outputs[0].events.back().type == 'F');
    assert(matrix.connected_to_source_count(0) == 1 && matrix.connected_to_target_count(0) == 1);
    assert(matrix.connect(0, 0) && !matrix.get_connection_policy(0, 0)->paused);
    assert(matrix.get_connection_policy(0, 0)->fixed_channel == 0);
    matrix.reset_matrix();
    assert(matrix.connections.count() == 0 && matrix.active_connection_notes.count() == 0);
    for (int i = 0; i < 4; ++i) {
        assert(matrix.connected_to_source_count(i) == 0 && matrix.connected_to_target_count(i) == 0);
        assert(!matrix.sources[i].has_activity && !matrix.targets[i].has_activity);
    }
    matrix.processNoteOn(-2, 60, 100);
    matrix.processNoteOn(50, 60, 100);
    matrix.processNoteOff(-2, 60, 0);
    matrix.processControlChange(50, 0, 0);
    matrix.processPitchBend(-2, 100);
    assert(!matrix.set_paused(0, 0, true) && !matrix.toggle_connect(-2, 0));
    assert(matrix.connected_to_source_count(-2) == 0 && matrix.connected_to_target_count(-2) == 0);
}

static void dropped_pause_tests() {
    Fixture matrix;
    assert(matrix.connect(0, 0));
    auto policy = *matrix.get_connection_policy(0, 0);
    policy.quantise_mode = MIDIMatrixManager::ConnectionQuantiseMode::FORCE_SCALE;
    transpose = -61;
    assert(matrix.set_connection_policy(0, 0, policy));
    matrix.processNoteOn(0, 60, 100, 1);
    assert(matrix.active_connection_notes.count() == 1 && matrix.held_notes_for_connection(0, 0) == 0);
    assert(matrix.outputs[0].events.empty());
    assert(matrix.set_paused(0, 0, true));
    assert(matrix.active_connection_notes.count() == 0);
    transpose = 0;
    matrix.migrate_connection_notes();
    assert(matrix.outputs[0].events.empty());
    policy = *matrix.get_connection_policy(0, 0);
    policy.quantise_mode = MIDIMatrixManager::ConnectionQuantiseMode::FORCE_OFF;
    assert(matrix.set_connection_policy(0, 0, policy));
    assert(matrix.outputs[0].events.empty());
    assert(matrix.set_paused(0, 0, false));
    assert(matrix.outputs[0].events.empty());
    matrix.processNoteOn(0, 60, 100, 1);
    assert(matrix.outputs[0].events.size() == 1);
    matrix.reset_matrix();
    assert(matrix.outputs[0].events.back().type == 'F' && matrix.active_connection_notes.count() == 0);
}

static void policy_migration_tests() {
    Fixture matrix;
    assert(matrix.connect(3, 0));
    assert(matrix.get_connection_policy(3, 0)->quantise_mode == MIDIMatrixManager::ConnectionQuantiseMode::FORCE_OFF);
    matrix.outputs[1].default_channel = 10;
    assert(matrix.connect(0, 1));
    assert(matrix.get_connection_policy(0, 1)->quantise_mode == MIDIMatrixManager::ConnectionQuantiseMode::FORCE_OFF);
    assert(matrix.connect(0, 0));
    matrix.processNoteOn(0, 60, 100, 1);
    auto policy = *matrix.get_connection_policy(0, 0);
    policy.fixed_channel = 4;
    assert(matrix.set_connection_policy(0, 0, policy));
    assert(matrix.outputs[0].events.size() == 3);
    assert(matrix.outputs[0].events[1].type == 'F' && matrix.outputs[0].events[1].channel == 1);
    assert(matrix.outputs[0].events[2].type == 'N' && matrix.outputs[0].events[2].channel == 4);
    matrix.processNoteOff(0, 60, 0, 1);
    assert(matrix.outputs[0].events.back().channel == 4 && matrix.outputs[0].events.back().type == 'F');
    policy.fixed_channel = 17;
    assert(!matrix.set_connection_policy(0, 0, policy));
    assert(matrix.get_connection_policy(0, 0)->fixed_channel == 4);
    matrix.disallow(0, 0);
    assert(!matrix.is_connected(0, 0) && !matrix.connect(0, 0));
    matrix.reset_matrix();
    assert(!matrix.connect(0, 0));
}

static std::vector<std::string> saved;
static void save_line(const char *line) { saved.emplace_back(line); }
static void save_line_ctx(const char *line, void *ctx) {
    static_cast<std::vector<std::string> *>(ctx)->emplace_back(line);
}

static void persistence_tests() {
    Fixture matrix;
    assert(matrix.load("source0", "target1", "version=1;channel=4;quantise=2;note_map=1"));
    assert(!matrix.get_connection_policy(0, 1)->paused);
    assert(matrix.load("source1", "target2", "version=2;channel=5;quantise=1;note_map=0;paused=1"));
    saved.clear();
    char prefix[] = "midi_matrix";
    matrix.save_recursive(prefix, strlen(prefix), save_line, SL_SCOPE_ROUTING);
    assert(saved.size() == 2);
    assert(saved[0] == "midi_matrix~source0~target1~connection=version=2;channel=4;quantise=2;note_map=1;paused=0");
    assert(saved[1] == "midi_matrix~source1~target2~connection=version=2;channel=5;quantise=1;note_map=0;paused=1");
    {
        Fixture restored;
        for (const auto &line : saved) {
            const auto source_start = line.find('~') + 1;
            const auto source_end = line.find('~', source_start);
            const auto target_end = line.find('~', source_end + 1);
            const auto value_start = line.find('=', target_end) + 1;
            assert(restored.load(line.substr(source_start, source_end - source_start).c_str(),
                line.substr(source_end + 1, target_end - source_end - 1).c_str(),
                line.c_str() + value_start));
        }
        assert(restored.connections.count() == 2);
        assert(restored.get_connection_policy(0, 1)->fixed_channel == 4);
        assert(!restored.get_connection_policy(0, 1)->paused);
        assert(restored.get_connection_policy(1, 2)->fixed_channel == 5);
        assert(restored.get_connection_policy(1, 2)->paused);
    }
    midi_matrix_manager = &matrix;
    std::vector<std::string> contextual;
    matrix.save_recursive(prefix, strlen(prefix), save_line_ctx, &contextual, SL_SCOPE_ROUTING);
    assert(contextual == saved);
    contextual.clear();
    matrix.print_dynamic_entries(prefix, save_line_ctx, &contextual, SL_SCOPE_ROUTING);
    assert(contextual == saved);
    saved.clear();
    matrix.save_recursive(prefix, strlen(prefix), save_line, 0);
    assert(saved.empty());
    matrix._emit_routing_lines("", 0, save_line, nullptr, nullptr);
    assert(saved[0].find("source0~target1~connection=version=2;") == 0);
    matrix.reset_matrix();
    assert(!matrix.load("source0", "target0", "version=2;paused=1", 0));
    assert(matrix.load("source0", "target0", "version=3;paused=1"));
    assert(matrix.connections.count() == 0);
    assert(matrix.load("source1", "target2", "version=2;channel=5;quantise=1;note_map=0;paused=1"));
    const auto warning_count = warnings.size();
    assert(matrix.load("source1", "target2", "version=2;channel=17"));
    assert(warnings.size() > warning_count);
    assert(matrix.get_connection_policy(1, 2)->paused && matrix.get_connection_policy(1, 2)->fixed_channel == 5);
    matrix.processNoteOn(1, 60, 100, 1);
    assert(matrix.outputs[2].events.empty() && matrix.active_connection_notes.count() == 0);
    assert(matrix.load("missing", "target0", "version=1;channel=0"));
    assert(matrix.connections.count() == 1);
    char source[] = "source3", field[] = "targets";
    char *segments[] = {source, field};
    assert(matrix.load_line(segments, 2, "target0; target1;"));
    assert(matrix.is_connected(3, 0) && matrix.is_connected(3, 1));
    assert(matrix.get_connection_policy(3, 0)->quantise_mode == MIDIMatrixManager::ConnectionQuantiseMode::FORCE_OFF);
}

static void popup_tests() {
    Fixture matrix;
    Popup popup;
    matrix.set_source_page_index(0, 7);
    popup.button_right();
    assert(popup.mode == Popup::Mode::CONTEXT_PANEL && popup.sel_target == -1);
    popup.selected_value_index = static_cast<int>(Popup::PopupRow::JUMP_SOURCE);
    popup.button_select();
    assert(menu->jumped_page == 7);
    assert(!popup.popup_row_enabled(Popup::PopupRow::CHANNEL));
    assert(!popup.popup_row_enabled(Popup::PopupRow::PAUSE_TOGGLE));
    popup.button_back();
    assert(popup.mode == Popup::Mode::SOURCE_SELECT);
    popup.mode = Popup::Mode::TARGET_SELECT;
    popup.sel_source = 0;
    popup.button_right();
    popup.selected_value_index = static_cast<int>(Popup::PopupRow::CONNECT_TOGGLE);
    popup.button_select();
    assert(matrix.is_connected(0, 0) && last_colour == GREEN);
    matrix.processNoteOn(0, 60, 100, 1);
    popup.selected_value_index = static_cast<int>(Popup::PopupRow::PAUSE_TOGGLE);
    popup.button_select();
    assert(matrix.get_connection_policy(0, 0)->paused && matrix.active_connection_notes.count() == 0);
    assert(strcmp(popup.popup_row_label(Popup::PopupRow::PAUSE_TOGGLE), "Paused: yes") == 0);
    popup.button_select();
    assert(!matrix.get_connection_policy(0, 0)->paused);
    assert(strcmp(popup.popup_row_label(Popup::PopupRow::PAUSE_TOGGLE), "Paused: no") == 0);
    popup.button_back();
    assert(popup.mode == Popup::Mode::TARGET_SELECT);
    matrix.disallow(0, 0);
    popup.toggle_connection_with_feedback(0, 0);
    assert(last_colour == RED && last_message.find("Cannot connect") == 0);
    matrix.reset_matrix();
    for (int source = 0; source < 4; ++source) {
        for (int target = 0; target < MAX_NUM_TARGETS; ++target) {
            if (matrix.targets_count <= target)
                matrix.register_target(&matrix.outputs[0], "extra");
            if (matrix.connections.count() < 128 && matrix.is_allowed(source, target))
                assert(matrix.connect(source, target));
        }
    }
    assert(matrix.connections.count() == 128);
    popup.toggle_connection_with_feedback(3, 49);
    assert(!matrix.is_connected(3, 49) && last_colour == RED);
}

static bool has_trace(const MIDINoteTrace &trace, MIDINoteTraceKind kind, int source = -2, int target = -2) {
    for (uint16_t index = 0; index < trace.count(); ++index) {
        const auto &event = trace.at(index);
        if (event.kind == kind && (source == -2 || event.source == source)
            && (target == -2 || event.target == target)) return true;
    }
    return false;
}

static bool output_contains(const char *text) {
    for (const auto &line : warnings)
        if (line.find(text) != std::string::npos) return true;
    return false;
}

static void diagnostic_ring_tests() {
    MIDINoteTrace trace;
    MIDINoteTraceEvent event;
    trace.record(event);
    assert(trace.count() == 0);
    trace.enabled = true;
    for (unsigned int index = 0; index < MIDINoteTrace::CAPACITY + 5; ++index) {
        event.time_ms = index;
        trace.record(event);
    }
    assert(trace.count() == MIDINoteTrace::CAPACITY && trace.overwritten == 5);
    assert(trace.at(0).time_ms == 5 && trace.at(trace.count() - 1).time_ms == MIDINoteTrace::CAPACITY + 4);
    trace.enabled = false;
    trace.record(event);
    assert(trace.count() == MIDINoteTrace::CAPACITY && trace.overwritten == 5);
    trace.clear();
    assert(trace.count() == 0 && trace.overwritten == 0 && !trace.enabled);
    trace.enabled = true;
    trace.source_filter = 1;
    trace.target_filter = 2;
    event.source = 0;
    event.target = 2;
    trace.record(event);
    assert(trace.count() == 0);
    event.source = 1;
    trace.record(event);
    assert(trace.count() == 1);
    event.source = event.target = -1;
    event.kind = MIDINoteTraceKind::RESET;
    trace.record(event);
    assert(trace.count() == 2);
    MIDINoteTraceContext first{&trace, event}, nested{&trace, event};
    MIDINoteTraceContext *slot = nullptr;
    {
        MIDINoteTraceScope outer(slot, &first);
        assert(slot == &first);
        {
            MIDINoteTraceScope inner(slot, &nested);
            assert(slot == &nested);
        }
        assert(slot == &first);
    }
    assert(slot == nullptr);
}

static void diagnostic_routing_tests() {
    Fixture matrix;
    matrix.connect(0, 0);
    matrix.processNoteOn(0, 50, 100, 1);
    assert(!matrix.note_trace);
    matrix.processNoteOff(0, 50, 0, 1);
    assert(matrix.enable_note_trace());
    now_ms = 100;
    auto policy = *matrix.get_connection_policy(0, 0);
    policy.quantise_mode = MIDIMatrixManager::ConnectionQuantiseMode::FORCE_SCALE;
    policy.fixed_channel = 4;
    transpose = 2;
    matrix.set_connection_policy(0, 0, policy);
    matrix.processNoteOn(0, 60, 100, 1);
    const auto &on = matrix.note_trace->at(matrix.note_trace->count() - 1);
    assert(on.kind == MIDINoteTraceKind::SEND_ON && on.source == 0 && on.target == 0);
    assert(on.input_pitch == 60 && on.input_channel == 1 && on.output_pitch == 62 && on.output_channel == 4);
    assert(on.started_ms == 100 && on.time_ms == 100 && on.refcount == 1 && on.fixed_channel == 4);
    assert(matrix.active_connection_notes.entries[0].started_ms == 100);
    // A release arriving at a different pitch is evidence, not an excuse to
    // change quantisation or guess which held note to stop.
    matrix.processNoteOff(0, 62, 0, 1);
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::UNMATCHED_OFF, 0, 0));
    assert(matrix.active_connection_notes.count() == 1);
    transpose = 3;
    now_ms = 150;
    matrix.migrate_connection_notes();
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::MIGRATE, 0, 0));
    assert(matrix.active_connection_notes.entries[0].started_ms == 100);
    now_ms = 200;
    matrix.processNoteOff(0, 60, 0, 1);
    const auto &off = matrix.note_trace->at(matrix.note_trace->count() - 1);
    assert(off.kind == MIDINoteTraceKind::SEND_OFF && off.output_pitch == 63);
    assert(off.output_channel == 4 && off.started_ms == 100 && matrix.active_connection_notes.count() == 0);
    assert(matrix.outputs[0].note_trace_context == nullptr);
    transpose = 0;
    matrix.connect(1, 0);
    policy.quantise_mode = MIDIMatrixManager::ConnectionQuantiseMode::FORCE_OFF;
    policy.fixed_channel = 0;
    matrix.set_connection_policy(0, 0, policy);
    matrix.processNoteOn(0, 60, 100, 1);
    matrix.processNoteOn(1, 60, 100, 1);
    matrix.processNoteOff(0, 60, 0, 1);
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::SUPPRESSED_OFF, 0, 0));
    matrix.processNoteOff(1, 60, 0, 1);
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::SEND_OFF, 1, 0));
    for (int index = 0; index < 9; ++index) matrix.processNoteOn(0, 64, 100, 1);
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::REFCOUNT_LIMIT));
    matrix.set_paused(0, 0, true);
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::PAUSE_RELEASE, 0, 0));
    matrix.processNoteOn(0, 64, 100, 1);
    matrix.processNoteOff(0, 64, 0, 1);
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::PAUSED_ON));
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::PAUSED_OFF));
    matrix.set_paused(0, 0, false);
    matrix.processNoteOn(0, 60, 100, 1);
    matrix.disconnect(0, 0);
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::DISCONNECT, 0, 0));
    matrix.reset_matrix();
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::RESET));
    assert(matrix.note_trace->count() > 0);
    // Switching routes/projects must preserve source-attributed release evidence.
    matrix.connect(0, 0);
    matrix.processNoteOn(0, 60, 100, 1);
    matrix.reset_matrix();
    matrix.connect(1, 0);
    matrix.processNoteOn(1, 61, 100, 1);
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::CLEANUP, 0, 0));
    assert(matrix.active_connection_notes.count() == 1);
    matrix.stop_all_notes_for_target(0, true);
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::PANIC_DISCARD, 1, 0));
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::PANIC_CHANNEL, -1, 0));
}

static void diagnostic_console_tests() {
    Fixture matrix;
    matrix.connect(0, 0);
    now_ms = UINT32_MAX - 5;
    matrix.processNoteOn(0, 60, 100, 1);
    now_ms = 10;
    warnings.clear();
    execute_midi_note_debug_command(&matrix, "held", "0 0");
    assert(output_contains("held_age_ms=16") && output_contains("in=60/ch1 out=60/ch1"));
    assert(output_contains("target_wide_refs") && output_contains("pitch=60 refs=1"));
    assert(!matrix.note_trace && matrix.active_connection_notes.count() == 1);
    const auto events_before = matrix.outputs[0].events.size();
    assert(execute_command(" notes on 0 0\r\n"));
    assert(matrix.note_trace && matrix.note_trace->enabled);
    for (const char *invalid : {"not-an-id", "50", "-2", "0 50", "0 0 extra", "0x2", "99999999999999999999999999"}) {
        execute_midi_note_debug_command(&matrix, "on", invalid);
        assert(matrix.note_trace->enabled && matrix.note_trace->source_filter == 0 && matrix.note_trace->target_filter == 0);
    }
    matrix.connect(1, 0);
    matrix.processNoteOn(1, 65, 100, 1);
    assert(!has_trace(*matrix.note_trace, MIDINoteTraceKind::PAIRED_ON, 1, 0));
    matrix.processNoteOff(0, 60, 0, 1);
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::PAIRED_OFF, 0, 0));
    execute_midi_note_debug_command(&matrix, "off", "");
    const auto saved_count = matrix.note_trace->count();
    matrix.processNoteOff(1, 65, 0, 1);
    assert(matrix.note_trace->count() == saved_count);
    execute_midi_note_debug_command(&matrix, "dump", "");
    assert(!matrix.note_trace->enabled && matrix.note_trace->count() == saved_count);
    assert(matrix.outputs[0].events.size() == events_before + 3);
    assert(output_contains("SEND means wrapper handoff attempt"));
    execute_midi_note_debug_command(&matrix, "clear", "");
    assert(matrix.note_trace->count() == 0);
    execute_midi_note_debug_command(&matrix, "on", "");
    assert(matrix.note_trace->source_filter == -1 && matrix.note_trace->target_filter == -1);
    DeviceBehaviourUltimateBase owner;
    matrix.set_source_owner(0, &owner, "ch2");
    matrix.processNoteOn(0, 67, 100, 1);
    warnings.clear();
    execute_midi_note_debug_command(&matrix, "owner", "Mock");
    assert(output_contains("in=67/ch1") && output_contains("role=ch2"));
    warnings.clear();
    execute_midi_note_debug_command(&matrix, "owner", "Missing");
    assert(output_contains("unknown or ambiguous"));
    execute_midi_note_debug_command(&matrix, "list", "");
    assert(output_contains("src=0 [source0]") && output_contains("tgt=0 [target0]"));
    assert(!matrix.enable_note_trace(50, -1));
    BehaviourDebugWrapper behaviour_wrapper;
    behaviour_wrapper.output = &owner;
    const auto state = behaviour_wrapper.get_note_debug_state();
    assert(state.behaviour && state.tuning == 2 && state.low == 24 && state.high == 100 && state.high_mode == 1);
}

static void diagnostic_target_and_overflow_tests() {
    Fixture matrix;
    matrix.enable_note_trace();
    matrix.connect(0, 0);
    DeviceBehaviourUltimateBase target;
    matrix.outputs[0].behaviour = &target;
    matrix.processNoteOn(0, 60, 100, 1);
    matrix.begin_behaviour_transform_change(&target);
    matrix.outputs[0].target_transpose = 2;
    matrix.end_behaviour_transform_change(&target);
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::TARGET_MIGRATE, 0, 0));
    const auto &resolved = matrix.note_trace->at(matrix.note_trace->count() - 2);
    assert(resolved.kind == MIDINoteTraceKind::SEND_ON && resolved.output_pitch == 62 && resolved.refcount == -1);
    assert(matrix.note_trace->at(matrix.note_trace->count() - 1).kind == MIDINoteTraceKind::MIGRATE_RESULT);
    matrix.reset_matrix();
    matrix.connect(0, 0);
    for (int index = 0; index < MIDI_CONNECTION_NOTE_CAPACITY; ++index)
        matrix.processNoteOn(0, index % 100, 100, 1);
    matrix.processNoteOn(0, 1, 100, 1);
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::LEDGER_FULL, 0, 0));
    assert(matrix.note_trace->overwritten > 0);
    assert(matrix.active_connection_notes.count() == MIDI_CONNECTION_NOTE_CAPACITY);
}

static void diagnostic_collapsed_pitch_tests() {
    Fixture matrix;
    matrix.enable_note_trace();
    collapse_scale_pairs = true;
    for (int source = 0; source < 2; ++source) {
        matrix.connect(source, 0);
        auto policy = *matrix.get_connection_policy(source, 0);
        policy.quantise_mode = MIDIMatrixManager::ConnectionQuantiseMode::FORCE_SCALE;
        matrix.set_connection_policy(source, 0, policy);
        matrix.processNoteOn(source, 60 + source, 100, 1);
    }
    assert(matrix.outputs[0].playing_notes[60] == 2);
    transpose = 2;
    matrix.migrate_connection_notes();
    assert(matrix.outputs[0].playing_notes[60] == 0 && matrix.outputs[0].playing_notes[62] == 2);
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::MIGRATE, 0, 0));
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::MIGRATE, 1, 0));
    matrix.processNoteOff(1, 61, 0, 1);
    assert(matrix.outputs[0].playing_notes[62] == 1);
    assert(has_trace(*matrix.note_trace, MIDINoteTraceKind::SUPPRESSED_OFF, 1, 0));
    matrix.processNoteOff(0, 60, 0, 1);
    assert(matrix.outputs[0].playing_notes[62] == 0 && matrix.active_connection_notes.count() == 0);
    assert(matrix.outputs[0].events.back().type == 'F' && matrix.outputs[0].events.back().pitch == 62);
    collapse_scale_pairs = false;
    transpose = 0;
}

int main() {
    store_tests();
    parser_tests();
    pause_and_routing_tests();
    dropped_pause_tests();
    policy_migration_tests();
    persistence_tests();
    popup_tests();
    diagnostic_ring_tests();
    diagnostic_routing_tests();
    diagnostic_console_tests();
    diagnostic_target_and_overflow_tests();
    diagnostic_collapsed_pitch_tests();
    std::cout << "MIDI connection tests passed: sparse capacity/order, pause/refcounts, activity, policy migration, save/load, popup\n";
    std::cout << "MIDI note diagnostic tests passed: attribution, pairing/migration, timestamps/wrap, refcounts, cleanup, bounded recorder and console\n";
}
