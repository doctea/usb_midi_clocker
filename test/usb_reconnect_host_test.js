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
const source = `
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
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
    template<typename... Args> void printf(Args...) {}
    template<typename... Args> void println(Args...) {}
} Serial;
#define Serial_printf Serial.printf
#define Serial_println Serial.println
#define Debug_printf Serial.printf
struct MIDIDeviceBase {
    uint32_t packed_id = 0;
    uint16_t idVendor() const { return packed_id >> 16; }
    uint16_t idProduct() const { return packed_id & 0xFFFF; }
    const char *manufacturer() const { return "test"; }
    const char *product() const { return "test"; }
};
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
    std::cout << "PASS: " << tests << " USB MIDI rebinding regression cases\\n";
}
`;

const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'usb-midi-reconnect-'));
try {
    const executable = path.join(directory, process.platform === 'win32' ? 'test.exe' : 'test');
    const build = spawnSync(process.env.CXX || 'g++', [
        '-std=c++11', '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined',
        '-x', 'c++', '-', '-o', executable,
    ], { input: source, encoding: 'utf8' });
    if (build.error) throw build.error;
    if (build.status !== 0) throw new Error(build.stderr || 'Host compile failed');
    const test = spawnSync(executable, [], { encoding: 'utf8', timeout: 10000 });
    process.stdout.write(test.stdout || '');
    process.stderr.write(test.stderr || '');
    if (test.error) throw test.error;
    if (test.status !== 0) throw new Error(`Host test failed: ${test.status} ${test.signal}`);
} finally {
    fs.rmSync(directory, { recursive: true, force: true });
}