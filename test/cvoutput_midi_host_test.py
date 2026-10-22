#!/usr/bin/env python3
"""Host regressions for CV pool/direct/off routing and scene-setting migration."""

from pathlib import Path
import runpy
import subprocess
import tempfile


root = Path(__file__).resolve().parent.parent
library = root.parent / "my_libraries" / "parameters"
extract = runpy.run_path(str(library / "test_cvoutput.py"))["extract_body"]
poly = (root / "include" / "behaviours" / "behaviour_polyphonic.h").read_text()
cv = (root / "include" / "behaviours" / "behaviour_cvoutput.h").read_text()


def methods(text, signatures):
    return "\n".join(extract(text, signature) for signature in signatures).replace(" override", "")


poly_methods = methods(poly, (
    "virtual bool is_voice_allowed_for_auto(",
    "int8_t find_slot_for(",
    "virtual routed_note_output_t resolve_routed_note_output(",
    "virtual void sendNoteOn(",
    "virtual void sendNoteOff(",
    "virtual void setup_saveable_settings(",
))
cv_methods = methods(cv, (
    "uint8_t get_channel_midi_mode(",
    "virtual bool is_voice_allowed_for_auto(",
    "void set_channel_midi_mode(",
    "virtual routed_note_output_t resolve_routed_note_output(",
    "virtual void send_resolved_routed_note_on(",
    "virtual void send_resolved_routed_note_off(",
    "virtual void actualSendNoteOn(",
    "virtual void actualSendNoteOff(",
    "virtual void sendNoteOn(",
    "void park_channel(",
))
settings = cv[cv.index("virtual void setup_saveable_settings() override"):]
midi_settings = extract(settings, "for (int i = 0; i < channel_count; ++i)")

source = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <string>
#define ENABLE_SCALES
#define SL_SCOPE_SCENE 0
constexpr int NOTE_OFF = -1, QUANTISE_MODE_NONE = 0, QUANTISE_MODE_CHORD = 2;
bool is_valid_note(int note) { return note >= 0 && note < 128; }
const char *get_note_name_c(int) { return "note"; }
struct SerialMock { template<class... T> void printf(const char *, T...) {} } Serial;
#define Serial_printf(...) Serial.printf(__VA_ARGS__)
struct String : std::string {
    using std::string::string;
    String(char c) : std::string(1, c) {}
    String(std::string value) : std::string(value) {}
};
String operator+(const String& a, const String& b) {
    return String(static_cast<const std::string&>(a) + static_cast<const std::string&>(b));
}
String operator+(const String& a, const char *b) {
    return String(static_cast<const std::string&>(a) + b);
}
struct LSaveableBool {
    std::string key;
    std::function<void(bool)> set;
    std::function<bool()> get;
    LSaveableBool(const char *name, const char *, void *,
                  std::function<void(bool)> setter, std::function<bool()> getter)
        : key(name), set(setter), get(getter) {}
    virtual ~LSaveableBool() {}
};
template<class T> using LSaveableSetting = LSaveableBool;
template<class T> struct VarSetting : LSaveableBool {
    VarSetting(const char *name, const char *group, T *value)
        : LSaveableBool(name, group, nullptr,
            [value](bool v) { *value = v; }, [value]() { return *value; }) {}
};
struct routed_note_output_t { int8_t pitch = NOTE_OFF; uint8_t channel = 0; bool emitted = false; };
struct DeviceBehaviourUltimateBase {
    bool debug = false;
    uint8_t current_channel = 0;
    unsigned transforms = 0;
    std::map<std::string, LSaveableBool*> settings;
    virtual ~DeviceBehaviourUltimateBase() {
        for (auto& setting : settings) delete setting.second;
    }
    virtual routed_note_output_t resolve_routed_note_output(uint8_t note, uint8_t channel) {
        return {int8_t(note), channel, true};
    }
    virtual void actualSendNoteOn(uint8_t, uint8_t, uint8_t) {}
    virtual void actualSendNoteOff(uint8_t, uint8_t, uint8_t) {}
    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
        actualSendNoteOn(note, velocity, channel);
    }
    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
        actualSendNoteOff(note, velocity, channel);
    }
    void begin_note_limit_change() { ++transforms; }
    void end_note_limit_change() {}
    void register_setting(LSaveableBool *setting, int) { settings[setting->key] = setting; }
};
struct Conductor {
    int get_global_quantise_mode() { return 1; }
    int8_t quantise_to_scale(int8_t note) { return note + 1; }
    int8_t quantise_to_chord(int8_t note, int) { return note + 2; }
} harmony;
Conductor *conductor = &harmony;
struct Output {
    bool midi_notes_enabled = true, park = true, active = false, locked = false;
    int8_t current_pitch_note = NOTE_OFF;
    int notes = 0, parks = 0;
    float base = 0;
    bool get_park_enabled() const { return park; }
    void set_park_enabled(bool enabled) { park = enabled; }
    bool is_note_active() const { return active; }
    void sendNoteOn(uint8_t note, uint8_t, uint8_t) {
        if (!midi_notes_enabled || locked) return;
        ++notes; current_pitch_note = note; active = true; base = note / 12.0f;
    }
    void sendNoteOff(uint8_t note, uint8_t, uint8_t) {
        if (current_pitch_note != note) return;
        active = false; current_pitch_note = NOTE_OFF;
    }
    void set_midi_notes_enabled(bool enabled) {
        midi_notes_enabled = enabled;
        if (!enabled) { base = 0; active = false; current_pitch_note = NOTE_OFF; }
    }
    void park_pitch(int8_t note) {
        if (!midi_notes_enabled || locked) return;
        ++parks; base = note / 12.0f;
    }
};
struct PolyphonicBehaviour : DeviceBehaviourUltimateBase {
    static const int8_t max_voice_count = 4;
    const int8_t CHANNEL_ROUND_ROBIN = 5;
    int8_t voices[4] = {-1,-1,-1,-1}, last_voices[4] = {-1,-1,-1,-1};
    bool allow_voice_for_auto[4] = {true,true,true,true};
''' + poly_methods + r'''
};
struct CVBehaviour : PolyphonicBehaviour {
    static const int8_t channel_count = 4;
    Output storage[4];
    Output *outputs[4] = {&storage[0],&storage[1],&storage[2],&storage[3]};
    int8_t last_channel_note[4] = {-1,-1,-1,-1};
    int8_t apply_per_channel_limits(uint8_t note, uint8_t) { return note; }
    void register_midi_settings() {
        PolyphonicBehaviour::setup_saveable_settings();
''' + midi_settings + r'''
    }
''' + cv_methods + r'''
};
int main() {
    CVBehaviour cv;
    cv.register_midi_settings();
    // Legacy scenes with Pooled=false retain dedicated-channel notes, not Off.
    cv.settings.at("Output 1")->set(false);
    assert(cv.get_channel_midi_mode(0) == 1);
    cv.sendNoteOn(48, 100, 1);
    assert(cv.storage[0].active);
    cv.set_channel_midi_mode(0, 2);
    cv.set_channel_midi_mode(0, 1);
    assert(cv.storage[0].active && cv.voices[0] == 48);
    cv.sendNoteOff(48, 0, 1);
    cv.park_channel(0);
    assert(cv.storage[0].parks > 0); // Direct channels can park too.
    cv.set_channel_midi_mode(0, 0);
    assert(cv.storage[0].base == 0 && !cv.storage[0].active && cv.voices[0] == NOTE_OFF);
    cv.sendNoteOn(60, 100, 1);
    cv.actualSendNoteOn(60, 100, 1);
    cv.send_resolved_routed_note_on(60, 100, 1);
    assert(!cv.resolve_routed_note_output(60, 1).emitted);
    assert(!cv.storage[0].active && cv.storage[0].base == 0);
    const auto parks = cv.storage[0].parks;
    cv.park_channel(0);
    assert(cv.storage[0].parks == parks);
    cv.set_channel_midi_mode(1, 1);
    cv.sendNoteOn(60, 100, 5);
    assert(!cv.storage[0].active && !cv.storage[1].active && cv.storage[2].active);
    cv.sendNoteOff(60, 0, 5);
    assert(cv.voices[2] == NOTE_OFF);
    cv.set_channel_midi_mode(0, 2);
    cv.sendNoteOn(36, 100, 5);
    assert(cv.storage[0].active);
    cv.set_channel_midi_mode(0, 0); // Changing modes clears a held pooled note.
    assert(!cv.storage[0].active && cv.storage[0].base == 0);
    // Scene round trip: legacy pool flags plus new MIDI-enabled and Park settings.
    cv.set_channel_midi_mode(2, 1);
    cv.settings.at("Ch C Park")->set(false);
    std::map<std::string, bool> saved;
    for (auto& item : cv.settings) saved[item.first] = item.second->get();
    CVBehaviour restored;
    restored.register_midi_settings();
    // Load legacy pool flags first, as they appear before the new settings on disk.
    for (int i = 0; i < 4; ++i) {
        const std::string key = "Output " + std::to_string(i + 1);
        restored.settings.at(key)->set(saved.at(key));
    }
    for (auto& item : saved) if (item.first.find("Ch ") == 0)
        restored.settings.at(item.first)->set(item.second);
    assert(restored.get_channel_midi_mode(0) == 0);
    assert(restored.get_channel_midi_mode(1) == 1);
    assert(restored.get_channel_midi_mode(2) == 1);
    assert(restored.get_channel_midi_mode(3) == 2);
    assert(!restored.storage[2].park);
    assert(restored.find_slot_for(NOTE_OFF) == 3);
    std::puts("CV On/Direct/Off routing, Park policy, and scene migration checks passed");
}
'''

with tempfile.TemporaryDirectory(prefix="cv-midi-test-") as directory:
    cpp = Path(directory) / "test.cpp"
    executable = Path(directory) / "test"
    cpp.write_text(source)
    subprocess.run([
        "g++", "-std=c++14", "-Wall", "-Wextra", "-Werror",
        "-Wno-unused-parameter", "-fsanitize=undefined", "-fno-sanitize-recover=all",
        str(cpp), "-o", str(executable),
    ], check=True)
    subprocess.run([str(executable)], check=True)
