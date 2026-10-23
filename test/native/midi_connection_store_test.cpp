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

static void routing_model_tests() {
    Fixture matrix;
    MIDIRoutingModel model(matrix);
    MIDIPortOwner sender("sender"), receiver("receiver");
    matrix.set_source_owner(0, &sender);
    matrix.set_source_owner(1, &sender);
    matrix.set_target_owner(0, &receiver);
    matrix.set_target_owner(1, &receiver);

    // store slots are filled out of source order; inbound listing must still be sorted
    assert(model.connect(0, 2) == MIDIRouteConnectResult::CONNECTED);
    assert(model.connect(3, 0) == MIDIRouteConnectResult::CONNECTED);
    assert(model.connect(0, 0) == MIDIRouteConnectResult::CONNECTED);
    assert(model.connect(1, 0) == MIDIRouteConnectResult::CONNECTED);
    assert(model.connect(1, 0) == MIDIRouteConnectResult::ALREADY_CONNECTED);
    assert(model.connect(-1, 0) == MIDIRouteConnectResult::INVALID);
    matrix.disallow(2, 3);
    assert(model.connect(2, 3) == MIDIRouteConnectResult::DISALLOWED);
    assert(strstr(MIDIRoutingModel::connect_result_message(MIDIRouteConnectResult::FULL), "full") != nullptr);

    MIDIRouteRow rows[8];
    assert(model.list_from_source(0, rows, 8) == 2 && rows[0].target == 0 && rows[1].target == 2);
    assert(model.list_to_target(0, rows, 8) == 3);
    assert(rows[0].source == 0 && rows[1].source == 1 && rows[2].source == 3);
    assert(rows[2].policy.quantise_mode == MIDIConnectionQuantiseMode::FORCE_OFF);
    assert(model.list_to_target(0, rows, 2) == 2 && rows[0].source == 0 && rows[1].source == 1);
    assert(model.list_to_target(2, rows, 0) == 0);
    assert(model.list_outbound(&sender, rows, 8) == 3);
    assert(rows[0].source == 0 && rows[0].target == 0 && rows[1].target == 2 && rows[2].source == 1);
    assert(model.list_outbound(&sender, rows, 2) == 2);
    assert(model.list_inbound(&receiver, rows, 8) == 3 && rows[2].source == 3);
    assert(model.list_inbound(nullptr, rows, 8) == 0);

    now_ms = 500;
    matrix.processNoteOn(0, 60, 100, 1);
    MIDIRouteRow row;
    assert(model.get_row(0, 0, row) && row.held_notes == 1 && row.has_activity && row.last_activity_ms == 500);
    assert(!model.get_row(2, 0, row));
    MIDIRouteSummary summary = model.summarise_outbound(&sender);
    assert(summary.total == 3 && summary.paused == 0 && summary.held_notes == 2);
    assert(summary.can_pause_all() && !summary.can_resume_all());

    // bulk pause releases held notes, leaves other routes alone, and is idempotent
    assert(model.set_paused_from_source(0, true) == 2);
    assert(matrix.held_notes_for_connection(0, 0) == 0 && matrix.held_notes_for_connection(0, 2) == 0);
    assert(model.set_paused_from_source(0, true) == 0);
    summary = model.summarise_from_source(0);
    assert(summary.paused == 2 && !summary.can_pause_all() && summary.can_resume_all());
    assert(!matrix.get_connection_policy(1, 0)->paused && !matrix.get_connection_policy(3, 0)->paused);
    assert(model.set_paused_to_target(0, false) == 1);
    assert(!matrix.get_connection_policy(0, 0)->paused && matrix.get_connection_policy(0, 2)->paused);
    assert(model.set_paused_outbound(&sender, true) == 2);
    assert(matrix.get_connection_policy(1, 0)->paused && !matrix.get_connection_policy(3, 0)->paused);
    assert(model.set_paused_inbound(&receiver, false) == 2);
    assert(matrix.get_connection_policy(0, 2)->paused);
    assert(model.set_paused_from_source(2, true) == 0 && model.set_paused_to_target(-1, true) == 0);
    assert(matrix.connections.count() == 4);

    // single-route policy edits
    assert(model.toggle_pause(0, 0) && matrix.get_connection_policy(0, 0)->paused);
    assert(model.toggle_pause(0, 0) && !matrix.get_connection_policy(0, 0)->paused);
    assert(!model.toggle_pause(2, 0));
    assert(model.cycle_channel(0, 0) && matrix.get_connection_policy(0, 0)->fixed_channel == 1);
    assert(model.cycle_channel(0, 0, -1) && model.cycle_channel(0, 0, -1));
    assert(matrix.get_connection_policy(0, 0)->fixed_channel == 16);
    assert(model.cycle_channel(0, 0) && matrix.get_connection_policy(0, 0)->fixed_channel == 0);
    assert(model.cycle_quantise(0, 0) && matrix.get_connection_policy(0, 0)->quantise_mode == MIDIConnectionQuantiseMode::FORCE_SCALE);
    assert(model.cycle_quantise(0, 0, -1) && model.cycle_quantise(0, 0, -1));
    assert(matrix.get_connection_policy(0, 0)->quantise_mode == MIDIConnectionQuantiseMode::FORCE_OFF);
    assert(MIDIRoutingModel::next_quantise_mode(MIDIConnectionQuantiseMode::FORCE_OFF, -1) == MIDIConnectionQuantiseMode::FORCE_CHORD);
    assert(MIDIRoutingModel::next_quantise_mode(MIDIConnectionQuantiseMode::FORCE_CHORD) == MIDIConnectionQuantiseMode::FORCE_OFF);
    assert(!model.cycle_channel(2, 0) && !model.cycle_quantise(2, 0));

    char badge[24];
    MIDIConnectionPolicy policy;
    MIDIRoutingModel::format_policy_badge(badge, sizeof(badge), policy);
    assert(badge[0] == '\0');
    policy.paused = true;
    policy.fixed_channel = 3;
    policy.quantise_mode = MIDIConnectionQuantiseMode::FORCE_SCALE;
    MIDIRoutingModel::format_policy_badge(badge, sizeof(badge), policy);
    assert(strcmp(badge, "|| c3 Qs") == 0);
    MIDIRoutingModel::format_policy_badge(badge, 4, policy);
    assert(strlen(badge) == 3);
    assert(strcmp(MIDIRoutingModel::quantise_mode_label(MIDIConnectionQuantiseMode::INHERIT_BEHAVIOUR), "inherit") == 0);

    // add-candidates exclude disallowed pairs and flag existing routes
    MIDIRouteCandidate candidates[8];
    assert(model.target_candidates_for_source(2, candidates, 8) == 3);
    assert(candidates[2].id == 2 && !candidates[0].connected);
    assert(model.target_candidates_for_source(0, candidates, 8) == 4);
    assert(candidates[0].connected && !candidates[1].connected && candidates[2].connected && candidates[2].paused);
    assert(model.source_candidates_for_target(3, candidates, 8) == 3 && candidates[2].id == 3);
    assert(model.target_candidates_for_source(0, candidates, 2) == 2);

    matrix.set_target_page_index(2, 9);
    assert(model.target_page(2) == 9 && model.source_page(0) == -1);

    // same-owner routes: refused for a device/behaviour owner, allowed for port groupings
    DeviceBehaviourUltimateBase device;
    matrix.set_source_owner(2, &device);
    matrix.set_target_owner(3, &device);
    matrix.set_target_owner(2, &device);
    assert(model.connect(2, 2) == MIDIRouteConnectResult::DISALLOWED && !matrix.connect(2, 2));
    assert(model.target_candidates_for_source(2, candidates, 8) == 2);
    assert(candidates[0].id == 0 && candidates[1].id == 1);
    assert(model.connect(1, 3) == MIDIRouteConnectResult::CONNECTED);
    assert(model.connect(0, 1) == MIDIRouteConnectResult::CONNECTED);   // sender -> receiver
    MIDIPortOwner port("MIDI Port 1");
    matrix.set_source_owner(3, &port);
    matrix.set_target_owner(1, &port);
    assert(model.connect(3, 1) == MIDIRouteConnectResult::CONNECTED);
    assert(model.disconnect(1, 3) && model.disconnect(0, 1) && model.disconnect(3, 1));
    matrix.set_source_owner(2, nullptr);
    matrix.set_source_owner(3, nullptr);
    matrix.set_target_owner(2, nullptr);
    matrix.set_target_owner(3, nullptr);
    matrix.set_target_owner(1, &receiver);

    assert(model.disconnect_others_to_target(0, 1) == 2);
    assert(!matrix.is_connected(0, 0) && matrix.is_connected(1, 0) && !matrix.is_connected(3, 0));
    assert(model.disconnect_others_from_source(0) == 1 && matrix.connected_to_source_count(0) == 0);
    assert(!model.disconnect(0, 0));
    assert(matrix.connected_to_target_count(0) == 1);
}

static void popup_bulk_pause_tests() {
    Fixture matrix;
    Popup popup;
    assert(matrix.connect(0, 0) && matrix.connect(0, 1) && matrix.connect(1, 1));
    popup.sel_source = 0;
    popup.button_right();
    assert(popup.sel_target == -1);
    assert(popup.popup_row_enabled(Popup::PopupRow::PAUSE_ALL_FROM_SOURCE));
    assert(!popup.popup_row_enabled(Popup::PopupRow::RESUME_ALL_FROM_SOURCE));
    assert(!popup.popup_row_enabled(Popup::PopupRow::PAUSE_ALL_TO_TARGET));
    assert(strcmp(popup.popup_row_label(Popup::PopupRow::PAUSE_ALL_FROM_SOURCE), "Pause all from source (2)") == 0);
    assert(strcmp(popup.popup_row_label(Popup::PopupRow::PAUSE_ALL_TO_TARGET), "Pause all to target") == 0);
    popup.selected_value_index = static_cast<int>(Popup::PopupRow::PAUSE_ALL_FROM_SOURCE);
    popup.button_select();
    assert(last_message == "Paused 2 routes" && last_colour == GREEN);
    assert(matrix.get_connection_policy(0, 0)->paused && matrix.get_connection_policy(0, 1)->paused);
    assert(!matrix.get_connection_policy(1, 1)->paused);
    assert(!popup.popup_row_enabled(Popup::PopupRow::PAUSE_ALL_FROM_SOURCE));
    assert(strcmp(popup.popup_row_label(Popup::PopupRow::RESUME_ALL_FROM_SOURCE), "Resume all from source (2)") == 0);

    popup.button_back();
    popup.mode = Popup::Mode::TARGET_SELECT;
    popup.sel_source = 0;
    popup.selected_value_index = 1;
    popup.button_right();
    assert(popup.sel_target == 1);
    assert(popup.popup_row_enabled(Popup::PopupRow::PAUSE_ALL_TO_TARGET));
    assert(popup.popup_row_enabled(Popup::PopupRow::RESUME_ALL_TO_TARGET));
    popup.selected_value_index = static_cast<int>(Popup::PopupRow::RESUME_ALL_TO_TARGET);
    popup.button_select();
    assert(last_message == "Resumed 1 route");
    assert(!matrix.get_connection_policy(0, 1)->paused && matrix.get_connection_policy(0, 0)->paused);
    popup.selected_value_index = static_cast<int>(Popup::PopupRow::DISCONNECT_OTHER_SOURCES_TO_TARGET);
    popup.button_select();
    assert(matrix.is_connected(0, 1) && !matrix.is_connected(1, 1));
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

struct RoutingTestOwner : MIDIEndpointOwner {
    const char *label;
    bool online = true;
    explicit RoutingTestOwner(const char *label) : label(label) {}
    const char *get_endpoint_owner_label() override { return label; }
    bool is_endpoint_owner_online() override { return online; }
};

static std::string row_text(ActionListSource &source, uint8_t index) {
    char text[41];
    source.row_text(index, text, sizeof(text));
    return text;
}

static int find_entry(MIDIOwnerRoutesSource &routes, MIDIOwnerRoutesSource::entry_t::Kind kind,
        MIDIRouteDirection direction, int endpoint = -1, int far = -1) {
    for (uint8_t index = 0; index < routes.count(); ++index) {
        const MIDIOwnerRoutesSource::entry_t &e = routes.entry(index);
        if (e.kind == kind && e.direction == direction && (endpoint < 0 || e.endpoint == endpoint) && (far < 0 || e.far == far))
            return index;
    }
    return -1;
}

static void move_to(ActionListNavigator &nav, int index) {
    assert(index >= 0);
    for (int step = 0; step < 64 && nav.cursor() != index; ++step) nav.move(1);
    assert(nav.cursor() == index);
}

static bool ends_with(const std::string &text, const std::string &suffix) {
    return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static void routing_lists_tests() {
    typedef MIDIOwnerRoutesSource::entry_t E;
    const MIDIRouteDirection IN = MIDIRouteDirection::INBOUND, OUT = MIDIRouteDirection::OUTBOUND;
    Fixture matrix;
    static const char *target_labels[] = {"target0", "target1", "target2", "target3"};
    for (int index = 0; index < 4; ++index) matrix.outputs[index].label = target_labels[index];
    RoutingTestOwner synth("synth"), other("other"), nothing("nothing");
    matrix.set_target_owner(0, &synth, "Voice A");
    matrix.set_target_owner(1, &synth, "Voice B");
    matrix.set_source_owner(0, &synth);
    matrix.set_source_owner(1, &other);
    assert(matrix.connect(1, 0) && matrix.connect(2, 0) && matrix.connect(1, 1) && matrix.connect(0, 2));

    // layout: per-target groups when there are several targets, a single source needs no group heading
    MIDIOwnerRoutesSource routes(&matrix, &synth);
    const E::Kind expected[] = {E::SECTION, E::ENDPOINT, E::ROUTE, E::ROUTE, E::ADD, E::ENDPOINT, E::ROUTE, E::ADD,
        E::PAUSE_ALL, E::SECTION, E::ROUTE, E::ADD, E::PAUSE_ALL};
    assert(routes.count() == sizeof(expected) / sizeof(expected[0]));
    for (uint8_t index = 0; index < routes.count(); ++index) assert(routes.entry(index).kind == expected[index]);
    assert(routes.entry(2).far == 1 && routes.entry(3).far == 2 && routes.entry(6).endpoint == 1);
    assert(routes.entry(10).direction == OUT && routes.entry(10).far == 2);
    assert(strcmp(routes.title(), "synth routing") == 0);
    assert(row_text(routes, 0) == "Inputs <- 3");
    assert(row_text(routes, 1) == " Voice A:");
    assert(row_text(routes, 2).compare(0, 10, "   source1") == 0 && ends_with(row_text(routes, 2), "source1"));
    assert(row_text(routes, 4) == "  + Add input...");
    assert(row_text(routes, 8) == " Pause all inputs (3)");
    assert(row_text(routes, 9) == "Outputs -> 1" && row_text(routes, 11) == "  + Add output...");
    assert(!routes.row_selectable(0) && !routes.row_selectable(1) && routes.row_selectable(2) && routes.row_selectable(4));
    assert(routes.row_colour(0) == 0 && routes.row_colour(2) == 0xFFFF);

    // badges, held notes, activity and paused dimming come from the live matrix
    MIDIConnectionPolicy policy = *matrix.get_connection_policy(2, 0);
    policy.paused = true;
    policy.fixed_channel = 3;
    assert(matrix.set_connection_policy(2, 0, policy));
    assert(ends_with(row_text(routes, 3), " || c3") && routes.row_dimmed(3) && !routes.row_dimmed(2));
    assert(row_text(routes, 0) == "Inputs <- 2 live, 1 paused");
    const uint32_t quiet = routes.generation();
    now_ms = 10000;
    matrix.processNoteOn(1, 60, 100, 1);
    assert(routes.generation() != quiet);
    assert(row_text(routes, 2).compare(0, 3, "  *") == 0 && ends_with(row_text(routes, 2), " ~1"));
    now_ms += MIDI_ROUTE_ACTIVITY_MS;
    assert(row_text(routes, 2).compare(0, 3, "   ") == 0);
    matrix.processNoteOff(1, 60, 0, 1);
    assert(!ends_with(row_text(routes, 2), "~1"));
    now_ms += MIDI_ROUTE_ACTIVITY_MS;

    // compact summaries
    char line[41];
    midi_routing_format_owner_summary(matrix, &synth, IN, line, sizeof(line));
    assert(strcmp(line, "<- source1, ||source2, source1") == 0);
    midi_routing_format_owner_summary(matrix, &synth, OUT, line, sizeof(line));
    assert(strcmp(line, "-> target2") == 0);
    midi_routing_format_owner_summary(matrix, &synth, IN, line, 16);
    assert(strcmp(line, "<- source1 +2") == 0);
    midi_routing_format_owner_summary(matrix, &synth, IN, line, 8);
    assert(strcmp(line, "<- +3") == 0);
    midi_routing_format_owner_summary(matrix, &nothing, IN, line, sizeof(line));
    assert(strcmp(line, "<- (none)") == 0);

    // route editor: channel/quantise cycling both ways, pause, jump, disconnect pops back
    ActionListNavigator nav;
    nav.reset(&routes);
    assert(nav.cursor() == 2);
    assert(!nav.select() && nav.get_depth() == 2);
    MIDIRouteEditSource &editor = MIDIOwnerRoutesSource::editor();
    assert(nav.current() == &editor && nav.cursor() == MIDIRouteEditSource::PAUSE);
    assert(strcmp(editor.title(), "source1 -> target0") == 0);
    assert(row_text(editor, MIDIRouteEditSource::STATUS) == "Live, 0 held");
    assert(row_text(editor, MIDIRouteEditSource::CHANNEL) == "Channel: default");
    assert(!editor.row_enabled(MIDIRouteEditSource::JUMP));
    move_to(nav, MIDIRouteEditSource::CHANNEL);
    nav.select();
    assert(matrix.get_connection_policy(1, 0)->fixed_channel == 1 && row_text(editor, MIDIRouteEditSource::CHANNEL) == "Channel: 1");
    assert(nav.right() && matrix.get_connection_policy(1, 0)->fixed_channel == 0);
    move_to(nav, MIDIRouteEditSource::QUANTISE);
    assert(row_text(editor, MIDIRouteEditSource::QUANTISE) == "Quantise: inherit");
    nav.select();
    assert(row_text(editor, MIDIRouteEditSource::QUANTISE) == "Quantise: scale");
    nav.right();
    assert(matrix.get_connection_policy(1, 0)->quantise_mode == MIDIConnectionQuantiseMode::INHERIT_BEHAVIOUR);
    move_to(nav, MIDIRouteEditSource::PAUSE);
    nav.select();
    assert(matrix.get_connection_policy(1, 0)->paused && row_text(editor, MIDIRouteEditSource::PAUSE) == "Resume");
    assert(row_text(editor, MIDIRouteEditSource::STATUS) == "Paused, 0 held");
    nav.select();
    assert(!matrix.get_connection_policy(1, 0)->paused);
    matrix.set_source_page_index(1, 7);
    assert(editor.row_enabled(MIDIRouteEditSource::JUMP) && row_text(editor, MIDIRouteEditSource::JUMP) == "Go to source1");
    move_to(nav, MIDIRouteEditSource::JUMP);
    routing_ui_jump = -1;
    nav.select();
    assert(routing_ui_jump == 7 && nav.get_depth() == 2);
    move_to(nav, MIDIRouteEditSource::DISCONNECT);
    assert(!nav.select());
    assert(!matrix.is_connected(1, 0) && routing_ui_message == "Disconnected" && !routing_ui_error);
    assert(nav.get_depth() == 1 && nav.current() == &routes);
    assert(routes.count() == 12 && routes.row_selectable((uint8_t)nav.cursor()));
    assert(!editor.row_enabled(MIDIRouteEditSource::PAUSE) && row_text(editor, MIDIRouteEditSource::STATUS) == "Disconnected");

    // right on a route jumps to the far end's page, or reports that there is none
    const int route_from_2 = find_entry(routes, E::ROUTE, IN, 0, 2);
    move_to(nav, route_from_2);
    routing_ui_jump = -1;
    assert(!nav.right() && routing_ui_jump == -1 && routing_ui_error);
    matrix.set_source_page_index(2, 4);
    assert(nav.right() && routing_ui_jump == 4);

    // picker: same-owner and disallowed ends are hidden, ticks toggle routes
    move_to(nav, find_entry(routes, E::ADD, IN, 1));
    nav.select();
    MIDIRoutePickerSource &picker = MIDIOwnerRoutesSource::picker();
    assert(nav.current() == &picker && strcmp(picker.title(), "Inputs to Voice B") == 0);
    assert(picker.count() == 3);
    assert(row_text(picker, 0) == "[x] source1" && row_text(picker, 1) == "[ ] source2" && row_text(picker, 2) == "[ ] source3");
    nav.move(1);
    nav.select();
    assert(matrix.is_connected(2, 1) && routing_ui_message == "Connected" && !routing_ui_error);
    assert(row_text(picker, 1) == "[x] source2");
    nav.select();
    assert(!matrix.is_connected(2, 1) && routing_ui_message == "Disconnected");
    assert(nav.back() && nav.current() == &routes);
    move_to(nav, find_entry(routes, E::ADD, OUT, 0));
    nav.select();
    assert(strcmp(picker.title(), "Outputs from source0") == 0 && picker.count() == 2);
    assert(row_text(picker, 0) == "[x] target2" && row_text(picker, 1) == "[ ] target3");
    nav.back();

    // pause all / resume all per section, leaving the other direction alone
    const int pause_inputs = find_entry(routes, E::PAUSE_ALL, IN);
    move_to(nav, pause_inputs);
    assert(row_text(routes, (uint8_t)pause_inputs) == " Pause all inputs (1)");
    nav.select();
    assert(routing_ui_message == "Paused 1 route");
    assert(matrix.get_connection_policy(1, 1)->paused && matrix.get_connection_policy(2, 0)->paused);
    assert(!matrix.get_connection_policy(0, 2)->paused);
    assert(row_text(routes, (uint8_t)pause_inputs) == " Resume all inputs (2)");
    nav.select();
    assert(routing_ui_message == "Resumed 2 routes" && !matrix.get_connection_policy(2, 0)->paused);

    // an owner without endpoints, offline owners, and the shared row buffer switching owners
    MIDIOwnerRoutesSource empty(&matrix, &nothing);
    assert(empty.count() == 1 && row_text(empty, 0) == "No MIDI routing" && !empty.row_selectable(0));
    nav.reset(&empty);
    assert(nav.cursor() == ActionListNavigator::NO_ROW && !nav.select());
    assert(routes.count() == 12 && routes.entry(0).kind == E::SECTION);
    other.online = false;
    assert(routes.row_dimmed((uint8_t)find_entry(routes, E::ROUTE, IN, 1, 1)));
    MIDIOwnerRoutesSource other_routes(&matrix, &other);
    assert(strcmp(other_routes.title(), "other routing (offline)") == 0);
    assert(other_routes.count() == 4 && other_routes.entry(1).kind == E::ROUTE && other_routes.entry(1).far == 1);
    assert(routes.count() == 12);

    // ports occupied by behaviours list them first; selecting or [>] jumps to the behaviour's page
    MIDIPortOwner port("MIDI Port 2");
    RoutingTestOwner neutron("Neutron");
    assert(port.add_linked_owner("Out", &neutron) && port.add_linked_owner("In", &other));
    assert(!port.add_linked_owner("In+Out", &synth) && port.get_linked_owner_count() == 2);
    midi_routing_format_link(port.get_linked_owner(0), line, sizeof(line));
    assert(strcmp(line, "Out used by Neutron") == 0);
    MIDIOwnerRoutesSource port_routes(&matrix, &port);
    assert(port_routes.count() == 2 && port_routes.entry(0).kind == E::LINK && port_routes.entry(1).kind == E::LINK);
    assert(row_text(port_routes, 1).find("In used by other") != std::string::npos);
    assert(port_routes.row_selectable(0) && port_routes.row_dimmed(1) && !port_routes.row_dimmed(0));
    nav.reset(&port_routes);
    assert(nav.cursor() == 0);
    routing_ui_jump = -1;
    routing_ui_error = false;
    assert(!nav.select() && routing_ui_jump == -1 && routing_ui_error);
    neutron.routing_page_index = 9;
    nav.select();
    assert(routing_ui_jump == 9 && nav.get_depth() == 1);
    other.routing_page_index = 5;
    move_to(nav, 1);
    assert(nav.right() && routing_ui_jump == 5);
    // links come before the port's own routes
    matrix.set_target_owner(3, &port, "Out");
    MIDIOwnerRoutesSource port_with_routes(&matrix, &port);
    assert(port_with_routes.count() > 2 && port_with_routes.entry(0).kind == E::LINK && port_with_routes.entry(2).kind == E::SECTION);
}

int main() {
    store_tests();
    parser_tests();
    pause_and_routing_tests();
    dropped_pause_tests();
    policy_migration_tests();
    persistence_tests();
    popup_tests();
    routing_model_tests();
    popup_bulk_pause_tests();
    routing_lists_tests();
    diagnostic_ring_tests();
    diagnostic_routing_tests();
    diagnostic_console_tests();
    diagnostic_target_and_overflow_tests();
    diagnostic_collapsed_pitch_tests();
    std::cout << "MIDI connection tests passed: sparse capacity/order, pause/refcounts, activity, policy migration, save/load, popup\n";
    std::cout << "MIDI routing model tests passed: listings, summaries, bulk pause, policy cycling, candidates, popup bulk actions\n";
    std::cout << "MIDI routing panel list tests passed: owner layout, badges/held/activity, compact summaries, route editor, picker, section pause, shared buffer\n";
    std::cout << "MIDI note diagnostic tests passed: attribution, pairing/migration, timestamps/wrap, refcounts, cleanup, bounded recorder and console\n";
}
