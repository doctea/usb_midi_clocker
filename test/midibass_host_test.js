const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const root = path.resolve(__dirname, '..');
function read(...parts) {
    return fs.readFileSync(path.join(root, ...parts), 'utf8');
}
function method(text, marker) {
    const start = text.indexOf(marker);
    if (start < 0) throw new Error(`Cannot locate ${marker}`);
    const body = text.indexOf('{', start);
    let depth = 1;
    for (let end = body + 1; end < text.length; ++end) {
        if (text[end] === '{') ++depth;
        if (text[end] === '}' && --depth === 0) return text.slice(start, end + 1);
    }
    throw new Error(`Unclosed method ${marker}`);
}
const base = read('include', 'behaviours', 'behaviour_base.h');
const bass = read('include', 'behaviours', 'behaviour_midibass.h')
    .replace(/^#include .*$/gm, '');
const source = `
#include <cassert>
#include <cstdint>
#include <vector>
using byte = uint8_t;
#define NOTE_OFF -1
#define MIDI_MIN_VELOCITY 0
#define MIDI_MAX_VELOCITY 127
#define PPQN 24
#define F(text) text
#define SL_SCOPE_SCENE 0
struct SerialMock {
    template<typename... Args> void printf(const char *, Args...) {}
} Serial;
#define Serial_printf(...) Serial.printf(__VA_ARGS__)
static bool is_valid_note(int note) { return note >= 0 && note < 128; }
static const char *get_note_name_c(int) { return "note"; }
static int8_t apply_note_limits(int note, int, int, int low, int high) {
    return note >= low && note <= high ? note : NOTE_OFF;
}
struct routed_note_output_t {
    int8_t pitch = NOTE_OFF;
    uint8_t channel = 0;
    bool emitted = false;
};
template<typename T> struct LSaveableSetting {
    template<typename... Args> LSaveableSetting(Args...) {}
};
struct MatrixMock { void stop_all_notes_for_target(int) {} } matrix;
static MatrixMock *midi_matrix_manager = &matrix;
struct Event { bool on; int note, channel; };
class DeviceBehaviourUltimateBase {
public:
    virtual ~DeviceBehaviourUltimateBase() {}
    bool debug = false;
    int target_id = 0, TUNING_OFFSET = 0;
    int8_t current_transposed_note = NOTE_OFF;
    uint8_t current_channel = 0;
    unsigned tracked_notes = 0;
    std::vector<Event> events;
    bool sounding[128][17] = {};
    uint8_t routed_note_dispatch_depth = 0;
    ${method(base, 'class RoutedNoteDispatchScope')};
    ${method(base, 'bool is_processing_routed_note() const')}
    virtual const char *get_label() { return "Neutron mock"; }
    virtual bool transmits_midi_notes() { return true; }
    virtual bool supports_note_limits() { return true; }
    ${method(base, 'virtual void on_bar_prepare(')};
    virtual void on_bar(int) {}
    virtual void on_end_bar(int) {}
    virtual void on_pre_clock(uint32_t) {}
    virtual void killCurrentNote() {}
    virtual void setup_saveable_settings() {}
    template<typename T> void register_setting(T *setting, int) { delete setting; }
    int getLowestNoteMode() { return 0; }
    int getHighestNoteMode() { return 0; }
    int get_effective_lowest_note() { return 0; }
    int get_effective_highest_note() { return 127; }
    void note_tracker_held_note_on(int, int, int) { ++tracked_notes; }
    virtual routed_note_output_t resolve_routed_note_output(uint8_t, uint8_t) {
        return routed_note_output_t{};
    }
    virtual void send_resolved_routed_note_on(uint8_t n, uint8_t v, uint8_t c) {
        actualSendNoteOn(n, v, c);
    }
    virtual void send_resolved_routed_note_off(uint8_t n, uint8_t v, uint8_t c) {
        actualSendNoteOff(n, v, c);
    }
    virtual void actualSendNoteOn(uint8_t note, uint8_t, uint8_t channel) {
        assert(is_valid_note(note) && channel <= 16);
        events.push_back(Event{true, note, channel});
        sounding[note][channel] = true;
    }
    virtual void actualSendNoteOff(uint8_t note, uint8_t, uint8_t channel) {
        assert(is_valid_note(note) && channel <= 16);
        events.push_back(Event{false, note, channel});
        sounding[note][channel] = false;
    }
    ${method(base, 'virtual void sendRoutedNoteOn(')}
    ${method(base, 'virtual void sendRoutedNoteOff(')}
    virtual void sendNoteOn(uint8_t, uint8_t, uint8_t);
    virtual void sendNoteOff(uint8_t n, uint8_t v, uint8_t c) {
        actualSendNoteOff(n + TUNING_OFFSET, v, c);
    }
};
${method(read('src', 'behaviours', 'behaviour_base.cpp'),
    'void DeviceBehaviourUltimateBase::sendNoteOn(')}
${bass}
struct Manager {
    std::vector<DeviceBehaviourUltimateBase *> *behaviours;
    ${method(read('include', 'behaviours', 'behaviour_manager.h'), 'void do_bar(')}
};
struct ProgressionMock : DeviceBehaviourUltimateBase {
    MIDIBassBehaviour &target;
    explicit ProgressionMock(MIDIBassBehaviour &target) : target(target) {}
    void on_bar(int bar) override {
        target.sendRoutedNoteOn(36 + bar, 127, 1);
        target.sendRoutedNoteOff(36 + bar, 0, 1);
        target.sendRoutedNoteOn(48 + bar, 127, 2);
        target.sendRoutedNoteOff(48 + bar, 0, 2);
    }
};
static void test_bar_order(bool source_first, int machinegun) {
    MIDIBassBehaviour target;
    ProgressionMock progression(target);
    std::vector<DeviceBehaviourUltimateBase *> behaviours;
    if (source_first) behaviours = {&progression, &target};
    else behaviours = {&target, &progression};
    Manager manager{&behaviours};
    target.set_drone(true);
    target.set_machinegun(machinegun);
    for (int bar = 0; bar < 8; ++bar) {
        manager.do_bar(bar);
        const int note = 36 + bar;
        assert(target.get_drone_note() == note);
        assert(target.current_transposed_note == note);
        assert(target.drone_channel == 1);
        assert(target.sounding[note][1]);
        assert(target.tracked_notes == 0);
        for (unsigned tick = 0; tick < PPQN * 4; ++tick) {
            target.on_pre_clock(tick);
            assert(target.get_drone_note() == note);
            if (machinegun) {
                if ((tick + 1) % (PPQN / machinegun) == 0)
                    assert(!target.sounding[note][1]);
                else
                    assert(target.sounding[note][1]);
            } else {
                assert(target.sounding[note][1]);
            }
        }
        target.on_end_bar(bar);
        assert(!target.sounding[note][1]);
        assert(target.get_drone_note() == NOTE_OFF);
        assert(target.current_transposed_note == NOTE_OFF);
        target.sendRoutedNoteOn(70, 127, 1);
        assert(target.get_drone_note() == NOTE_OFF);
        for (int pitch = 0; pitch < 128; ++pitch)
            for (int channel = 0; channel <= 16; ++channel)
                assert(!target.sounding[pitch][channel]);
    }
    manager.do_bar(0);
    // A missed end-bar callback must release the old drone before the next source callback.
    manager.do_bar(1);
    assert(!target.sounding[36][1] && target.sounding[37][1]);
    assert(target.get_drone_note() == 37);
    target.set_drone(false);
    assert(!target.sounding[37][1]);
    assert(target.current_transposed_note == NOTE_OFF);
    assert(target.get_drone_note() == NOTE_OFF);
}
int main() {
    for (int machinegun = 0; machinegun <= 4; ++machinegun) {
        test_bar_order(true, machinegun);
        test_bar_order(false, machinegun);
    }
}
`;
const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'midibass-test-'));
try {
    const file = path.join(directory, 'test.cpp');
    const executable = path.join(directory, process.platform === 'win32' ? 'test.exe' : 'test');
    fs.writeFileSync(file, source);
    const compile = spawnSync('g++', ['-std=c++11', '-Wall', '-Wextra', '-Werror',
        '-Wno-unused-parameter', '-fsanitize=undefined', file, '-o', executable],
    { encoding: 'utf8' });
    if (compile.error) throw compile.error;
    if (compile.status !== 0) throw new Error(compile.stdout + compile.stderr);
    const run = spawnSync(executable, [], { encoding: 'utf8' });
    if (run.error) throw run.error;
    if (run.status !== 0) throw new Error(run.stdout + run.stderr);
    console.log('MIDIBass regression passed: both callback orders, drone sustain/release, machinegun 0..4, displays, and missed end-bar cleanup');
} finally {
    for (const name of fs.readdirSync(directory)) fs.unlinkSync(path.join(directory, name));
    fs.rmdirSync(directory);
}
