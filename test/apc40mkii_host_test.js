const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const root = path.resolve(__dirname, '..');
const libraries = path.resolve(root, '../my_libraries');
const apcLibrary = process.env.APC40MKII_DIR || path.join(libraries, 'apc40mkii');
const parameters = process.env.PARAMETERS_DIR || path.join(libraries, 'parameters');
const foundling = process.env.FOUNDLING_DIR || path.join(libraries, 'foundling');
function withoutIncludes(filename) {
    return fs.readFileSync(filename, 'utf8').replace(/^\s*#(?:include .*|pragma once)\r?$/gm, '');
}
const parameterInput = withoutIncludes(path.join(parameters, 'include/parameter_inputs/ParameterInput.h'));
const midiInput = withoutIncludes(path.join(parameters, 'include/parameter_inputs/MIDIParameterInput.h'));
const midiSource = withoutIncludes(path.join(parameters, 'include/midi/midi_cc_source.h'));
const learnMethods = fs.readFileSync(path.join(parameters, 'include/ParameterManager.h'), 'utf8')
    .match(/^\s*#ifdef ENABLE_PARAMETERS_MIDI_LEARN\r?\n([\s\S]*?)^\s*#endif/m)[1];
const learnControl = fs.readFileSync(path.join(parameters, 'include/mymenu_items/ParameterMenuItems.h'), 'utf8')
    .match(/class ParameterModSlotMidiLearnControl[\s\S]*?\n    };/)[0];
const modulationRow = fs.readFileSync(path.join(parameters, 'include/mymenu_items/ParameterMenuItems.h'), 'utf8')
    .match(/class ParameterModSlotRow[\s\S]*?\n};/)[0];
const lifecycleOnly = process.argv.includes('--learn-lifecycle-only');
const learnEnabled = lifecycleOnly || process.argv.includes('--learn');
const apcInputs = withoutIncludes(path.join(apcLibrary, 'src/APC40ParameterInputs.h'));
const behaviour = withoutIncludes(path.join(root, 'include/behaviours/behaviour_apc40mkii.h'));
const transport = withoutIncludes(path.join(root, 'src/behaviours/behaviour_apc40mkii.cpp'));
const source = `
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>
#include "APC40Controller.h"
#include "transport_state.h"
using byte = uint8_t;
#define ENABLE_APC40MKII
#define APC40_ENABLE_PARAMETERS 1
#define FLASHMEM
#define F(text) text
#define Debug_printf(...) ((void)0)
template<typename Value> Value constrain(Value value, Value low, Value high) {
    return std::max(low, std::min(value, high));
}
enum VALUE_TYPE { UNIPOLAR, BIPOLAR };
template<typename Value> struct LinkedList : std::vector<Value> {
    void add(Value value) { this->push_back(value); }
};
${parameterInput}
${midiInput}
struct FloatParameter {
    struct Connection {
        BaseParameterInput *parameter_input = nullptr;
        bool midi_learn_active = false;
        float amount = 0.37f;
    } connections[3];
    unsigned assignments = 0;
    bool is_valid_slot(int slot) const { return slot >= 0 && slot < 3; }
    void set_slot_input(int slot, BaseParameterInput *input) {
        assert(is_valid_slot(slot));
        connections[slot].parameter_input = input;
        ++assignments;
    }
    void set_slot_0_input(BaseParameterInput *input) { set_slot_input(0, input); }
    void set_slot_1_input(BaseParameterInput *input) { set_slot_input(1, input); }
    void set_slot_2_input(BaseParameterInput *input) { set_slot_input(2, input); }
    BaseParameterInput *get_slot_0_input() { return connections[0].parameter_input; }
    BaseParameterInput *get_slot_1_input() { return connections[1].parameter_input; }
    BaseParameterInput *get_slot_2_input() { return connections[2].parameter_input; }
};
struct ParameterManager {
    std::vector<void *> inputs;
    std::vector<BaseParameterInput *> available;
    std::vector<BaseParameterInput *> *available_inputs = &available;
    template<typename Input> void addInput(Input *input) { inputs.push_back(input); }
    #ifdef ENABLE_PARAMETERS_MIDI_LEARN
        ${learnMethods}
    #endif
} manager;
ParameterManager *parameter_manager = &manager;
${midiSource}
${apcInputs}
struct DisplayMock {
    std::string text;
    void setTextSize(int) {}
    void print(const char *value) { text = value; }
    int getCursorY() { return 0; }
} display;
constexpr uint16_t C_WHITE = 0xffff;
constexpr uint16_t BLACK = 0;
constexpr int REDRAW_ON_CUSTOM = 1;
struct MenuItem {
    struct { bool go_back_on_select = false; } flags;
    uint16_t default_fg = C_WHITE;
    DisplayMock *tft = &display;
    explicit MenuItem(const char *) {}
    virtual ~MenuItem() = default;
    void colours(bool, uint16_t, uint16_t) {}
    void add_redraw_policy(int) {}
    virtual bool check_needs_redraw_custom(bool, bool) { return false; }
    virtual int renderValue(bool, bool, uint16_t) { return 0; }
    virtual bool action_opened() { return false; }
    virtual bool wants_fullscreen_overlay_when_opened_in_bar() { return false; }
};
#ifdef ENABLE_PARAMETERS_MIDI_LEARN
${learnControl}
void test_learn_lifecycle() {
    FloatParameter first, second;
    MIDICCParameterInput input((char *)"Test", "Test", 7, 1);
    assert(!manager.is_midi_learn_active());
    assert(!manager.propose_midi_learn(&input));
    assert(!manager.start_midi_learn(nullptr, 0));
    assert(!manager.start_midi_learn(&first, 3));
    assert(!manager.start_midi_learn(&first, 255));
    assert(manager.start_midi_learn(&first, 1));
    const auto revision = manager.get_midi_learn_revision();
    assert(manager.start_midi_learn(&first, 1));
    assert(manager.get_midi_learn_revision() == revision);
    assert(!manager.start_midi_learn(nullptr, 0) && first.connections[1].midi_learn_active);
    assert(manager.start_midi_learn(&second, 2));
    assert(!first.connections[1].midi_learn_active && second.connections[2].midi_learn_active);
    manager.stop_midi_learn(&first, 1);
    assert(manager.is_midi_learn_active());
    assert(!manager.propose_midi_learn(nullptr));
    assert(manager.propose_midi_learn(&input));
    assert(second.connections[2].parameter_input == &input && second.assignments == 1);
    assert(second.connections[2].amount == 0.37f && !second.connections[2].midi_learn_active);
    assert(!manager.is_midi_learn_active() && !manager.propose_midi_learn(&input));
    FloatParameter *target = &first;
    ParameterModSlotMidiLearnControl control("Learn", &target, 0);
    assert(!control.wants_fullscreen_overlay_when_opened_in_bar());
    control.renderValue(false, false, 20);
    assert(display.text == "L");
    assert(!control.action_opened() && manager.is_midi_learn_active());
    assert(control.check_needs_redraw_custom(false, false));
    control.renderValue(false, false, 20);
    assert(display.text == "?" && !control.check_needs_redraw_custom(false, false));
    assert(!control.action_opened() && !manager.is_midi_learn_active());
    assert(control.check_needs_redraw_custom(false, false));
    target = nullptr;
    assert(!control.action_opened() && !manager.is_midi_learn_active());
    target = &first;
    ParameterModSlotMidiLearnControl invalid("Learn", &target, 3);
    invalid.renderValue(false, false, 20);
    assert(!invalid.action_opened() && !manager.is_midi_learn_active());
}
#endif
struct SubMenuItemBarCustomProportions : MenuItem {
    std::vector<float> proportions;
    std::vector<MenuItem *> items;
    SubMenuItemBarCustomProportions(const char *label, int columns, bool, bool)
        : MenuItem(label), proportions(columns) {}
    void set_column_proportion(unsigned column, float proportion) { proportions.at(column) = proportion; }
    void add(MenuItem *item) { items.push_back(item); }
    virtual int small_display(int, int, int, int, bool, bool, bool) { return 0; }
};
struct RowControlMock : MenuItem {
    template<typename... Args> RowControlMock(const char *label, Args...) : MenuItem(label) {}
};
template<typename Parameter> using ParameterInputSelectorControl = RowControlMock;
using ParameterMapPercentageControl = RowControlMock;
using ParameterConnectionPolarityTypeSelectorControl = RowControlMock;
using ParameterModSlotSHControl = RowControlMock;
${modulationRow}
void test_modulation_row_layout() {
    FloatParameter parameter;
    FloatParameter *target = &parameter;
    ParameterModSlotRow row("Slot", &target, 0);
    #ifdef ENABLE_PARAMETERS_MIDI_LEARN
        assert(row.items.size() == 5 && row.proportions.size() == 5);
        assert(dynamic_cast<ParameterModSlotMidiLearnControl *>(row.items[3]));
    #else
        assert(row.items.size() == 4 && row.proportions.size() == 4);
    #endif
    float total = 0;
    for (float proportion : row.proportions) {
        assert(proportion > 0);
        total += proportion;
    }
    assert(std::fabs(total - 1.0f) < 0.0001f);
}
#ifndef APC40_TEST_LIFECYCLE_ONLY
struct MIDIDeviceBase {
    std::vector<std::vector<uint8_t>> sent;
    void sendSysEx(std::size_t size, const uint8_t *data, bool) {
        sent.emplace_back(data, data + size);
    }
    void sendNoteOn(uint8_t note, uint8_t value, uint8_t channel) {
        sent.push_back({static_cast<uint8_t>(0x90 | (channel - 1)), note, value});
    }
    void sendNoteOff(uint8_t note, uint8_t value, uint8_t channel) {
        sent.push_back({static_cast<uint8_t>(0x80 | (channel - 1)), note, value});
    }
    void sendControlChange(uint8_t number, uint8_t value, uint8_t channel) {
        sent.push_back({static_cast<uint8_t>(0xB0 | (channel - 1)), number, value});
    }
    void setHandleControlChange(void (*)(uint8_t, uint8_t, uint8_t)) {}
    void setHandleNoteOn(void (*)(uint8_t, uint8_t, uint8_t)) {}
    void setHandleNoteOff(void (*)(uint8_t, uint8_t, uint8_t)) {}
};
struct DeviceBehaviourUSBBase {
    MIDIDeviceBase *device = nullptr;
    virtual ~DeviceBehaviourUSBBase() = default;
    virtual uint32_t get_packed_id() { return 0; }
    virtual const char *get_label() { return "USB"; }
    virtual void init() {}
    virtual void loop(uint32_t) {}
    virtual void setup_callbacks() {}
    virtual void disconnect_device() { device = nullptr; }
    virtual void receive_note_on(byte, byte, byte) {}
    virtual void receive_note_off(byte, byte, byte) {}
    virtual void receive_control_change(byte, byte, byte) {}
    bool is_connected() { return device != nullptr; }
};
struct SerialMock {
    explicit operator bool() const { return true; }
    template<typename... Args> void printf(const char *, Args...) {}
} Serial;
static float bpm = 120.0f;
static uint32_t now = 0;
[[maybe_unused]] static uint32_t millis() { return now; }
static float get_bpm() { return bpm; }
static void set_bpm(float value) { bpm = value; }
TransportState transport_state;
static bool playing = false;
static unsigned stop_calls = 0;
static unsigned continue_calls = 0;
static uint32_t song_position = 384;
static void clock_stop() { ++stop_calls; playing = false; }
static void clock_continue() { ++continue_calls; playing = true; }
${behaviour}
${transport}
static bool close_to(float actual, float expected) {
    return std::fabs(actual - expected) < 0.0001f;
}
static bool sent_cc(const MIDIDeviceBase &device, uint8_t number, uint8_t value, uint8_t channel = 1) {
    const std::vector<uint8_t> expected = {static_cast<uint8_t>(0xB0 | (channel - 1)), number, value};
    return std::find(device.sent.begin(), device.sent.end(), expected) != device.sent.end();
}
static bool sent_button(const MIDIDeviceBase &device, uint8_t number, bool active) {
    const std::vector<uint8_t> expected = {
        static_cast<uint8_t>(active ? 0x90 : 0x80), number, static_cast<uint8_t>(active ? 3 : 0)
    };
    return std::find(device.sent.begin(), device.sent.end(), expected) != device.sent.end();
}
static MIDIParameterInputBase *find_input(DeviceBehaviour_APC40MKII &behaviour, const char *name) {
    for (auto *input : *behaviour.parameter_inputs)
        if (std::strcmp(input->get_name(), name) == 0) return input;
    assert(false);
    return nullptr;
}
class RawScanProbe : public MIDIParameterInputBase {
public:
    unsigned scans = 0;
    RawScanProbe() : MIDIParameterInputBase((char *)"Probe", "Test") {}
    bool responds_to_cc(byte, byte) override { ++scans; return false; }
};
int main() {
    test_modulation_row_layout();
    DeviceBehaviour_APC40MKII behaviour;
    assert(manager.inputs.size() == 35);
    auto *pan = behaviour.get_track_knob_input(APC40TrackKnobBank::Pan, 0);
    auto *sends = behaviour.get_track_knob_input(APC40TrackKnobBank::Sends, 0);
    auto *user = behaviour.get_track_knob_input(APC40TrackKnobBank::User, 7);
    assert(pan && sends && user);
    assert(sends->get_bank() == APC40TrackKnobBank::Sends && sends->get_track() == 0);
    assert(user->get_track() == 7);
    assert(std::strcmp(pan->get_group_and_name(), "APC40 Pan:PanKnob1") == 0);
    assert(std::strcmp(sends->get_group_and_name(), "APC40 Sends:SendsKnob1") == 0);
    assert(std::strcmp(user->get_group_and_name(), "APC40 User:UserKnob8") == 0);
    assert(!behaviour.get_track_knob_input(APC40TrackKnobBank::Pan, 8));
    assert(!behaviour.get_track_knob_input(static_cast<APC40TrackKnobBank>(3), 0));
    assert(pan->supports_bipolar_input());
    assert(sends->set_value(37) && sends->get_value() == 37);
    assert(pan->get_value() == 0);
    assert(user->set_value(255) && user->get_value() == 127);
    behaviour.init();
    assert(!behaviour.get_record_enabled());
    playing = true;
    transport_state.record_enabled = true;
    MIDIDeviceBase device;
    behaviour.device = &device;
    behaviour.init();
    assert(sent_button(device, APC40MKII::transport::NOTE_BUTTON_PLAY, true));
    assert(sent_button(device, APC40MKII::transport::NOTE_BUTTON_RECORD, true));
    assert(sent_button(device, APC40MKII::transport::NOTE_BUTTON_SESSION, false));
    assert(sent_cc(device, APC40MKII::tracks::CC_KNOB_1 + 8, 2));
    assert(stop_calls == 0 && continue_calls == 0);
    device.sent.clear();
    behaviour.loop(0);
    assert(device.sent.empty());
    behaviour.receive_note_on(1, APC40MKII::transport::NOTE_BUTTON_PLAY, 127);
    assert(!playing && stop_calls == 1 && song_position == 384);
    assert(sent_button(device, APC40MKII::transport::NOTE_BUTTON_PLAY, false));
    behaviour.receive_note_on(1, APC40MKII::transport::NOTE_BUTTON_PLAY, 127);
    assert(stop_calls == 1 && continue_calls == 0);
    behaviour.receive_note_off(1, APC40MKII::transport::NOTE_BUTTON_PLAY, 127);
    assert(stop_calls == 1 && continue_calls == 0);
    behaviour.receive_note_on(1, APC40MKII::transport::NOTE_BUTTON_PLAY, 127);
    assert(playing && continue_calls == 1 && song_position == 384);
    assert(sent_button(device, APC40MKII::transport::NOTE_BUTTON_PLAY, true));
    behaviour.receive_note_on(1, APC40MKII::transport::NOTE_BUTTON_RECORD, 127);
    assert(!behaviour.get_record_enabled());
    behaviour.receive_note_on(1, APC40MKII::transport::NOTE_BUTTON_RECORD, 127);
    assert(!behaviour.get_record_enabled());
    behaviour.receive_note_on(1, APC40MKII::transport::NOTE_BUTTON_RECORD, 0);
    behaviour.receive_note_on(1, APC40MKII::transport::NOTE_BUTTON_RECORD, 127);
    assert(behaviour.get_record_enabled());
    device.sent.clear();
    behaviour.receive_note_on(1, APC40MKII::transport::NOTE_BUTTON_SESSION, 127);
    assert(device.sent.empty());
    behaviour.receive_note_off(1, APC40MKII::transport::NOTE_BUTTON_SESSION, 0);
    unsigned session_presses = 0;
    behaviour.on_session_pressed = [&] { ++session_presses; behaviour.set_session_state(true); };
    behaviour.receive_note_on(1, APC40MKII::transport::NOTE_BUTTON_SESSION, 127);
    behaviour.receive_note_on(1, APC40MKII::transport::NOTE_BUTTON_SESSION, 127);
    assert(session_presses == 1);
    assert(sent_button(device, APC40MKII::transport::NOTE_BUTTON_SESSION, true));
    device.sent.clear();
    playing = false;
    transport_state.record_enabled = false;
    behaviour.set_session_state(false);
    behaviour.loop(0);
    assert(sent_button(device, APC40MKII::transport::NOTE_BUTTON_PLAY, false));
    assert(sent_button(device, APC40MKII::transport::NOTE_BUTTON_RECORD, false));
    assert(sent_button(device, APC40MKII::transport::NOTE_BUTTON_SESSION, false));
    assert(stop_calls == 1 && continue_calls == 1 && session_presses == 1);
    device.sent.clear();
    behaviour.loop(0);
    assert(device.sent.empty());
    RawScanProbe probe;
    behaviour.addParameterInput(&probe);
    behaviour.init();
    assert(device.sent.empty());
    device.sent.clear();
    assert(sends->set_value(42));
    assert(device.sent.empty());
    behaviour.receive_note_on(1, APC40MKII::tracks::NOTE_BUTTON_SENDS, 127);
    assert(sent_cc(device, APC40MKII::tracks::CC_KNOB_1, 42));
    device.sent.clear();
    behaviour.receive_control_change(1, APC40MKII::tracks::CC_KNOB_1, 99);
    assert(sends->get_value() == 99 && pan->get_value() == 0);
    assert(probe.scans == 0);
    assert(device.sent.empty());
    assert(sends->set_value(64));
    assert(sent_cc(device, APC40MKII::tracks::CC_KNOB_1, 64));
    assert(close_to(sends->get_normal_value_unipolar(), 64.0f / 127.0f));
    assert(close_to(sends->get_normal_value_bipolar(), 0.0f));
    device.sent.clear();
    sends->input_type = BIPOLAR;
    sends->loop();
    assert(sent_cc(device, APC40MKII::tracks::CC_KNOB_1 + 8, 3));
    assert(sent_cc(device, APC40MKII::tracks::CC_KNOB_1, 64));
    device.sent.clear();
    sends->loop();
    assert(device.sent.empty());
    sends->input_type = UNIPOLAR;
    sends->loop();
    assert(sent_cc(device, APC40MKII::tracks::CC_KNOB_1 + 8, 2));
    device.sent.clear();
    user->input_type = BIPOLAR;
    user->loop();
    assert(device.sent.empty());
    behaviour.receive_note_on(1, APC40MKII::tracks::NOTE_BUTTON_USER, 127);
    assert(sent_cc(device, APC40MKII::tracks::CC_KNOB_8 + 8, 3));
    assert(sent_cc(device, APC40MKII::tracks::CC_KNOB_8, 127));
    assert(user->set_value(0));
    assert(close_to(user->get_normal_value_bipolar(), -1.0f));
    assert(std::strstr(user->getOutputValue(), "-100"));
    assert(user->set_value(127));
    assert(close_to(user->get_normal_value_bipolar(), 1.0f));
    behaviour.receive_control_change(2, APC40MKII::tracks::CC_KNOB_8, 10);
    assert(user->get_value() == 127);
    assert(probe.scans == 1);
    probe.scans = 0;
    behaviour.receive_control_change(1, APC40MKII::tempo::CC_TEMPO, 1);
    assert(close_to(bpm, 121.0f));
    behaviour.receive_control_change(1, APC40MKII::tempo::CC_TEMPO, 127);
    assert(close_to(bpm, 120.0f) && probe.scans == 0);
    behaviour.receive_control_change(2, APC40MKII::tempo::CC_TEMPO, 1);
    assert(close_to(bpm, 120.0f) && probe.scans == 1);
    probe.scans = 0;
    auto *cue = find_input(behaviour, "CueKnob");
    assert(!cue->responds_to_cc(APC40MKII::tracks::CC_CUE_LEVEL, 1));
    behaviour.receive_control_change(1, APC40MKII::tracks::CC_CUE_LEVEL, 5);
    assert(close_to(cue->get_normal_value_unipolar(), 5.0f / 127.0f));
    behaviour.receive_control_change(1, APC40MKII::tracks::CC_CUE_LEVEL, 127);
    assert(close_to(cue->get_normal_value_unipolar(), 4.0f / 127.0f));
    behaviour.receive_control_change(1, APC40MKII::tracks::CC_CUE_LEVEL, 120);
    assert(close_to(cue->get_normal_value_unipolar(), 0.0f));
    for (unsigned turn = 0; turn < 3; ++turn)
        behaviour.receive_control_change(1, APC40MKII::tracks::CC_CUE_LEVEL, 63);
    assert(close_to(cue->get_normal_value_unipolar(), 1.0f));
    assert(probe.scans == 0);
    behaviour.receive_control_change(2, APC40MKII::tracks::CC_CUE_LEVEL, 127);
    assert(close_to(cue->get_normal_value_unipolar(), 1.0f));
    assert(probe.scans == 1);
    probe.scans = 0;
    behaviour.receive_control_change(1, APC40MKII::tracks::CC_FADER, 11);
    behaviour.receive_control_change(8, APC40MKII::tracks::CC_FADER, 88);
    assert(close_to(find_input(behaviour, "TrackFader1")->get_normal_value_unipolar(), 11.0f / 127.0f));
    assert(close_to(find_input(behaviour, "TrackFader8")->get_normal_value_unipolar(), 88.0f / 127.0f));
    assert(probe.scans == 0);
    behaviour.receive_control_change(9, APC40MKII::tracks::CC_FADER, 55);
    assert(probe.scans == 1);
    probe.scans = 0;
    behaviour.receive_control_change(1, APC40MKII::tracks::CC_FADER_MASTER, 70);
    behaviour.receive_control_change(1, APC40MKII::CC_CROSSFADER, 60);
    assert(close_to(find_input(behaviour, "MasterFader")->get_normal_value_unipolar(), 70.0f / 127.0f));
    assert(close_to(find_input(behaviour, "Crossfader")->get_normal_value_unipolar(), 60.0f / 127.0f));
    assert(probe.scans == 2);
    behaviour.receive_control_change(1, 100, 55);
    assert(probe.scans == 3);
    behaviour.disconnect_device();
    device.sent.clear();
    playing = true;
    behaviour.set_record_enabled(true);
    behaviour.set_session_state(true);
    behaviour.loop(0);
    assert(user->set_value(23));
    user->input_type = UNIPOLAR;
    user->loop();
    assert(device.sent.empty());
    behaviour.device = &device;
    behaviour.init();
    assert(playing && behaviour.get_record_enabled());
    assert(sent_button(device, APC40MKII::transport::NOTE_BUTTON_PLAY, true));
    assert(sent_button(device, APC40MKII::transport::NOTE_BUTTON_RECORD, true));
    assert(sent_button(device, APC40MKII::transport::NOTE_BUTTON_SESSION, true));
    assert(stop_calls == 1 && continue_calls == 1 && session_presses == 1);
    assert(user->get_value() == 23 && sends->get_value() == 64);
    assert(sent_cc(device, APC40MKII::tracks::CC_KNOB_8, 23));
    assert(sent_cc(device, APC40MKII::tracks::CC_KNOB_8 + 8, 2));
    device.sent.clear();
    assert(user->set_value(23));
    user->loop();
    assert(device.sent.empty());
    behaviour.receive_note_on(1, APC40MKII::transport::NOTE_BUTTON_PLAY, 127);
    behaviour.receive_note_on(1, APC40MKII::transport::NOTE_BUTTON_RECORD, 127);
    behaviour.receive_note_on(1, APC40MKII::transport::NOTE_BUTTON_SESSION, 127);
    assert(!playing && !behaviour.get_record_enabled());
    assert(stop_calls == 2 && continue_calls == 1 && session_presses == 2);
    #ifdef ENABLE_PARAMETERS_MIDI_LEARN
        test_learn_lifecycle();
        behaviour.on_session_pressed = nullptr;
        behaviour.set_session_state(false);
        FloatParameter target, other;
        const auto arm = [&] {
            behaviour.receive_note_off(1, APC40MKII::transport::NOTE_BUTTON_SESSION, 0);
            behaviour.receive_note_on(1, APC40MKII::transport::NOTE_BUTTON_SESSION, 127);
        };
        behaviour.receive_note_off(1, APC40MKII::transport::NOTE_BUTTON_SESSION, 0);
        behaviour.receive_note_on(1, APC40MKII::transport::NOTE_BUTTON_SESSION, 127);
        assert(manager.start_midi_learn(&target, 1));
        behaviour.receive_control_change(1, 48, 5);
        assert(manager.is_midi_learn_active() && target.assignments == 0);
        assert(sent_button(device, APC40MKII::transport::NOTE_BUTTON_SESSION, true));
        device.sent.clear();
        behaviour.loop(0);
        assert(device.sent.empty());
        arm();
        now += 250;
        behaviour.loop(0);
        assert(sent_button(device, APC40MKII::transport::NOTE_BUTTON_SESSION, false));
        behaviour.receive_control_change(2, 48, 7);
        behaviour.receive_control_change(1, 100, 7);
        behaviour.receive_control_change(1, 16, 7);
        assert(manager.is_midi_learn_active() && target.assignments == 0);
        behaviour.receive_control_change(1, 55, 71);
        assert(target.connections[1].parameter_input == user && target.assignments == 1);
        assert(user->get_value() == 71 && !manager.is_midi_learn_active());
        behaviour.receive_control_change(1, 55, 72);
        assert(target.assignments == 1 && user->get_value() == 72);
        assert(manager.start_midi_learn(&target, 0));
        arm();
        assert(manager.start_midi_learn(&other, 2));
        behaviour.receive_control_change(1, 55, 73);
        assert(manager.is_midi_learn_active() && other.assignments == 0);
        arm();
        manager.stop_midi_learn();
        assert(manager.start_midi_learn(&other, 2));
        behaviour.receive_control_change(1, 55, 74);
        assert(other.assignments == 0);
        arm();
        behaviour.disconnect_device();
        behaviour.device = &device;
        behaviour.init();
        behaviour.receive_control_change(1, 55, 75);
        assert(manager.is_midi_learn_active() && other.assignments == 0);
        arm();
        behaviour.receive_control_change(1, 55, 76);
        assert(other.connections[2].parameter_input == user && other.assignments == 1);
        for (uint8_t bank = 0; bank < 3; ++bank) {
            behaviour.receive_note_on(1, static_cast<uint8_t>(87 + bank), 127);
            for (uint8_t track = 0; track < 8; ++track) {
                assert(manager.start_midi_learn(&target, 0));
                arm();
                behaviour.receive_control_change(1, static_cast<uint8_t>(48 + track), 65);
                assert(!manager.is_midi_learn_active());
                assert(target.connections[0].parameter_input ==
                    behaviour.get_track_knob_input(static_cast<APC40TrackKnobBank>(bank), track));
            }
        }
        probe.scans = 0;
        for (uint8_t track = 0; track < 8; ++track) {
            assert(manager.start_midi_learn(&target, 0));
            arm();
            behaviour.receive_control_change(track + 1, 7, 90);
            char name[16];
            std::snprintf(name, sizeof(name), "TrackFader%u", track + 1);
            auto *input = find_input(behaviour, name);
            assert(!manager.is_midi_learn_active() && target.connections[0].parameter_input == input);
            assert(close_to(input->get_normal_value_unipolar(), 90.0f / 127.0f));
        }
        assert(probe.scans == 0);
        const uint8_t faders[] = {14, 15};
        const char *names[] = {"MasterFader", "Crossfader"};
        for (unsigned index = 0; index < 2; ++index) {
            assert(manager.start_midi_learn(&target, 0));
            arm();
            behaviour.receive_control_change(1, faders[index], 91);
            auto *input = find_input(behaviour, names[index]);
            assert(!manager.is_midi_learn_active() && target.connections[0].parameter_input == input);
            assert(close_to(input->get_normal_value_unipolar(), 91.0f / 127.0f));
        }
        assert(probe.scans == 2);
        assert(manager.start_midi_learn(&target, 0));
        arm();
        behaviour.receive_control_change(1, 47, 0);
        assert(manager.is_midi_learn_active());
        behaviour.receive_control_change(1, 47, 127);
        assert(!manager.is_midi_learn_active() && target.connections[0].parameter_input == cue);

        APC40USBHostTransport map_transport;
        APC40Controller map_controller(map_transport);
        map_controller.setup_banks();
        APC40ParameterInputMap map;
        APC40LogicalKnobParameterInput device_knob((char *)"Device8Knob8", "APC40 Device8",
            map_controller, APC40ControlId{APC40ControlKind::DeviceKnob, 7, 7});
        assert(device_knob.set_value(42) && device_knob.get_value() == 42);
        device_knob.input_type = BIPOLAR;
        device_knob.loop();
        APC40VirtualEncoderState state;
        assert(map_controller.getKnobState(device_knob.get_control_id(), state));
        assert(state.value == 42 && state.polarity == APC40EncoderPolarity::Bipolar);
        assert(std::strcmp(device_knob.getInputInfo(), "Device 8 knob 8") == 0);
        assert(map.bind(device_knob.get_control_id(), &device_knob));
        assert(map.resolve(device_knob.get_control_id()) == &device_knob);
        assert(map.bind({APC40ControlKind::TrackKnob, 0, 0}, pan));
        assert(map.resolve({APC40ControlKind::TrackKnob, 0, 0}) == pan);
        assert(map.resolve({APC40ControlKind::DeviceKnob, 0, 0}) == nullptr);
        assert(!map.bind({APC40ControlKind::DeviceKnob, 8, 0}, pan));
        assert(!map.bind({APC40ControlKind::TrackKnob, 3, 0}, pan));
        assert(!map.bind({APC40ControlKind::TrackFader, 0, 8}, pan));
        assert(!map.bind({APC40ControlKind::MasterFader, 1, 0}, pan));
        assert(!map.resolve({APC40ControlKind::Pad, 0, 0}));
        assert(!map_controller.getKnobState({APC40ControlKind::DeviceKnob, 8, 0}, state));
        assert(!map_controller.setKnobState({APC40ControlKind::TrackKnob, 0, 8}, state));
        assert(!map_controller.getKnobState({APC40ControlKind::TrackFader, 0, 0}, state));
        assert(map.bind(device_knob.get_control_id(), nullptr));
        assert(map.resolve(device_knob.get_control_id()) == nullptr);
        std::cout << "MIDI learn bank identity, fader fallback, target changes and reconnect checks passed\\n";
    #endif
    std::cout << "APC40 MKII buttons, banked inputs, feedback, Cue, faders and reconnect checks passed\\n";
}
#else
int main() {
    test_modulation_row_layout();
    test_learn_lifecycle();
    std::cout << "MIDI learn manager and menu lifecycle checks passed\\n";
}
#endif
`;
const temporary = fs.mkdtempSync(path.join(os.tmpdir(), 'apc40mkii-host-test-'));
try {
    const filename = path.join(temporary, 'test.cpp');
    const executable = path.join(temporary, process.platform === 'win32' ? 'test.exe' : 'test');
    fs.writeFileSync(filename, source);
    const implementation = ['APC40Controller.cpp', 'APC40Device.cpp', 'APC40Feedback.cpp', 'APC40BankSet.cpp', 'APCControl.cpp'];
    const compile = spawnSync(process.env.CXX || 'g++', [
        '-std=c++17', '-Wall', '-Wextra', '-Wno-unused-parameter', '-Wno-unknown-pragmas',
        ...(learnEnabled ? ['-DENABLE_PARAMETERS_MIDI_LEARN', '-DMENU_PERF_PARTIAL_UPDATES=1'] : []),
        ...(lifecycleOnly ? ['-DAPC40_TEST_LIFECYCLE_ONLY'] : []),
        '-fsanitize=undefined', '-fno-sanitize-recover=undefined',
        '-I', path.join(apcLibrary, 'src'), '-I', path.join(foundling, 'include'),
        '-I', path.join(root, 'include'),
        filename, ...implementation.map(name => path.join(apcLibrary, 'src', name)), '-o', executable
    ], { encoding: 'utf8' });
    if (compile.error) throw compile.error;
    if (compile.status !== 0) throw new Error(compile.stderr || compile.stdout);
    const run = spawnSync(executable, [], { encoding: 'utf8' });
    if (run.error) throw run.error;
    process.stdout.write(run.stdout);
    process.stderr.write(run.stderr);
    if (run.status !== 0) throw new Error(`Host test failed: ${run.status ?? run.signal}`);
} finally {
    fs.rmSync(temporary, { recursive: true, force: true });
}