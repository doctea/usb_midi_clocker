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
using byte = uint8_t;
#define ENABLE_APC40MKII
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
struct ParameterManager {
    std::vector<void *> inputs;
    template<typename Input> void addInput(Input *input) { inputs.push_back(input); }
} manager;
ParameterManager *parameter_manager = &manager;
${parameterInput}
${midiInput}
${midiSource}
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
    virtual void setup_callbacks() {}
    virtual void disconnect_device() { device = nullptr; }
    virtual void receive_note_on(byte, byte, byte) {}
    virtual void receive_note_off(byte, byte, byte) {}
    virtual void receive_control_change(byte, byte, byte) {}
    bool is_connected() { return device != nullptr; }
};
struct SerialMock {
    template<typename... Args> void printf(const char *, Args...) {}
} Serial;
${behaviour}
${transport}
static bool close_to(float actual, float expected) {
    return std::fabs(actual - expected) < 0.0001f;
}
static bool sent_cc(const MIDIDeviceBase &device, uint8_t number, uint8_t value, uint8_t channel = 1) {
    const std::vector<uint8_t> expected = {static_cast<uint8_t>(0xB0 | (channel - 1)), number, value};
    return std::find(device.sent.begin(), device.sent.end(), expected) != device.sent.end();
}
static MIDIParameterInputBase *find_input(DeviceBehaviour_APC40MKII &behaviour, const char *name) {
    for (auto *input : *behaviour.parameter_inputs)
        if (std::strcmp(input->get_name(), name) == 0) return input;
    assert(false);
    return nullptr;
}
int main() {
    DeviceBehaviour_APC40MKII behaviour;
    assert(manager.inputs.size() == 35);
    auto *pan = behaviour.get_track_knob_input(APC40TrackKnobBank::Pan, 0);
    auto *sends = behaviour.get_track_knob_input(APC40TrackKnobBank::Sends, 0);
    auto *user = behaviour.get_track_knob_input(APC40TrackKnobBank::User, 7);
    assert(pan && sends && user);
    assert(sends->get_bank() == APC40TrackKnobBank::Sends && sends->get_track() == 0);
    assert(user->get_track() == 7);
    assert(!behaviour.get_track_knob_input(APC40TrackKnobBank::Pan, 8));
    assert(!behaviour.get_track_knob_input(static_cast<APC40TrackKnobBank>(3), 0));
    assert(pan->supports_bipolar_input());
    assert(sends->set_value(37) && sends->get_value() == 37);
    assert(pan->get_value() == 0);
    assert(user->set_value(255) && user->get_value() == 127);
    behaviour.init();
    MIDIDeviceBase device;
    behaviour.device = &device;
    behaviour.init();
    assert(sent_cc(device, APC40MKII::tracks::CC_KNOB_1 + 8, 2));
    device.sent.clear();
    assert(sends->set_value(42));
    assert(device.sent.empty());
    behaviour.receive_note_on(1, APC40MKII::tracks::NOTE_BUTTON_SENDS, 127);
    assert(sent_cc(device, APC40MKII::tracks::CC_KNOB_1, 42));
    device.sent.clear();
    behaviour.receive_control_change(1, APC40MKII::tracks::CC_KNOB_1, 99);
    assert(sends->get_value() == 99 && pan->get_value() == 0);
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
    behaviour.receive_control_change(2, APC40MKII::tracks::CC_CUE_LEVEL, 127);
    assert(close_to(cue->get_normal_value_unipolar(), 1.0f));
    behaviour.receive_control_change(1, APC40MKII::tracks::CC_FADER, 11);
    behaviour.receive_control_change(8, APC40MKII::tracks::CC_FADER, 88);
    assert(close_to(find_input(behaviour, "TrackFader1")->get_normal_value_unipolar(), 11.0f / 127.0f));
    assert(close_to(find_input(behaviour, "TrackFader8")->get_normal_value_unipolar(), 88.0f / 127.0f));
    behaviour.receive_control_change(1, APC40MKII::tracks::CC_FADER_MASTER, 70);
    behaviour.receive_control_change(1, APC40MKII::CC_CROSSFADER, 60);
    assert(close_to(find_input(behaviour, "MasterFader")->get_normal_value_unipolar(), 70.0f / 127.0f));
    assert(close_to(find_input(behaviour, "Crossfader")->get_normal_value_unipolar(), 60.0f / 127.0f));
    behaviour.disconnect_device();
    device.sent.clear();
    assert(user->set_value(23));
    user->input_type = UNIPOLAR;
    user->loop();
    assert(device.sent.empty());
    behaviour.device = &device;
    behaviour.init();
    assert(user->get_value() == 23 && sends->get_value() == 64);
    assert(sent_cc(device, APC40MKII::tracks::CC_KNOB_8, 23));
    assert(sent_cc(device, APC40MKII::tracks::CC_KNOB_8 + 8, 2));
    device.sent.clear();
    assert(user->set_value(23));
    user->loop();
    assert(device.sent.empty());
    std::cout << "APC40 MKII banked inputs, feedback, Cue, faders and reconnect checks passed\\n";
}
`;
const temporary = fs.mkdtempSync(path.join(os.tmpdir(), 'apc40mkii-host-test-'));
try {
    const filename = path.join(temporary, 'test.cpp');
    const executable = path.join(temporary, process.platform === 'win32' ? 'test.exe' : 'test');
    fs.writeFileSync(filename, source);
    const implementation = ['APC40Controller.cpp', 'APC40Device.cpp', 'APC40Feedback.cpp', 'APC40BankSet.cpp', 'APCControl.cpp'];
    const compile = spawnSync(process.env.CXX || 'g++', [
        '-std=c++17', '-Wall', '-Wextra', '-Wno-unused-parameter', '-Wno-unknown-pragmas',
        '-fsanitize=undefined', '-fno-sanitize-recover=undefined',
        '-I', path.join(apcLibrary, 'src'), '-I', path.join(foundling, 'include'),
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