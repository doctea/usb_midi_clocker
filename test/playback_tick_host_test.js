const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const main = fs.readFileSync(path.join(__dirname, '..', 'src', 'main.cpp'), 'utf8');
const start = main.indexOf('void do_tick(uint32_t in_ticks) {');
if (start < 0) throw new Error('Cannot locate production do_tick');
let depth = 1;
let end = main.indexOf('{', start) + 1;
for (; end < main.length && depth; ++end) {
    if (main[end] === '{') ++depth;
    if (main[end] === '}') --depth;
}
if (depth) throw new Error('Unclosed do_tick');
const source = `
#include <cassert>
#include <cstdint>
#define F(text) text
#define Serial_println(...) ((void)0)
#define Serial_printf(...) ((void)0)
static unsigned flushes = 0;
#define Serial_flush() (++flushes)
#define DEBUG_MAIN_PRINTLN(text) ((void)0)
#define ATOMIC_RESTORESTATE 0
#define ATOMIC_BLOCK(mode) if (true)
static bool playing = false, debug_flag = false;
static bool restart_on_next_bar = false;
struct SerialMock {
    template <typename... Args> void printf(const char *, Args...) {}
} Serial;
static uint32_t ticks = 0;
static float micros_per_tick = 1000;
static uint32_t micros() { return 100; }
static uint32_t millis() { return 1; }
static bool is_restart_on_next_bar() { return false; }
static void set_restart_on_next_bar(bool) {}
static void global_on_restart() {}
static bool is_bpm_on_bar(uint32_t tick) { return tick % 96 == 0; }
static bool is_bpm_on_phrase(uint32_t tick) { return tick % 384 == 0; }
static bool is_bpm_on_beat(uint32_t tick) { return tick % 24 == 0; }
#define BPM_CURRENT_PHRASE (ticks / 384)
#define BPM_CURRENT_BAR_OF_PHRASE ((ticks / 96) % 4)
#define BPM_CURRENT_BEAT (ticks / 24)
struct Project { void on_phrase(uint32_t) {} } project_instance;
static Project *project = &project_instance;
struct Behaviours {
    unsigned clocks = 0, tick_calls = 0;
    void do_bar(uint32_t) {}
    void do_end_phrase_pre_clock(uint32_t) {}
    void do_pre_clock(uint32_t) {}
    void do_end_beat(uint32_t) {}
    void do_end_bar(int) {}
    void do_end_phrase(uint32_t) {}
    void send_clocks() { ++clocks; }
    void do_ticks(uint32_t) { ++tick_calls; }
} behaviours;
static Behaviours *behaviour_manager = &behaviours;
struct Parameters {
    void tick_sh() {}
    bool pending_calibration() { return false; }
    void process_calibration() {}
} parameters;
static Parameters *parameter_manager = &parameters;
${main.slice(start, end)}
int main() {
    do_tick(0);
    assert(behaviours.clocks == 0 && flushes == 0);
    playing = true;
    for (uint32_t tick = 0; tick < 768; ++tick) do_tick(tick);
    assert(behaviours.clocks == 768 && behaviours.tick_calls == 768);
    assert(flushes == 0);
}
`;
const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'playback-tick-test-'));
try {
    const file = path.join(directory, 'test.cpp');
    const executable = path.join(directory, process.platform === 'win32' ? 'test.exe' : 'test');
    fs.writeFileSync(file, source);
    const compile = spawnSync('g++', ['-std=c++11', '-Wall', '-Wextra', '-Werror',
        '-Wno-unused-variable', '-Wno-unused-but-set-variable',
        '-fsanitize=undefined', file, '-o', executable], { encoding: 'utf8' });
    if (compile.error) throw compile.error;
    if (compile.status !== 0) throw new Error(compile.stdout + compile.stderr);
    const run = spawnSync(executable, [], { encoding: 'utf8' });
    if (run.error) throw run.error;
    if (run.status !== 0) throw new Error(run.stdout + run.stderr);
    console.log('Playback tick regression passed: 768 ticks dispatch clocks/behaviours without serial flush when debug logging is off');
} finally {
    for (const name of fs.readdirSync(directory)) fs.unlinkSync(path.join(directory, name));
    fs.rmdirSync(directory);
}
