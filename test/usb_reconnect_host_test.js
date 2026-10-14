const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

function extract(file, startMarker, endMarker) {
    const source = fs.readFileSync(path.join(__dirname, '..', file), 'utf8');
    const start = source.indexOf(startMarker);
    const end = source.indexOf(endMarker, start);
    if (start < 0 || end < 0) throw new Error(`Cannot locate ${startMarker}`);
    return source.slice(start, end);
}

const handlers = extract(
    'src/usb/multi_usb_handlers.cpp',
    'static void disconnect_usb_midi_slot(',
    '//#define SINGLE_FRAME_READ_ONCE',
);
const connect = extract(
    'include/behaviours/behaviour_manager.h',
    '            bool attempt_usb_device_connect(',
    '\n        #endif',
);
const mainSource = fs.readFileSync(path.join(__dirname, '..', 'src/main.cpp'), 'utf8');
const markerCalls = mainSource.match(/\bRECORD_USB_PHASE\(\w+\);/g);
if (!markerCalls || markerCalls.length < 2) throw new Error('Cannot locate loop phase markers');
const source = `
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#define ENABLE_USB
#ifndef USBHOST_T36_ENABLE_DIAGNOSTICS
#define USBHOST_T36_ENABLE_DIAGNOSTICS 1
#endif
#define F(value) value
#define NUM_USB_MIDI_DEVICES 4
static bool interrupts_enabled = true;
struct AtomicSnapshot {
    bool active = true;
    bool saved = interrupts_enabled;
    AtomicSnapshot() { interrupts_enabled = false; }
    ~AtomicSnapshot() { interrupts_enabled = saved; }
};
#define IRQ_PROTECT_USB_CHANGES
#define ATOMIC_BLOCK(state) for (AtomicSnapshot snapshot; snapshot.active; snapshot.active = false)
struct QuietSerial {
    std::string output;
    template<typename... Args> void printf(const char *format, Args... args) {
        char buffer[512]; std::snprintf(buffer, sizeof(buffer), format, args...);
        output += buffer;
    }
    template<typename... Args> void println(Args...) {}
    void println(const char *text) { output += text; output += "\\n"; }
    void write(int character) { output += static_cast<char>(character); }
} Serial;
#define Serial_printf Serial.printf
#define Serial_println Serial.println
#define Debug_printf Serial.printf
struct MIDIDeviceBase {
    struct TransmitDiagnosticInfo {
        uint32_t timer_callbacks = 0, submissions = 0, completions = 0, errors = 0;
        uint16_t queued = 0;
        uint8_t realtime_queued = 0, buffer1_count = 0, buffer2_count = 0, packet_capacity = 0;
    } tx_diagnostics;
    uint32_t packed_id = 0;
    uint32_t drops = 0, realtime_drops = 0;
    uint16_t pending = 0, realtime_pending = 0;
    bool queue_ready = true;
    uint16_t idVendor() { return packed_id >> 16; }
    uint16_t idProduct() { return packed_id & 0xFFFF; }
    const char *manufacturer() const { return "test"; }
    const char *product() const { return "test"; }
    bool hasTransmitQueueStorage() const { return queue_ready; }
    uint16_t getTransmitPendingCount() const { return pending; }
    uint16_t getRealtimeTransmitPendingCount() const { return realtime_pending; }
    uint32_t getTransmitDropCount() const { return drops; }
    uint32_t getRealtimeTransmitDropCount() const { return realtime_drops; }
    TransmitDiagnosticInfo getTransmitDiagnosticInfo() const { return tx_diagnostics; }
    uint32_t getTransmitErrorCount() const { return tx_diagnostics.errors; }
};
struct USBHub {
    struct PortDiagnosticInfo {
        uint32_t status = 0;
        uint16_t vid = 0, pid = 0;
        uint8_t state = 0, address = 0, enumeration_state = 0;
        bool has_device = false;
    };
    struct HubDiagnosticInfo {
        uint32_t change_bits = 0;
        uint8_t port_count = 0, reset_port = 0;
        bool change_pipe_present = false;
        PortDiagnosticInfo ports[7] = {};
    } diagnostics;
    uint32_t packed_id = 0;
    uint16_t idVendor() { return packed_id >> 16; }
    uint16_t idProduct() { return packed_id & 0xFFFF; }
    HubDiagnosticInfo getPortDiagnostics() const { return diagnostics; }
};
static USBHub hub1, hub2, hub3, hub4;
struct DeviceBehaviourUSBBase {
    MIDIDeviceBase *device = nullptr;
    uint32_t expected_id = 0;
    unsigned connects = 0, disconnects = 0;
    bool is_connected() const { return device != nullptr; }
    bool matches_identifiers(uint32_t packed_id) const { return expected_id == packed_id; }
    void connect_device(MIDIDeviceBase *connected) {
        assert(interrupts_enabled); device = connected; ++connects;
    }
    void disconnect_device() {
        assert(interrupts_enabled); device = nullptr; ++disconnects;
    }
};
struct usb_midi_slot {
    uint16_t vid = 0, pid = 0;
    uint32_t packed_id = 0;
    MIDIDeviceBase *device = nullptr;
    DeviceBehaviourUSBBase *behaviour = nullptr;
};
static MIDIDeviceBase devices[NUM_USB_MIDI_DEVICES];
static usb_midi_slot usb_midi_slots[NUM_USB_MIDI_DEVICES];
struct USBHost {
${extract('../my_libraries/USBHost_t36/USBHost_t36.h', '    enum class DiagnosticError', '    static void begin();')}
    static bool halted;
    static bool corrupt_pool;
    static volatile HostFault host_fault;
    static volatile bool cleanup_stop_confirmed;
    #if USBHOST_T36_ENABLE_DIAGNOSTICS
    static DiagnosticInfo diagnostic;
    static DiagnosticProgress previous_progress;
    static ControllerDiagnosticInfo controller;
    static PipeDiagnosticInfo pipe_diagnostics[2];
    static uint32_t pipe_count;
    #endif
    static bool hadSystemError() { return halted; }
    static void countFree(uint32_t &devices, uint32_t &pipes, uint32_t &transfers, uint32_t &strings) {
        devices = 29; pipes = corrupt_pool ? UINT32_MAX : 66; transfers = 145; strings = 23;
    }
};
bool USBHost::halted = false;
bool USBHost::corrupt_pool = false;
volatile USBHost::HostFault USBHost::host_fault = USBHost::HostFault::None;
volatile bool USBHost::cleanup_stop_confirmed = false;
${extract('../my_libraries/USBHost_t36/ehci.cpp', 'const char *USBHost::getHostFaultReason()', '\nbool USBHost::stop_controller_for_cleanup(')}
static USBHost::DiagnosticPhase recorded_phase = USBHost::DiagnosticPhase::None;
#if USBHOST_T36_ENABLE_DIAGNOSTICS
USBHost::DiagnosticInfo USBHost::diagnostic = {};
USBHost::DiagnosticProgress USBHost::previous_progress = {};
USBHost::ControllerDiagnosticInfo USBHost::controller = {};
USBHost::ControllerDiagnosticInfo USBHost::getControllerDiagnosticInfo() { return controller; }
USBHost::PipeDiagnosticInfo USBHost::pipe_diagnostics[2] = {};
uint32_t USBHost::pipe_count = 0;
uint32_t USBHost::getAsyncPipeDiagnostics(PipeDiagnosticInfo *info, uint32_t capacity) {
    const uint32_t count = pipe_count < capacity ? pipe_count : capacity;
    for (uint32_t index = 0; index < count; ++index) info[index] = pipe_diagnostics[index];
    return count;
}
USBHost::DiagnosticProgress USBHost::getPreviousDiagnosticProgress() { return previous_progress; }
void USBHost::setDiagnosticPhase(DiagnosticPhase phase) { recorded_phase = phase; }
USBHost::DiagnosticInfo USBHost::getDiagnosticInfo() { return diagnostic; }
void USBHost::clearDiagnosticInfo() { diagnostic = DiagnosticInfo{}; }
#endif
${extract('src/main.cpp', '#if defined(ENABLE_USB) && USBHOST_T36_ENABLE_DIAGNOSTICS', '\n#ifdef ENABLE_USBSERIAL')}
void check_marker_phase_names() {
    ${markerCalls.join('\n    ')}
}
struct SimulatedStall {};
struct TestUSBTask {
    bool stall = false;
    void Task() {
        assert(!interrupts_enabled);
        if (stall) throw SimulatedStall{};
    }
} Usb;
void run_usb_task_slice() {
${extract('src/main.cpp', '    RECORD_USB_PHASE(USBTask);', '    if (debug_flag) { Serial_println(F("just did Usb.Task()"))')}
}
static bool debug_flag = false, clock_stall = false;
static bool cv_clock_stall = false, cv_receive_stall = false, menu_ticks_stall = false;
static bool playing = true;
static uint32_t ticks = 24;
#define Serial_flush() do {} while (0)
static bool update_clock_ticks() {
    assert(recorded_phase == USBHost::DiagnosticPhase::ClockTicks);
    if (clock_stall) throw SimulatedStall{};
    return true;
}
static bool take_cv_clock_tick(uint32_t &observed_at_us) {
    assert(!interrupts_enabled && recorded_phase == USBHost::DiagnosticPhase::CVClock);
    if (cv_clock_stall) throw SimulatedStall{};
    observed_at_us = 500;
    return true;
}
constexpr int CLOCK_EXTERNAL_CV = 1;
static void clock_receive_external_pulse_at(int source, uint32_t observed_at_us) {
    assert(interrupts_enabled && recorded_phase == USBHost::DiagnosticPhase::CVClock);
    assert(source == CLOCK_EXTERNAL_CV && observed_at_us == 500);
    if (cv_receive_stall) throw SimulatedStall{};
}
struct TestMenuTicks {
    unsigned calls = 0;
    void update_ticks(uint32_t observed_ticks) {
        assert(interrupts_enabled && recorded_phase == USBHost::DiagnosticPhase::MenuTicks);
        assert(observed_ticks == ticks);
        ++calls;
        if (menu_ticks_stall) throw SimulatedStall{};
    }
} menu_ticks;
static TestMenuTicks *menu = &menu_ticks;
#define USE_UCLOCK
#define ENABLE_CLOCK_INPUT_CV
#define ENABLE_SCREEN
bool run_clock_update_slice() {
${extract('src/main.cpp', '  RECORD_USB_PHASE(ClockUpdate);', '\n  if (!playing ||')}
    return ticked;
}
#undef USE_UCLOCK
#undef ENABLE_CLOCK_INPUT_CV
#undef ENABLE_SCREEN
#if USBHOST_T36_ENABLE_DIAGNOSTICS
${extract('../my_libraries/USBHost_t36/diagnostics.cpp', 'const char *USBHost::diagnosticPhaseName(', 'USBHost::DiagnosticInfo USBHost::getDiagnosticInfo()')}
${extract('../my_libraries/USBHost_t36/diagnostics.cpp', 'const char *USBHost::diagnosticErrorName(', '\n#endif')}
#endif
constexpr int FILE_WRITE = 1, FILE_READ = 0;
struct File {
    static std::string output;
    static unsigned closes;
    static bool write_error;
    bool valid = false;
    size_t offset = 0;
    explicit File(bool opened) : valid(opened) {}
    explicit operator bool() const { return valid; }
    template<typename... Args> void printf(const char *format, Args... args) {
        char buffer[512]; std::snprintf(buffer, sizeof(buffer), format, args...);
        output += buffer;
    }
    void println(const char *text) { output += text; output += "\\n"; }
    bool getWriteError() const { return write_error; }
    void close() { ++closes; }
    bool available() const { return offset < output.size(); }
    int read() { return static_cast<unsigned char>(output[offset++]); }
};
std::string File::output;
unsigned File::closes = 0;
bool File::write_error = false;
struct TestSD {
    bool present = true, open_ok = true;
    unsigned opens = 0;
    bool mediaPresent() const { return present; }
    File open(const char *filename, int mode) {
        assert(std::string(filename) == "usb_debug.log");
        assert(mode == FILE_WRITE || mode == FILE_READ);
        ++opens; return File(open_ok);
    }
    bool remove(const char *filename) {
        assert(std::string(filename) == "usb_debug.log");
        File::output.clear(); return true;
    }
} SD;
#ifndef USB_DIAGNOSTICS_TEST_NO_SD
#define ENABLE_SD
#endif
namespace storage {
${extract('src/storage.cpp', '  bool log_usb_diagnostics_after_reset(', '\n  void log_crashreport()').replace(/\n  #endif\s*$/, '')}
}
${extract('src/usb/multi_usb_handlers.cpp', 'void dump_usb_diagnostics()', 'static void disconnect_usb_midi_slot(')}
struct Coord { int x, y; };
struct TestDisplay {
    int cursor_y = 0;
    std::string output;
    void setCursor(int, int y) { cursor_y = y; }
    void setTextSize(int) {}
    int getCursorY() const { return cursor_y; }
    void append(const char *text) {
        output += text;
        for (const char *character = text; *character; ++character)
            if (*character == '\\n') cursor_y += 8;
    }
    template<typename... Args> void printf(const char *format, Args... args) {
        char buffer[512]; std::snprintf(buffer, sizeof(buffer), format, args...);
        append(buffer);
    }
    void println(const char *text) { append(text); append("\\n"); }
} display;
static TestDisplay *tft = &display;
struct MenuItem {
    struct { bool selectable = true; } flags;
    explicit MenuItem(const char *) {}
    virtual ~MenuItem() = default;
    virtual int display(Coord, bool, bool) { return 0; }
    void header(const char *title, Coord, bool, bool) { tft->println(title); }
    void colours(bool) {}
};
${extract('include/mymenu/menu_usb.h', '    class USBHostDebugPanel', '\n#endif')}
struct DeviceBehaviourManager {
    std::vector<DeviceBehaviourUSBBase *> *behaviours_usb = nullptr;
${connect}
};
static DeviceBehaviourManager manager;
static DeviceBehaviourManager *behaviour_manager = &manager;
${handlers}
static constexpr uint32_t id_apcmini = 0x09E80028;
static constexpr uint32_t id_beatstep = 0x1C750206;
static constexpr uint32_t id_midilights = 0x13371337;
static DeviceBehaviourUSBBase apcmini, beatstep, midilights;
static std::vector<DeviceBehaviourUSBBase *> behaviours;
static unsigned tests = 0;
static void reset() {
    apcmini = DeviceBehaviourUSBBase{}; apcmini.expected_id = id_apcmini;
    beatstep = DeviceBehaviourUSBBase{}; beatstep.expected_id = id_beatstep;
    midilights = DeviceBehaviourUSBBase{}; midilights.expected_id = id_midilights;
    behaviours = {&apcmini, &beatstep, &midilights}; manager.behaviours_usb = &behaviours;
    for (unsigned port = 0; port < NUM_USB_MIDI_DEVICES; ++port) {
        devices[port] = MIDIDeviceBase{}; usb_midi_slots[port] = usb_midi_slot{};
        usb_midi_slots[port].device = &devices[port];
    }
    interrupts_enabled = true;
}
static void require_binding(unsigned port, DeviceBehaviourUSBBase &behaviour) {
    assert(usb_midi_slots[port].behaviour == &behaviour);
    assert(behaviour.device == &devices[port]);
    assert(usb_midi_slots[port].packed_id == devices[port].packed_id);
    assert(interrupts_enabled);
}
int main() {
    #if USBHOST_T36_ENABLE_DIAGNOSTICS
    run_usb_task_slice();
    assert(recorded_phase == USBHost::DiagnosticPhase::Loop && interrupts_enabled); ++tests;
    Usb.stall = true;
    bool stalled = false;
    try { run_usb_task_slice(); } catch (const SimulatedStall &) { stalled = true; }
    assert(stalled && recorded_phase == USBHost::DiagnosticPhase::USBTask); ++tests;
    assert(run_clock_update_slice() && recorded_phase == USBHost::DiagnosticPhase::Loop); ++tests;
    clock_stall = true; stalled = false;
    try { run_clock_update_slice(); } catch (const SimulatedStall &) { stalled = true; }
    assert(stalled && recorded_phase == USBHost::DiagnosticPhase::ClockTicks); ++tests;
    clock_stall = false; cv_clock_stall = true; stalled = false;
    try { run_clock_update_slice(); } catch (const SimulatedStall &) { stalled = true; }
    assert(stalled && interrupts_enabled && recorded_phase == USBHost::DiagnosticPhase::CVClock); ++tests;
    cv_clock_stall = false; cv_receive_stall = true; stalled = false;
    try { run_clock_update_slice(); } catch (const SimulatedStall &) { stalled = true; }
    assert(stalled && recorded_phase == USBHost::DiagnosticPhase::CVClock); ++tests;
    cv_receive_stall = false; menu_ticks_stall = true; stalled = false;
    try { run_clock_update_slice(); } catch (const SimulatedStall &) { stalled = true; }
    assert(stalled && recorded_phase == USBHost::DiagnosticPhase::MenuTicks); ++tests;
    menu_ticks_stall = false; playing = false;
    const unsigned before_menu_ticks = menu_ticks.calls;
    assert(run_clock_update_slice() && recorded_phase == USBHost::DiagnosticPhase::Loop);
    assert(menu_ticks.calls == before_menu_ticks); ++tests;
    #else
    check_marker_phase_names();
    run_usb_task_slice();
    assert(recorded_phase == USBHost::DiagnosticPhase::None && interrupts_enabled); ++tests;
    #endif
    reset(); devices[0].packed_id = id_apcmini; update_usb_midi_device_connections();
    require_binding(0, apcmini); ++tests;
    devices[0].packed_id = 0; update_usb_midi_device_connections();
    assert(!apcmini.is_connected() && usb_midi_slots[0].behaviour == nullptr); ++tests;

    reset(); devices[0].packed_id = id_apcmini; update_usb_midi_device_connections();
    devices[0].packed_id = 0; update_usb_midi_device_connections();
    devices[1].packed_id = id_apcmini; update_usb_midi_device_connections();
    require_binding(1, apcmini);
    devices[0].packed_id = id_beatstep; update_usb_midi_device_connections();
    require_binding(0, beatstep); require_binding(1, apcmini); ++tests;

    reset(); devices[1].packed_id = id_apcmini; update_usb_midi_device_connections();
    usb_midi_slots[0].behaviour = &apcmini;
    setup_usb_midi_device(0, 0);
    assert(usb_midi_slots[0].behaviour == nullptr);
    require_binding(1, apcmini); assert(apcmini.disconnects == 0); ++tests;

    reset(); devices[2].packed_id = id_apcmini; update_usb_midi_device_connections();
    devices[2].packed_id = 0; devices[0].packed_id = id_apcmini;
    update_usb_midi_device_connections(); require_binding(0, apcmini);
    assert(usb_midi_slots[2].behaviour == nullptr); ++tests;

    reset(); devices[0].packed_id = id_apcmini; update_usb_midi_device_connections();
    devices[0].packed_id = 0; devices[2].packed_id = id_apcmini;
    update_usb_midi_device_connections(); require_binding(2, apcmini);
    assert(usb_midi_slots[0].behaviour == nullptr); ++tests;

    reset(); devices[0].packed_id = id_apcmini; devices[1].packed_id = id_beatstep;
    devices[2].packed_id = id_midilights; update_usb_midi_device_connections();
    devices[0].packed_id = id_beatstep; devices[1].packed_id = id_apcmini;
    update_usb_midi_device_connections();
    require_binding(0, beatstep); require_binding(1, apcmini); require_binding(2, midilights);
    assert(midilights.connects == 1 && midilights.disconnects == 0); ++tests;

    const unsigned connected_before = apcmini.connects;
    update_usb_midi_device_connections();
    require_binding(1, apcmini); assert(apcmini.connects == connected_before); ++tests;

    for (unsigned cycle = 0; cycle < 10; ++cycle) {
        for (auto &device : devices) device.packed_id = 0;
        update_usb_midi_device_connections();
        for (auto &slot : usb_midi_slots) assert(slot.behaviour == nullptr && slot.packed_id == 0);
        assert(!apcmini.is_connected() && !beatstep.is_connected() && !midilights.is_connected());
        const unsigned apc_port = cycle % 3, beat_port = (cycle + 1) % 3, light_port = (cycle + 2) % 3;
        devices[apc_port].packed_id = id_apcmini; devices[beat_port].packed_id = id_beatstep;
        devices[light_port].packed_id = id_midilights; update_usb_midi_device_connections();
        require_binding(apc_port, apcmini); require_binding(beat_port, beatstep); require_binding(light_port, midilights);
        ++tests;
    }
    reset(); devices[0].packed_id = 0x88889999; update_usb_midi_device_connections();
    assert(usb_midi_slots[0].packed_id == 0x88889999 && usb_midi_slots[0].behaviour == nullptr);
    assert(!apcmini.is_connected() && !beatstep.is_connected()); ++tests;
    #if USBHOST_T36_ENABLE_DIAGNOSTICS
    USBHostDebugPanel panel;
    assert(!panel.flags.selectable);
    assert(panel.display(Coord{0, 40}, false, false) < 240);
    assert(display.output.find("Free devices:29 pipes:66") != std::string::npos);
    assert(display.output.find("Error records:0") != std::string::npos); ++tests;
    Serial.output.clear(); dump_usb_diagnostics();
    assert(Serial.output.find("USB error device unavailable: no retained error event") != std::string::npos);
    assert(Serial.output.find("USB error device=0000:0000") == std::string::npos); ++tests;
    for (bool asynchronous : {false, true}) {
        USBHost::controller = {asynchronous ? 0x31u : 0x11u, asynchronous ? 0xc000u : 0x4000u,
                               0x01000007, 123, 0x20001000, 0x20002000, 0x1005, false};
        Serial.output.clear(); dump_usb_diagnostics();
        assert(Serial.output.find("intr=01000007 frame=123 async_hw=20001000 async_sw=20002000 port=00001005 enumeration_busy=0") != std::string::npos);
        assert(Serial.output.find(asynchronous ?
            "USB schedules run=1 halted=0 async_enable=1 async_running=1 periodic_enable=1 periodic_running=1" :
            "USB schedules run=1 halted=0 async_enable=0 async_running=0 periodic_enable=1 periodic_running=1") != std::string::npos);
        ++tests;
    }
    USBHost::pipe_count = 2;
    USBHost::pipe_diagnostics[0].address = 0x20002000;
    USBHost::pipe_diagnostics[0].horizontal_link = 0x20003002;
    USBHost::pipe_diagnostics[0].capabilities1 = 0x00408006;
    auto &bulk_pipe = USBHost::pipe_diagnostics[1];
    bulk_pipe.address = 0x20003000;
    bulk_pipe.horizontal_link = 0x20002002;
    bulk_pipe.capabilities1 = 0x00400206;
    bulk_pipe.vid = 0x2e8a; bulk_pipe.pid = 0x10c1;
    bulk_pipe.device_address = 6; bulk_pipe.hub = 2; bulk_pipe.port = 2;
    bulk_pipe.type = 2;
    bulk_pipe.current = 0x20004000; bulk_pipe.next = 0x20004020;
    bulk_pipe.token = 0x80000082; bulk_pipe.dummy = 0x20004040;
    bulk_pipe.pending_count = 2; bulk_pipe.active_count = 1;
    bulk_pipe.first_pending = 0x20004000; bulk_pipe.first_pending_token = 0x8000;
    bulk_pipe.first_pending_next = 0x20004020; bulk_pipe.next_is_pending = true;
    Serial.output.clear(); dump_usb_diagnostics();
    assert(Serial.output.find("USB async pipes=2 limit=32") != std::string::npos);
    assert(Serial.output.find("USB async pipe=20002000 link=20003002 head=1") != std::string::npos);
    assert(Serial.output.find("USB async pipe=20003000 link=20002002 head=0 id=2E8A:10C1 address=6 hub=2 port=2 speed=0 type=2 direction=0 ep=2") != std::string::npos);
    assert(Serial.output.find("current=20004000 next=20004020 token=80000082 dummy=20004040") != std::string::npos); ++tests;
    assert(Serial.output.find("USB async work pipe=20002000 submitted=0 completed=0 errors=0 pending=0 active=0 first=00000000 first_token=00000000 first_next=00000000 next_pending=0 truncated=0") != std::string::npos);
    assert(Serial.output.find("USB async work pipe=20003000 submitted=0 completed=0 errors=0 pending=2 active=1 first=20004000 first_token=00008000 first_next=20004020 next_pending=1 truncated=0") != std::string::npos); ++tests;
    devices[0].tx_diagnostics.timer_callbacks = 13;
    devices[0].tx_diagnostics.submissions = 9;
    devices[0].tx_diagnostics.completions = 7;
    devices[0].tx_diagnostics.errors = 1;
    devices[0].tx_diagnostics.buffer1_count = 4;
    devices[0].tx_diagnostics.buffer2_count = 8;
    devices[0].tx_diagnostics.packet_capacity = 16;
    Serial.output.clear(); dump_usb_diagnostics();
    assert(Serial.output.find("USB MIDI tx port=0 buffers=4/8 capacity=16 timer=13 submitted=9 completed=7 errors=1") != std::string::npos); ++tests;
    hub1.packed_id = 0x05e30608;
    hub1.diagnostics.port_count = 2;
    hub1.diagnostics.change_pipe_present = true;
    hub1.diagnostics.change_bits = 2;
    hub1.diagnostics.ports[0].status = 0x00000103;
    hub1.diagnostics.ports[0].state = 9;
    hub1.diagnostics.ports[0].has_device = true;
    hub1.diagnostics.ports[0].vid = 0x2e8a;
    hub1.diagnostics.ports[0].pid = 0x10c1;
    hub1.diagnostics.ports[0].address = 6;
    hub1.diagnostics.ports[0].enumeration_state = 15;
    Serial.output.clear(); dump_usb_diagnostics();
    assert(Serial.output.find("USB hub slot=0 id=05E3:0608 ports=2 change_pipe=1 change_bits=00000002 reset_port=0") != std::string::npos);
    assert(Serial.output.find("USB hub port=0:1 status=00000103 state=9 child=1 id=2E8A:10C1 address=6 enum=15") != std::string::npos); ++tests;
    bulk_pipe.next_is_pending = false; bulk_pipe.followup_truncated = true;
    Serial.output.clear(); dump_usb_diagnostics();
    assert(Serial.output.find("next_pending=0 truncated=1") != std::string::npos); ++tests;
    USBHost::pipe_count = 0;
    USBHost::halted = true;
    USBHost::diagnostic.count = 1;
    USBHost::diagnostic.error = USBHost::DiagnosticError::EnumerationTransfer;
    USBHost::diagnostic.previous_boot = true;
    USBHost::diagnostic.detail = 0x40;
    USBHost::previous_progress = {1234, USBHost::DiagnosticPhase::USBTask};
    devices[0].drops = 7; devices[0].realtime_drops = 2;
    devices[0].pending = 3; devices[0].queue_ready = false;
    usb_midi_slots[3].device = nullptr;
    display.output.clear();
    assert(panel.display(Coord{0, 40}, false, false) < 240);
    assert(display.output.find("EHCI: HALTED") != std::string::npos);
    assert(display.output.find("TX drops:7 RT:2") != std::string::npos);
    assert(display.output.find("Missing TX queues:1") != std::string::npos);
    assert(display.output.find("Enumeration USB error") != std::string::npos);
    assert(display.output.find("Detail:00000040") != std::string::npos); ++tests;
    assert(display.output.find("Prev: Usb.Task") != std::string::npos); ++tests;
    Serial.output.clear(); dump_usb_diagnostics();
    assert(Serial.output.find("previous boot") != std::string::npos);
    assert(Serial.output.find("dropped=7 realtime_dropped=2") != std::string::npos); ++tests;
    assert(Serial.output.find("previous boot phase=Usb.Task at=1234 ms") != std::string::npos); ++tests;
    USBHost::corrupt_pool = true;
    USBHost::diagnostic.error = USBHost::DiagnosticError::PipePoolCorruption;
    display.output.clear();
    assert(panel.display(Coord{0, 40}, false, false) < 240);
    assert(display.output.find("Free pools: CORRUPT") != std::string::npos);
    assert(display.output.find("Pipe free-list corrupt") != std::string::npos); ++tests;
    Serial.output.clear(); dump_usb_diagnostics();
    assert(Serial.output.find("free pools CORRUPT (counts unavailable)") != std::string::npos);
    assert(Serial.output.find("last=Pipe free-list corrupt") != std::string::npos); ++tests;
    USBHost::corrupt_pool = false;
    USBHost::diagnostic.error = USBHost::DiagnosticError::EnumerationTransfer;
    #else
    USBHostDebugPanel panel;
    assert(panel.display(Coord{0, 40}, false, false) < 240);
    assert(display.output.find("Free devices:29 pipes:66") != std::string::npos);
    assert(display.output.find("Detailed diagnostics disabled") != std::string::npos);
    Serial.output.clear(); dump_usb_diagnostics();
    assert(Serial.output.find("USB detailed diagnostics disabled") != std::string::npos);
    assert(Serial.output.find("USB MIDI port=0") != std::string::npos); ++tests;
    #endif
    assert(Serial.output.find("USB fault reason=none cleanup_stop_confirmed=0") != std::string::npos); ++tests;
    for (const auto fault : {USBHost::HostFault::AsyncAdvanceTimeout, USBHost::HostFault::PeriodicFrameTimeout}) {
        for (const bool stopped : {false, true}) {
            USBHost::host_fault = fault;
            USBHost::cleanup_stop_confirmed = stopped;
            Serial.output.clear(); dump_usb_diagnostics();
            const std::string reason = fault == USBHost::HostFault::AsyncAdvanceTimeout
                ? "async-doorbell-timeout" : "periodic-frame-timeout-after-restart";
            const std::string expected = "USB fault reason=" + reason + " cleanup_stop_confirmed=" + (stopped ? "1" : "0");
            assert(Serial.output.find(expected) != std::string::npos); ++tests;
        }
    }
    USBHost::host_fault = USBHost::HostFault::None;
    USBHost::cleanup_stop_confirmed = false;
#if defined(ENABLE_SD) && USBHOST_T36_ENABLE_DIAGNOSTICS
    File::output = "older snapshot\\n";
    assert(storage::log_usb_diagnostics_after_reset(true));
    assert(File::output.find("older snapshot\\n") == 0);
    assert(File::output.find("Watchdog reset: yes") != std::string::npos);
    assert(File::output.find("Previous phase: Usb.Task at 1234 ms") != std::string::npos);
    assert(File::output.find("last=Enumeration USB error") != std::string::npos);
    assert(File::closes == 1); ++tests;
    USBHost::diagnostic = {};
    USBHost::previous_progress = {5678, USBHost::DiagnosticPhase::RetentionTest};
    assert(storage::log_usb_diagnostics_after_reset(false));
    assert(File::output.find("Previous phase: Retention test at 5678 ms") != std::string::npos);
    assert(File::output.find("No retained USB error event.") != std::string::npos); ++tests;
    USBHost::previous_progress = {};
    const unsigned opens_before = SD.opens;
    assert(!storage::log_usb_diagnostics_after_reset(false) && SD.opens == opens_before); ++tests;
    assert(storage::log_usb_diagnostics_after_reset(true));
    assert(File::output.find("Previous phase: No retained phase") != std::string::npos); ++tests;
    SD.present = false;
    assert(!storage::log_usb_diagnostics_after_reset(true)); ++tests;
    SD.present = true; SD.open_ok = false;
    assert(!storage::log_usb_diagnostics_after_reset(true)); ++tests;
    SD.open_ok = true; File::write_error = true;
    assert(!storage::log_usb_diagnostics_after_reset(true)); ++tests;
    File::write_error = false;
    Serial.output.clear(); storage::dump_usb_diagnostic_log();
    assert(Serial.output == File::output); ++tests;
    storage::clear_usb_diagnostic_log(); assert(File::output.empty()); ++tests;
#else
    File::output = "untouched";
    const unsigned opens_before_disabled_log = SD.opens;
    assert(!storage::log_usb_diagnostics_after_reset(true));
    assert(SD.opens == opens_before_disabled_log && File::output == "untouched"); ++tests;
#ifndef ENABLE_SD
    storage::dump_usb_diagnostic_log(); storage::clear_usb_diagnostic_log();
    assert(SD.opens == 0 && File::output == "untouched"); ++tests;
#endif
#endif
    std::cout << "PASS: " << tests << " USB MIDI rebinding/diagnostic cases ("
#ifdef ENABLE_SD
              << "SD enabled"
#else
              << "SD disabled"
#endif
              << ", diagnostics=" << USBHOST_T36_ENABLE_DIAGNOSTICS << ")\\n";
}
`;

const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'usb-midi-reconnect-'));
try {
    for (const diagnostics of [false, true]) for (const withSD of [true, false]) {
        const executable = path.join(directory, `test-${withSD ? 'sd' : 'no-sd'}-${diagnostics ? 'diag' : 'quiet'}${process.platform === 'win32' ? '.exe' : ''}`);
        const build = spawnSync(process.env.CXX || 'g++', [
            `-DUSBHOST_T36_ENABLE_DIAGNOSTICS=${diagnostics ? 1 : 0}`,
            '-std=c++11', '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined',
            ...(withSD ? [] : ['-DUSB_DIAGNOSTICS_TEST_NO_SD']),
            '-x', 'c++', '-', '-o', executable,
        ], { input: source, encoding: 'utf8' });
        if (build.error) throw build.error;
        if (build.status !== 0) throw new Error(build.stderr || 'Host compile failed');
        const test = spawnSync(executable, [], { encoding: 'utf8', timeout: 10000 });
        process.stdout.write(test.stdout || '');
        process.stderr.write(test.stderr || '');
        if (test.error) throw test.error;
        if (test.status !== 0) throw new Error(`Host test failed: ${test.status} ${test.signal}`);
    }
} finally {
    fs.rmSync(directory, { recursive: true, force: true });
}