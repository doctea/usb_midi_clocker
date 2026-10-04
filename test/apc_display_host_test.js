const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const library = process.env.USBHOST_T36_DIR || path.resolve(__dirname, '../../my_libraries/USBHost_t36');
function extract(filename, startMarker, endMarker) {
    const content = fs.readFileSync(filename, 'utf8');
    const start = content.indexOf(startMarker);
    const end = content.indexOf(endMarker, start);
    if (start < 0 || end < 0) throw new Error(`Cannot locate ${startMarker} in ${filename}`);
    return content.slice(start, end);
}
const tryNoteOn = extract(path.join(library, 'USBHost_t36.h'), '    bool trySendNoteOn(', '    void sendPolyPressure(');
const txComplete = extract(path.join(library, 'midi.cpp'), 'void MIDIDeviceBase::tx_data(', 'void MIDIDeviceBase::disconnect()');
const tx = extract(path.join(library, 'midi.cpp'), 'void MIDIDeviceBase::disconnect()', 'void MIDIDeviceBase::send_sysex_buffer_has_term(');
const queueConfig = extract(path.join(library, 'USBHost_t36.h'), '#ifndef USBHOST_MIDI_TX_QUEUE_SIZE', '#if !defined(__MK66FX1M0__)');
const queueFields = extract(path.join(library, 'USBHost_t36.h'), '    static_assert(USBHOST_MIDI_TX_QUEUE_SIZE', '    Pipe_t *rxpipe;');
const sendNow = extract(path.join(library, 'USBHost_t36.h'), '    void send_now(void)', '    bool read(uint8_t channel');
const noteMethods = extract(path.join(library, 'USBHost_t36.h'), '    void sendNoteOff(', '    bool trySendNoteOn(');
const sendMethods = extract(path.join(library, 'USBHost_t36.h'), '    void sendPolyPressure(', '    void send_now(void)');
const sysexMethods = extract(path.join(library, 'midi.cpp'), 'void MIDIDeviceBase::send_sysex_buffer_has_term(', 'bool MIDIDeviceBase::read(');
const display = extract(path.resolve(__dirname, '../src/midi/midi_apcmini_display.cpp'), 'apc_gate_page_t apc_gate_page =', '// x,y addressing for apcmini grid');
const startup = extract(path.resolve(__dirname, '../src/usb/multi_usb_handlers.cpp'), 'void setup_multi_usb()', '\n#endif');
const source = `
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>
${queueConfig}
static bool irq_enabled = true;
static unsigned disables = 0;
static bool __irq_enabled() { return irq_enabled; }
static void __disable_irq() { irq_enabled = false; ++disables; }
static void __enable_irq() { irq_enabled = true; }
#ifdef USBHOST_MIDI_TX_QUEUE_ALLOCATOR
static bool allocation_available = true;
static unsigned allocation_calls = 0;
static std::vector<void *> allocations;
static void *test_allocate(size_t bytes) {
    assert(irq_enabled); ++allocation_calls;
    if (!allocation_available) return nullptr;
    void *storage = std::malloc(bytes);
    if (storage) allocations.push_back(storage);
    return storage;
}
#endif
struct AtomicSnapshot {
    bool active = true;
    bool saved = irq_enabled;
    AtomicSnapshot() { __disable_irq(); }
    ~AtomicSnapshot() { irq_enabled = saved; }
};
#define ATOMIC_BLOCK(state) for (AtomicSnapshot snapshot; snapshot.active; snapshot.active = false)
#define MIDI_NUM_NOTES 128
static uint32_t fake_micros = 0;
static uint32_t micros() { return fake_micros; }
static bool is_valid_note(int8_t note) { return note >= 0; }
enum apc_gate_page_t { CLOCKS };
struct Pipe_t {};
struct Transfer_t { const void *buffer; };
template<typename... Args> static void println(Args...) {}
template<typename... Args> static void print(Args...) {}
static void print_hexbytes(const void *, uint32_t) {}
struct USBDriverTimer {
    bool active = false;
    unsigned delay = 0;
    void stop() { active = false; }
    void start(unsigned value) { active = true; delay = value; }
};
struct MIDIDeviceBase {
    Pipe_t pipe;
    Pipe_t *txpipe = &pipe, *rxpipe = &pipe;
    uint32_t tx_buffer1[128] = {}, tx_buffer2[128] = {};
    uint16_t tx_size = 16;
    volatile uint8_t tx1_count = 0, tx2_count = 0;
    bool rx_packet_queued = true;
    USBDriverTimer txtimer;
    void disconnect();
    void write_packed(uint32_t data);
    bool try_write_packed(uint32_t data);
    bool try_write_buffer(uint32_t data);
    void drain_tx_queue();
    void flush_tx_buffers();
    void tx_data(const Transfer_t *transfer);
    void timer_event(USBDriverTimer *timer);
    void send_sysex_buffer_has_term(const uint8_t *data, uint32_t length, uint8_t cable);
    void send_sysex_add_term_bytes(const uint8_t *data, uint32_t length, uint8_t cable);
${queueFields}
${noteMethods}
${tryNoteOn}
${sendMethods}
${sendNow}
};
static bool queue_available = true;
static unsigned queue_attempts = 0;
static std::vector<std::vector<uint32_t>> packets;
static bool queue_Data_Transfer(Pipe_t *, uint32_t *buffer, uint32_t bytes, MIDIDeviceBase *) {
    assert(!irq_enabled); ++queue_attempts;
    if (!queue_available) return false;
    packets.emplace_back(buffer, buffer + bytes / 4); return true;
}
${tx}
${txComplete}
${sysexMethods}
struct APCBehaviour {
    MIDIDeviceBase *device = nullptr;
    bool is_connected() { return device != nullptr; }
};
static APCBehaviour behaviour;
static APCBehaviour *behaviour_apcmini = &behaviour;
${display}
static unsigned tests = 0;
static unsigned usb_starts = 0, usb_tasks = 0;
struct QuietSerial {
    template<typename... Args> void print(Args...) {}
    template<typename... Args> void printf(Args...) {}
    template<typename... Args> void println(Args...) {}
} Serial;
#define F(value) value
#define Serial_flush() ((void)0)
#define NUM_USB_MIDI_DEVICES 12
static void tft_print(const char *) {}
static void delay(unsigned) {}
static MIDIDeviceBase startup_devices[NUM_USB_MIDI_DEVICES];
struct TestSlot { MIDIDeviceBase *device; };
static TestSlot usb_midi_slots[NUM_USB_MIDI_DEVICES];
struct TestUSBHost {
    void begin() {
        for (const auto &slot : usb_midi_slots) assert(slot.device->hasTransmitQueueStorage());
        ++usb_starts;
    }
    void Task() { ++usb_tasks; }
} Usb;
${startup}
static void reset(MIDIDeviceBase &device) {
    device = MIDIDeviceBase{}; device.txpipe = &device.pipe; device.rxpipe = &device.pipe;
    irq_enabled = true; disables = 0; queue_available = true; queue_attempts = 0; packets.clear();
    fake_micros = 0; behaviour_apcmini = &behaviour; behaviour.device = &device;
    assert(device.beginTransmitQueues() && device.hasTransmitQueueStorage());
    apcdisplay_initialise_last_sent();
}
static void timer_tick(MIDIDeviceBase &device) {
    irq_enabled = false; device.txtimer.active = false;
    device.timer_event(&device.txtimer); irq_enabled = true;
}
static void complete_pending(MIDIDeviceBase &device) {
    for (unsigned round = 0; device.getTransmitPendingCount() && round < 1000; ++round) {
        if (device.tx1_count == device.tx_size / 4) {
            Transfer_t completion{device.tx_buffer1}; device.tx_data(&completion);
        }
        if (device.tx2_count == device.tx_size / 4) {
            Transfer_t completion{device.tx_buffer2}; device.tx_data(&completion);
        }
        timer_tick(device);
    }
    assert(device.getTransmitPendingCount() == 0); timer_tick(device);
}
int main() {
    MIDIDeviceBase device;
    reset(device); device.txpipe = nullptr;
    assert(!device.trySendNoteOn(60, 5, 1) && irq_enabled && queue_attempts == 0); ++tests;
    reset(device); assert(device.trySendNoteOn(60, 5, 3, 2));
    assert(device.tx_buffer1[0] == 0x053C9229 && device.tx1_count == 1);
    assert(device.txtimer.active && device.txtimer.delay == 1500 && irq_enabled); ++tests;
    for (unsigned note = 1; note < 4; ++note) assert(device.trySendNoteOn(note, 1, 1));
    assert(device.tx1_count == 4 && packets.size() == 1 && packets[0].size() == 4); ++tests;
    for (unsigned note = 4; note < 8; ++note) assert(device.trySendNoteOn(note, 1, 1));
    const unsigned before = disables;
    assert(!device.trySendNoteOn(8, 1, 1) && disables <= before + 3);
    assert(device.tx2_count == 4 && packets.size() == 2 && irq_enabled); ++tests;
    irq_enabled = false; const unsigned protected_before = disables;
    device.write_packed(0x12345678);
    assert(!irq_enabled && disables <= protected_before + 4 && packets.size() == 2);
    assert(device.tx_queue_count == 1); ++tests;
    irq_enabled = true; Transfer_t completed{device.tx_buffer1}; device.tx_data(&completed);
    assert(device.tx_queue_count == 0 && device.tx_buffer1[0] == 0x12345678);
    assert(device.trySendNoteOn(8, 1, 1) && device.tx1_count == 2 && device.tx2_count == 4); ++tests;
    reset(device);
    for (unsigned note = 0; note < 3; ++note) assert(device.trySendNoteOn(note, 1, 1));
    queue_available = false;
    assert(!device.trySendNoteOn(3, 1, 1) && device.tx1_count == 3 && device.txtimer.active);
    assert(packets.empty() && irq_enabled); ++tests;
    timer_tick(device);
    assert(device.tx1_count == 3 && device.txtimer.active && packets.empty()); ++tests;
    queue_available = true; timer_tick(device);
    assert(device.tx1_count == 4 && packets.size() == 1 && packets[0].size() == 3);
    for (unsigned note = 0; note < 3; ++note) assert(packets[0][note] == (0x01009009 | (note << 16)));
    assert(device.trySendNoteOn(3, 1, 1) && device.tx2_count == 1); ++tests;
    queue_available = false; timer_tick(device);
    assert(device.tx1_count == 4 && device.tx2_count == 1 && device.txtimer.active); ++tests;
    queue_available = true; timer_tick(device);
    assert(device.tx2_count == 4 && packets.size() == 2 && packets[1].size() == 1); ++tests;
    device.disconnect();
    assert(!device.txpipe && !device.rxpipe && !device.txtimer.active);
    assert(device.tx1_count == 0 && device.tx2_count == 0 && !device.rx_packet_queued);
    assert(!device.trySendNoteOn(3, 1, 1)); ++tests;
    reset(device); device.tx_size = 512;
    assert(device.trySendNoteOn(0, 1, 1) && device.txtimer.delay == 200);
    for (unsigned note = 1; note < 128; ++note) assert(device.trySendNoteOn(note, 1, 1));
    assert(device.tx1_count == 128 && packets.size() == 1 && packets[0].size() == 128); ++tests;
    reset(device); device.write_packed(0x01234567);
    assert(device.tx1_count == 1 && device.tx_buffer1[0] == 0x01234567 && irq_enabled); ++tests;
    reset(device); irq_enabled = false;
    apcdisplay_sendNoteOn(60, 1, 1, false);
    apcdisplay_flush_pending();
    assert(!irq_enabled && device.tx1_count == 0 && apc_pending_notes[60].dirty);
    assert(apc_note_last_sent[60] == -1); ++tests;
    irq_enabled = true; fake_micros = 1000; apcdisplay_flush_pending();
    assert(apc_note_last_sent[60] == 1 && !apc_pending_notes[60].dirty && irq_enabled); ++tests;
    apcdisplay_sendNoteOn(60, 1, 1, false);
    assert(!apc_pending_notes[60].dirty); ++tests;
    apcdisplay_sendNoteOn(60, 1, 1, true);
    assert(apc_pending_notes[60].dirty);
    fake_micros += 1000; apcdisplay_flush_pending();
    assert(!apc_pending_notes[60].dirty && device.tx1_count == 2); ++tests;
    reset(device);
    apcdisplay_sendNoteOn(60, 1, 1, false); apcdisplay_sendNoteOff(60, 0, 1, false);
    fake_micros = 1000; apcdisplay_flush_pending();
    assert(device.tx_buffer1[0] == 0x003C9009 && apc_note_last_sent[60] == 0); ++tests;
    reset(device);
    for (unsigned note = 0; note < 12; ++note) apcdisplay_sendNoteOn(note, 1, 1, false);
    fake_micros = 1000; apcdisplay_flush_pending();
    assert(packets.size() == 1 && device.tx1_count == 4 && apc_pending_notes[4].dirty);
    apcdisplay_flush_pending();
    assert(packets.size() == 1 && device.tx2_count == 0); ++tests;
    fake_micros += 1000; apcdisplay_flush_pending();
    assert(packets.size() == 2 && device.tx2_count == 4);
    fake_micros += 1000; apcdisplay_flush_pending();
    assert(apc_note_last_sent[8] == -1 && apc_pending_notes[8].dirty && packets.size() == 2); ++tests;
    device.tx1_count = 0; fake_micros += 1000; apcdisplay_flush_pending();
    assert(packets.size() == 3 && apc_note_last_sent[11] == 1 && !apc_pending_notes[11].dirty); ++tests;
    reset(device); queue_available = false;
    for (unsigned note = 0; note < 4; ++note) apcdisplay_sendNoteOn(note, 1, 1, false);
    fake_micros = 1000; apcdisplay_flush_pending();
    assert(apc_note_last_sent[2] == 1 && apc_note_last_sent[3] == -1);
    assert(device.tx1_count == 3 && apc_pending_notes[3].dirty);
    queue_available = true; fake_micros += 1000; apcdisplay_flush_pending();
    assert(packets.size() == 1 && apc_note_last_sent[3] == 1 && !apc_pending_notes[3].dirty); ++tests;
    reset(device); apcdisplay_sendNoteOn(127, 5, 16, false);
    fake_micros = 1000; apcdisplay_flush_pending();
    assert(device.tx_buffer1[0] == 0x057F9F09 && apc_next_pending_note == 0); ++tests;
    reset(device);
    apcdisplay_sendNoteOn(-1, 1, 1, false); apcdisplay_sendNoteOn(0, -1, 1, false);
    apcdisplay_sendNoteOn(0, 1, 0, false); apcdisplay_sendNoteOn(0, 1, 17, false);
    behaviour_apcmini = nullptr; apcdisplay_sendNoteOn(0, 1, 1, false); apcdisplay_flush_pending();
    assert(device.tx1_count == 0 && !apc_pending_notes[0].dirty); ++tests;
    reset(device); apcdisplay_sendNoteOn(0, 1, 1, false); behaviour.device = nullptr;
    fake_micros = 1000; apcdisplay_flush_pending(); assert(apc_pending_notes[0].dirty);
    behaviour.device = &device; apcdisplay_initialise_last_sent();
    assert(!apc_pending_notes[0].dirty && apc_note_last_sent[0] == -1); ++tests;
    reset(device); fake_micros = 0xFFFFFF00; apcdisplay_initialise_last_sent();
    apcdisplay_sendNoteOn(0, 1, 1, false); fake_micros += 1000; apcdisplay_flush_pending();
    assert(apc_note_last_sent[0] == 1 && irq_enabled); ++tests;
    reset(device); irq_enabled = false;
    for (unsigned message = 0; message < 39; ++message) device.write_packed(message);
    assert(!irq_enabled && device.tx_queue_count == 31 && device.getTransmitDropCount() == 0);
    assert(device.getTransmitPendingCount() == 31); ++tests;
    device.write_packed(39); assert(device.tx_queue_count == 32); ++tests;
    irq_enabled = true;
    while (device.tx_queue_count) {
        Transfer_t completion{device.tx_buffer1}; device.tx_data(&completion);
        if (device.tx2_count == 4) { completion.buffer = device.tx_buffer2; device.tx_data(&completion); }
    }
    timer_tick(device);
    unsigned ordered = 0;
    for (const auto &packet : packets) for (uint32_t message : packet) assert(message == ordered++);
    assert(ordered == 40 && irq_enabled && device.getTransmitDropCount() == 0); ++tests;
    reset(device); irq_enabled = false;
    for (unsigned message = 0; message < 8 + MIDIDeviceBase::TX_QUEUE_SIZE + 2; ++message) device.write_packed(message);
    assert(device.tx_queue_count == MIDIDeviceBase::TX_QUEUE_SIZE && device.getTransmitDropCount() == 2);
    assert(disables < 1000 && !irq_enabled); ++tests;
    device.disconnect();
    assert(device.tx_queue_count == 0 && !device.txtimer.active);
    assert(device.getTransmitDropCount() == 2); ++tests;
    reset(device); queue_available = false;
    for (unsigned message = 0; message < 20; ++message) device.write_packed(message);
    assert(device.tx_queue_count == 17 && device.getTransmitDropCount() == 0);
    queue_available = true; timer_tick(device);
    assert(device.tx_queue_count < 17 && packets[0] == std::vector<uint32_t>({0, 1, 2, 3})); ++tests;
    reset(device); device.tx_size = 512;
    for (unsigned message = 0; message < 260; ++message) device.write_packed(message);
    assert(device.tx_queue_count == 4);
    Transfer_t large_completion{device.tx_buffer1}; device.tx_data(&large_completion);
    assert(device.tx_queue_count == 0 && device.tx1_count == 4 && irq_enabled); ++tests;
    reset(device); irq_enabled = false; device.write_packed(0x0000F80F);
    assert(!irq_enabled && packets.size() == 1 && packets[0] == std::vector<uint32_t>({0x0000F80F}));
    assert(device.tx1_count == 4 && device.tx_queue_count == 0 && device.tx_realtime_count == 0); ++tests;
    reset(device); device.tx_size = 512; device.write_packed(0x0000FA0F);
    assert(packets.size() == 1 && packets[0] == std::vector<uint32_t>({0x0000FA0F}));
    assert(device.tx1_count == 128 && device.getRealtimeTransmitDropCount() == 0); ++tests;
    reset(device);
    for (unsigned message = 0; message < 40; ++message) device.write_packed(message);
    device.write_packed(0x0000FA0F); device.write_packed(0x0000F80F); device.write_packed(0x0000FC0F);
    assert(device.tx_queue_count == 32 && device.tx_realtime_count == 3);
    assert(device.getTransmitPendingCount() == 35 && device.getRealtimeTransmitPendingCount() == 3); ++tests;
    Transfer_t rt_completion{device.tx_buffer1}; device.tx_data(&rt_completion);
    assert(packets[2] == std::vector<uint32_t>({0x0000FA0F}) && device.tx_queue_count == 32); ++tests;
    rt_completion.buffer = device.tx_buffer2; device.tx_data(&rt_completion);
    assert(packets[3] == std::vector<uint32_t>({0x0000F80F}) && device.tx_queue_count == 32); ++tests;
    rt_completion.buffer = device.tx_buffer1; device.tx_data(&rt_completion);
    assert(packets[4] == std::vector<uint32_t>({0x0000FC0F}) && device.tx_queue_count == 32);
    assert(device.tx_realtime_count == 0 && irq_enabled); ++tests;
    rt_completion.buffer = device.tx_buffer2; device.tx_data(&rt_completion);
    assert(packets[5] == std::vector<uint32_t>({8, 9, 10, 11})); ++tests;
    reset(device);
    for (unsigned message = 0; message < 8; ++message) device.write_packed(message);
    for (unsigned message = 0; message < MIDIDeviceBase::TX_REALTIME_QUEUE_SIZE + 2; ++message) device.write_packed(0x0000F80F);
    assert(device.tx_realtime_count == 16 && device.getRealtimeTransmitDropCount() == 2);
    assert(device.getTransmitDropCount() == 2 && device.tx_queue_count == 0); ++tests;
    device.disconnect(); assert(device.tx_realtime_count == 0 && !device.txtimer.active); ++tests;
    reset(device); device.write_packed(0x003C9009);
    assert(packets.empty()); device.send_now();
    assert(packets.size() == 1 && packets[0] == std::vector<uint32_t>({0x003C9009}) && irq_enabled); ++tests;
    reset(device);
    const std::vector<uint32_t> sysex = {0x0201F004, 0x05040304, 0xF7080607};
    for (uint32_t message : sysex) device.write_packed(message);
    device.write_packed(0x0000F80F);
    assert(packets[0] == std::vector<uint32_t>({sysex[0], sysex[1], sysex[2], 0x0000F80F})); ++tests;
    reset(device); device.tx_size = 512;
    const unsigned bounded_backlog = MIDIDeviceBase::TX_QUEUE_SIZE < 100 ? MIDIDeviceBase::TX_QUEUE_SIZE : 100;
    for (unsigned message = 0; message < 256 + bounded_backlog; ++message) device.write_packed(message);
    Transfer_t bounded_completion{device.tx_buffer1}; device.tx_data(&bounded_completion);
    assert(device.tx_queue_count == bounded_backlog - 16 && device.tx1_count == 16); ++tests;
    reset(device); std::vector<uint32_t> expected_notes;
    irq_enabled = false;
    for (unsigned note = 0; note < 20; ++note) {
        device.sendNoteOn(note, 100, 3); device.sendNoteOff(note, 64, 3);
        expected_notes.push_back(0x64009209 | (note << 16));
        expected_notes.push_back(0x40008208 | (note << 16));
    }
    assert(!irq_enabled && device.tx_queue_count == 32);
    irq_enabled = true; complete_pending(device);
    std::vector<uint32_t> delivered_notes;
    for (const auto &packet : packets) delivered_notes.insert(delivered_notes.end(), packet.begin(), packet.end());
    assert(delivered_notes == expected_notes && device.getTransmitDropCount() == 0); ++tests;
    reset(device); device.sendControlChange(74, 1, 2); device.sendControlChange(74, 2, 2);
    device.sendPitchBend(0, 4); device.send_now();
    assert(packets[0] == std::vector<uint32_t>({0x014AB10B, 0x024AB10B, 0x4000E30E})); ++tests;
    reset(device); uint8_t sysex_bytes[30] = {};
    sysex_bytes[0] = 0xF0; sysex_bytes[29] = 0xF7;
    for (unsigned index = 1; index < 29; ++index) sysex_bytes[index] = index;
    device.sendSysEx(30, sysex_bytes, true, 2); device.sendRealTime(0xF8, 2);
    assert(device.tx_queue_count == 2 && device.tx_realtime_count == 1);
    complete_pending(device);
    std::vector<uint8_t> delivered_sysex;
    unsigned delivered_clocks = 0;
    for (const auto &packet : packets) for (uint32_t message : packet) {
        if ((message & 0x0F) == 0x0F) {
            assert(message == 0x0000F82F); ++delivered_clocks;
        } else {
            assert((message & 0xF0) == 0x20);
            for (unsigned offset = 8; offset <= 24; offset += 8) delivered_sysex.push_back(message >> offset);
        }
    }
    assert(delivered_sysex == std::vector<uint8_t>(sysex_bytes, sysex_bytes + 30));
    assert(delivered_clocks == 1 && device.getTransmitDropCount() == 0); ++tests;
    reset(device); device.sendRealTime(0xF9); assert(device.tx1_count == 0 && packets.empty()); ++tests;
    #ifdef USBHOST_MIDI_TX_QUEUE_ALLOCATOR
    assert(sizeof(device.tx_queue) == sizeof(uint32_t *) && sizeof(device.tx_realtime_queue) == sizeof(uint32_t *)); ++tests;
    const unsigned allocations_before = allocation_calls;
    uint32_t *queue_before = device.tx_queue;
    assert(device.beginTransmitQueues() && allocation_calls == allocations_before); ++tests;
    device.sendNoteOn(60, 1, 1); device.sendRealTime(0xF8); timer_tick(device);
    assert(allocation_calls == allocations_before); ++tests;
    device.disconnect(); assert(device.hasTransmitQueueStorage() && device.tx_queue == queue_before);
    assert(device.beginTransmitQueues() && allocation_calls == allocations_before); ++tests;
    device = MIDIDeviceBase{}; device.txpipe = &device.pipe; device.rxpipe = &device.pipe;
    allocation_available = false;
    assert(!device.beginTransmitQueues() && !device.hasTransmitQueueStorage()); ++tests;
    for (unsigned message = 0; message < 9; ++message) device.write_packed(message);
    assert(device.tx_queue_count == 0 && device.getTransmitDropCount() == 1);
    device.sendRealTime(0xF8);
    assert(device.tx_realtime_count == 0 && device.getRealtimeTransmitDropCount() == 1); ++tests;
    const unsigned failed_calls = allocation_calls;
    irq_enabled = false;
    assert(!device.beginTransmitQueues() && allocation_calls == failed_calls && !irq_enabled); ++tests;
    irq_enabled = true; allocation_available = true;
    assert(device.beginTransmitQueues() && device.hasTransmitQueueStorage()); ++tests;
    #else
    assert(sizeof(device.tx_queue) == USBHOST_MIDI_TX_QUEUE_SIZE * sizeof(uint32_t));
    assert(sizeof(device.tx_realtime_queue) == 16 * sizeof(uint32_t)); ++tests;
    #endif
    for (unsigned port = 0; port < NUM_USB_MIDI_DEVICES; ++port) usb_midi_slots[port].device = &startup_devices[port];
    #ifdef USBHOST_MIDI_TX_QUEUE_ALLOCATOR
    const unsigned startup_allocations = allocation_calls;
    #endif
    setup_multi_usb(); assert(usb_starts == 1 && usb_tasks == 5); ++tests;
    #ifdef USBHOST_MIDI_TX_QUEUE_ALLOCATOR
    assert(allocation_calls == startup_allocations + NUM_USB_MIDI_DEVICES); ++tests;
    for (void *storage : allocations) std::free(storage);
    #endif
    std::cout << "PASS: " << tests << " USB MIDI/APC display regression cases, normal_capacity="
              << USBHOST_MIDI_TX_QUEUE_SIZE << ", inline_queue_storage="
              << sizeof(device.tx_queue) + sizeof(device.tx_realtime_queue) << " bytes/device\\n";
}
`;
const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'apc-midi-tx-'));
try {
    const executable = path.join(directory, process.platform === 'win32' ? 'test.exe' : 'test');
    for (const configuration of [[], ['-DUSBHOST_MIDI_TX_QUEUE_SIZE=128'],
        ['-DUSBHOST_MIDI_TX_QUEUE_ALLOCATOR=test_allocate'],
        ['-DUSBHOST_MIDI_TX_QUEUE_SIZE=128', '-DUSBHOST_MIDI_TX_QUEUE_ALLOCATOR=test_allocate']]) {
        const build = spawnSync(process.env.CXX || 'g++', [
            '-std=c++11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
            '-fsanitize=undefined', '-x', 'c++', '-', '-o', executable, ...configuration,
        ], { input: source, encoding: 'utf8' });
        if (build.error) throw build.error;
        if (build.status !== 0) throw new Error(build.stderr || 'Host compile failed');
        const result = spawnSync(executable, [], { encoding: 'utf8', timeout: 10000 });
        process.stdout.write(result.stdout || ''); process.stderr.write(result.stderr || '');
        if (result.error) throw result.error;
        if (result.status !== 0) throw new Error(`Host test failed: ${result.status} ${result.signal}`);
    }
} finally {
    fs.rmSync(directory, { recursive: true, force: true });
}