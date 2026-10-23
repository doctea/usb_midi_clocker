const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const root = path.resolve(__dirname, '..');
function read(file) {
    return fs.readFileSync(path.join(root, file), 'utf8');
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
const manager = read(path.join('include', 'midi', 'midi_mapper_matrix_manager.h'));
const wrapper = read(path.join('include', 'midi', 'midi_out_wrapper.h'));
const ui = read(path.join('include', 'mymenu', 'menu_midi_matrix.h'));
const consoleHeader = read(path.join('include', 'debug_console.h'));
const commandStart = consoleHeader.indexOf('bool execute_command(');
const commandEnd = consoleHeader.indexOf('} else if (strcmp(command, "debug")', commandStart);
if (commandStart < 0 || commandEnd < 0) throw new Error('Cannot locate notes console dispatch');
const classStart = manager.indexOf('class MIDIMatrixManager :');
const classEnd = manager.indexOf('\n    private:', classStart);
if (classStart < 0 || classEnd < 0) throw new Error('Cannot locate matrix public implementation');
const fixture = `
#include <cassert>
#include <cstdarg>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <cstdio>
#include <cctype>
#include <memory>
#include <new>
#ifdef ARDUINO
    #include <util/atomic.h>
#else
    static unsigned diagnostic_irq_depth = 0;
#endif
#include "midi/midi_note_diagnostics.h"
#include "midi/midi_connection_store.h"
#include "midi/midi_connection_note_state.h"
#define FLASHMEM
#define F(text) text
static std::vector<std::string> warnings;
static void Serial_printf(const char *format, ...) {
    assert(diagnostic_irq_depth == 0);
    char text[512];
    va_list args;
    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    warnings.push_back(text);
}
#define Serial_println(text) Serial_printf("%s", text)
#define Serial_flush() ((void)0)
#define MAX_NUM_SOURCES 50
#define MAX_NUM_TARGETS 50
#define NUM_REGISTERED_SOURCES sources_count
#define NUM_REGISTERED_TARGETS targets_count
#define LANGST_HANDEL_ROUT 25
#define SL_MAX_LINE 512
#define SL_MAX_LABEL 80
#define MIDI_MIN_VELOCITY 0
#define MIDI_NUM_NOTES 128
#define NOTE_OFF -1
#define GM_CHANNEL_DRUMS 10
static uint32_t now_ms = 0;
static uint32_t millis() { return now_ms; }
static bool is_valid_note(int pitch) { return pitch >= 0 && pitch < 128; }
static const char *get_note_name_c(int) { return "note"; }
enum { QUANTISE_MODE_NONE, QUANTISE_MODE_SCALE, QUANTISE_MODE_CHORD };
static int transpose = 0;
static bool collapse_scale_pairs = false;
static int8_t quantise_pitch_to_scale(int8_t pitch) { return (collapse_scale_pairs ? pitch - pitch % 2 : pitch) + transpose; }
static int8_t quantise_pitch_to_chord(int8_t pitch, int) { return pitch + transpose; }
struct Conductor {
    int mode = QUANTISE_MODE_NONE;
    int get_global_quantise_mode() { return mode; }
} conductor_instance;
static Conductor *conductor = &conductor_instance;
class DeviceBehaviourUltimateBase : public MIDIEndpointOwner {
public:
    const char *get_endpoint_owner_label() override { return "Mock"; }
    int8_t TUNING_OFFSET = 2;
    int8_t get_effective_lowest_note() { return 24; }
    int8_t get_effective_highest_note() { return 100; }
    int getLowestNoteMode() { return 0; }
    int getHighestNoteMode() { return 1; }
};
class MIDITrack {};
class HardwareSerial {};
namespace midi {
    template <typename> class SerialMIDI {};
    template <typename> class MidiInterface {};
}
constexpr int NUM_MIDI_OUTS = 1;
static midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *midi_out_serial[NUM_MIDI_OUTS] = {};
struct Event { char type; int pitch, value, channel; };
struct NoteTarget {
    virtual void sendNoteOn(uint8_t, uint8_t, uint8_t) {}
    virtual void sendNoteOff(uint8_t, uint8_t, uint8_t) {}
    virtual void sendControlChange(uint8_t, uint8_t, uint8_t) {}
};
class MIDIOutputWrapper : public NoteTarget {
public:
    bool debug = false, always_force_stop_all = false;
    MIDINoteTraceContext *note_trace_context = nullptr;
    int8_t default_channel = 1;
    const char *label = "Mock";
    int8_t playing_notes[128] = {};
    int last_note = NOTE_OFF, current_note = NOTE_OFF;
    std::vector<Event> events;
    void log_message_on(int, int, int) {}
    void log_message_off(int, int, int) {}
    void actual_sendNoteOn(int p, int v, int c) { assert(diagnostic_irq_depth == 0); events.push_back({'N', p, v, c}); }
    void actual_sendNoteOff(int p, int v, int c) { assert(diagnostic_irq_depth == 0); events.push_back({'F', p, v, c}); }
    void actual_sendControlChange(int p, int v, int c) { events.push_back({'C', p, v, c}); }
    void actual_sendPitchBend(int b, int c) { events.push_back({'B', b, 0, c}); }
    ${method(wrapper, 'virtual void sendNoteOn(uint8_t')}
    ${method(wrapper, 'virtual void sendNoteOff(uint8_t')}
    ${method(wrapper, 'virtual void sendControlChange(uint8_t')}
    ${method(wrapper, 'virtual void sendPitchBend(int')}
    MIDIEndpointOwner *get_owner() { return nullptr; }
    DeviceBehaviourUltimateBase *behaviour = nullptr;
    int target_transpose = 0;
    bool routes_to_behaviour(DeviceBehaviourUltimateBase *candidate) { return behaviour == candidate; }
    routed_note_output_t resolve_routed_note_output(int8_t p, uint8_t c) { return {static_cast<int8_t>(p + target_transpose), c, true}; }
    ${method(wrapper, 'virtual MIDINoteTargetDebugState get_note_debug_state(')}
    ${method(wrapper, 'virtual void send_resolved_routed_note_off(')}
    ${method(wrapper, 'virtual void send_resolved_routed_note_on(')}
    void stop_all_notes(bool) {}
    void force_stop_all_notes_on_channel(uint8_t) {}
};
struct BehaviourDebugWrapper : MIDIOutputWrapper {
    DeviceBehaviourUltimateBase *output = nullptr;
    ${method(wrapper.slice(wrapper.indexOf('class MIDIOutputWrapper_Behaviour :')), 'virtual MIDINoteTargetDebugState get_note_debug_state(')}
};
static MIDIOutputWrapper *make_midioutputwrapper(const char *, MIDITrack *, uint8_t) {
    return nullptr;
}
using sl_scope_t = unsigned int;
constexpr sl_scope_t SL_SCOPE_ALL = 255, SL_SCOPE_ROUTING = 1;
using SL_PrintCallback = void (*)(const char *, void *);
struct ISaveableSettingHost {
    virtual void save_recursive(char *, size_t, void (*)(const char *), sl_scope_t = SL_SCOPE_ALL) {}
    virtual void save_recursive(char *, size_t, void (*)(const char *, void *), void *, sl_scope_t = SL_SCOPE_ALL) {}
    virtual void print_dynamic_entries(const char *, SL_PrintCallback, void *, sl_scope_t) {}
    virtual bool load_line(char **, int, const char *, sl_scope_t = SL_SCOPE_ALL) { return false; }
    virtual void setup_saveable_settings() {}
};
template <int, int> struct SHDynamic : ISaveableSettingHost {};
static void sl_trim_inplace(char *text) {
    char *first = text;
    while (isspace(static_cast<unsigned char>(*first))) ++first;
    memmove(text, first, strlen(first) + 1);
    size_t length = strlen(text);
    while (length && isspace(static_cast<unsigned char>(text[length - 1]))) text[--length] = 0;
}
${manager.slice(classStart, classEnd)}
};
${read(path.join('include', 'midi', 'midi_routing_model.h')).replace('#pragma once', '').replace('#include "midi/midi_mapper_matrix_manager.h"', '')}
${read(path.join('include', 'midi', 'midi_routing_lists.h')).replace('#pragma once', '').replace('#include "midi/midi_routing_model.h"', '')}
static std::string routing_ui_message;
static bool routing_ui_error = false;
static int routing_ui_jump = -1;
void midi_routing_ui_message(const char *text, bool error) { routing_ui_message = text; routing_ui_error = error; }
void midi_routing_ui_jump_to_page(int16_t page_index) { routing_ui_jump = page_index; }
${read(path.join('include', 'midi', 'midi_note_debug_console.h')).replace('#pragma once', '').replace('#include "midi/midi_mapper_matrix_manager.h"', '')}
struct SerialConsole {
    template <typename... Args> void printf(const char *format, Args... args) { Serial_printf(format, args...); }
} Serial;
${consoleHeader.slice(commandStart, commandEnd)}
    }
    return false;
}
static constexpr unsigned MENU_MESSAGE_MAX = 128;
static constexpr int RED = 1, GREEN = 2;
static std::string last_message;
static int last_colour = 0;
static void menu_set_last_message(const char *text, int colour) {
    last_message = text;
    last_colour = colour;
}
struct Menu {
    int jumped_page = -1;
    void jump_to_page_with_return(int page) { jumped_page = page; }
} menu_instance;
static Menu *menu = &menu_instance;
struct Selector {
    virtual bool button_select() { return false; }
    virtual bool button_right() { return false; }
    virtual bool button_back() { return false; }
};
struct Popup : Selector {
    enum class Mode { SOURCE_SELECT, TARGET_SELECT, CONTEXT_PANEL };
    ${ui.slice(ui.indexOf('enum class PopupRow'), ui.indexOf('enum class Mode'))}
    Mode mode = Mode::SOURCE_SELECT, context_return_mode = Mode::TARGET_SELECT;
    int selected_context_index = 0, selected_value_index = 0;
    int selected_source_value_index = 0, selected_target_value_index = 0;
    int actual_value_index = 0, selected_source_index = -1;
    int sel_source = -1, sel_target = -1;
    char ctx_scratch[40];
    struct { bool go_back_on_select = false; } flags;
    int get_num_available() { return static_cast<int>(PopupRow::_COUNT); }
    void setter(int) {}
    ${method(ui, 'int wrap_index(')}
    ${method(ui, 'MIDIRoutingModel routing()')}
    ${method(ui, 'bool popup_row_enabled(')}
    ${method(ui, 'const char *popup_row_label(')}
    ${method(ui, 'void popup_disconnect_others_to_target(')}
    ${method(ui, 'void popup_disconnect_others_from_source(')}
    ${method(ui, 'void popup_set_paused_bulk(')}
    ${method(ui, 'void enter_context_panel(')}
    ${method(ui, 'void toggle_connection_with_feedback(')}
    ${method(ui, 'virtual bool button_select(')}
    ${method(ui, 'virtual bool button_right(')}
    ${method(ui, 'virtual bool button_back(')}
};
`;
const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'midi-connections-test-'));
try {
    fs.writeFileSync(path.join(directory, 'matrix_fixture.h'), fixture);
    fs.mkdirSync(path.join(directory, 'util'));
    fs.writeFileSync(path.join(directory, 'util', 'atomic.h'), `
#pragma once
static unsigned diagnostic_irq_depth = 0;
struct DiagnosticAtomicTest {
    unsigned previous;
    bool active = true;
    DiagnosticAtomicTest() : previous(diagnostic_irq_depth) { ++diagnostic_irq_depth; }
    ~DiagnosticAtomicTest() { diagnostic_irq_depth = previous; }
};
#define ATOMIC_RESTORESTATE 0
#define ATOMIC_BLOCK(mode) for (DiagnosticAtomicTest guard; guard.active; guard.active = false)
`);
    for (const arduino of [false, true]) {
        const executable = path.join(directory, process.platform === 'win32' ? 'test.exe' : 'test');
        const compile = spawnSync('g++', ['-std=c++11', '-Wall', '-Wextra', '-Werror',
            '-fsanitize=undefined', ... (arduino ? ['-DARDUINO'] : []),
            '-I', path.join(root, 'include'), '-I', directory,
            '-I', path.resolve(root, '..', 'my_libraries', 'mymenu', 'include'),
            path.join(__dirname, 'native', 'midi_connection_store_test.cpp'), '-o', executable],
            { encoding: 'utf8' });
        if (compile.error) throw compile.error;
        if (compile.status !== 0) throw new Error(compile.stdout + compile.stderr);
        const run = spawnSync(executable, [], { encoding: 'utf8' });
        if (run.error) throw run.error;
        if (run.status !== 0) throw new Error(run.stdout + run.stderr);
        process.stdout.write(`${arduino ? 'IRQ-guard mock' : 'Native'}:\n${run.stdout}`);
    }
} finally {
    const atomicHeader = path.join(directory, 'util', 'atomic.h');
    if (fs.existsSync(atomicHeader)) fs.unlinkSync(atomicHeader);
    if (fs.existsSync(path.join(directory, 'util'))) fs.rmdirSync(path.join(directory, 'util'));
    for (const name of fs.readdirSync(directory)) fs.unlinkSync(path.join(directory, name));
    fs.rmdirSync(directory);
}
