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
const manager = read(path.join('src', 'behaviours', 'behaviour_manager.cpp'));
const matrix = read(path.join('include', 'midi', 'midi_mapper_matrix_manager.h'));
const ui = read(path.join('include', 'mymenu', 'menu_midi_matrix.h'));
const rowsStart = ui.indexOf('enum class PopupRow');
const rowsEnd = ui.indexOf('enum class Mode', rowsStart);
if (rowsStart < 0 || rowsEnd < 0) throw new Error('Cannot locate popup rows');
const source = `
#include <cassert>
#include <cstdint>
#include <vector>
#include "midi/midi_endpoint.h"
#define F(text) text
#define Serial_printf(...) ((void)0)
#define Serial_println(...) ((void)0)
#define Serial_print(...) ((void)0)
#define Serial_flush() ((void)0)
void debug_free_ram() {}
constexpr uint16_t C_WHITE = 0xFFFF;
using source_id_t = int;
using target_id_t = int;
template <class T> struct List {
    std::vector<T> values;
    unsigned int size() const { return values.size(); }
};
struct MenuItemList {
    unsigned int count = 0;
    unsigned int size() const { return count; }
};
struct SeparatorMenuItem {
    explicit SeparatorMenuItem(const char *) {}
};
struct Menu {
    List<int> storage;
    List<int> *pages = &storage;
    uint16_t get_next_colour() { return C_WHITE; }
    int add_page(const char *, uint16_t, bool, const char *) {
        const int index = pages->size();
        pages->values.push_back(index);
        return index;
    }
    void add(SeparatorMenuItem *item, uint16_t) { delete item; }
    void add(MenuItemList *, uint16_t) {}
};
struct DeviceBehaviourUltimateBase : MIDIEndpointOwner {
    Menu *menu;
    MenuItemList items;
    unsigned int custom_pages = 0;
    bool parameters = false;
    bool dedicated = false;
    int source_id = -1, target_id = -1;
    uint16_t colour = C_WHITE;
    explicit DeviceBehaviourUltimateBase(Menu *menu) : menu(menu) {}
    const char *get_label() { return "Behaviour"; }
    const char *get_endpoint_owner_label() override { return get_label(); }
    MenuItemList *make_menu_items() {
        for (unsigned int i = 0; i < custom_pages; ++i)
            menu->add_page("Custom", colour, true, "Custom group");
        return &items;
    }
    bool has_parameters() { return parameters; }
    bool show_dedicated_parameters_page() { return dedicated; }
    bool is_page_scrollable() { return true; }
    void *get_parameters() { return nullptr; }
};
struct MIDIMatrixManager {
    static constexpr int MAX_NUM_SOURCES = 4, MAX_NUM_TARGETS = 1;
    int NUM_REGISTERED_SOURCES = 4, NUM_REGISTERED_TARGETS = 1;
    struct Entry {
        int16_t menu_page_index = -1;
        MIDIEndpointOwner *owner = nullptr;
        const char *role = nullptr;
    };
    Entry storage[4], targets[1];
    Entry *sources = storage;
    ${method(matrix, 'int16_t get_source_page_index(')}
    ${method(matrix, 'int16_t get_target_page_index(')}
    ${method(matrix, 'MIDIEndpointOwner *get_source_owner(')}
    ${method(matrix, 'MIDIEndpointOwner *get_target_owner(')}
    ${method(matrix, 'void set_source_owner(')}
    ${method(matrix, 'void set_target_owner(')}
    void *get_connection_policy(int, int) { return nullptr; }
    int connected_to_target_count(int) { return 0; }
    int connected_to_source_count(int) { return 0; }
};
MIDIMatrixManager *midi_matrix_manager;
struct ParameterManager {
    void addParameterSubMenuItems(Menu *, const char *, void *, uint16_t) {}
} parameters;
ParameterManager *parameter_manager = &parameters;
struct DeviceBehaviourManager {
    void create_single_behaviour_menu_items(Menu *, DeviceBehaviourUltimateBase *);
};
${method(manager, 'void DeviceBehaviourManager::create_single_behaviour_menu_items(')}
struct Popup {
    ${ui.slice(rowsStart, rowsEnd)}
    int sel_source = -1, sel_target = -1;
    ${method(ui, 'bool popup_row_enabled(')}
};
int main() {
    DeviceBehaviourManager behaviours;
    for (int preceding_pages : {0, 3}) {
        for (bool standard_page : {false, true}) {
            Menu menu;
            for (int i = 0; i < preceding_pages; ++i)
                menu.add_page("Other", C_WHITE, true, "Other");
            DeviceBehaviourUltimateBase euclidian(&menu);
            euclidian.custom_pages = 5;
            euclidian.items.count = standard_page ? 1 : 0;
            MIDIMatrixManager matrix;
            midi_matrix_manager = &matrix;
            for (auto &entry : matrix.storage) entry.owner = &euclidian;
            matrix.targets[0].owner = &euclidian;
            behaviours.create_single_behaviour_menu_items(&menu, &euclidian);
            const int expected = preceding_pages + (standard_page ? 5 : 0);
            assert(euclidian.routing_page_index == expected);
            assert(menu.pages->size() == static_cast<unsigned>(preceding_pages + 5 + standard_page));
            Popup popup;
            for (int channel = 0; channel < 4; ++channel) {
                popup.sel_source = channel;
                assert(matrix.get_source_page_index(channel) == expected);
                assert(popup.popup_row_enabled(Popup::PopupRow::JUMP_SOURCE));
                assert(!popup.popup_row_enabled(Popup::PopupRow::JUMP_TARGET));
            }
            popup.sel_target = 0;
            assert(popup.popup_row_enabled(Popup::PopupRow::JUMP_TARGET));
            matrix.storage[1].menu_page_index = 2;
            assert(matrix.get_source_page_index(1) == 2);
        }
    }
    Menu menu;
    MIDIMatrixManager matrix;
    midi_matrix_manager = &matrix;
    DeviceBehaviourUltimateBase empty(&menu);
    behaviours.create_single_behaviour_menu_items(&menu, &empty);
    assert(empty.routing_page_index == -1 && menu.pages->size() == 0);
    empty.routing_page_index = menu.add_page("Explicit", C_WHITE, true, "Custom");
    empty.custom_pages = 2;
    behaviours.create_single_behaviour_menu_items(&menu, &empty);
    assert(empty.routing_page_index == 0);
}
`;
const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'routing-page-test-'));
const file = path.join(directory, 'test.cpp');
const executable = path.join(directory, process.platform === 'win32' ? 'test.exe' : 'test');
try {
    fs.writeFileSync(file, source);
    const compile = spawnSync('g++', ['-std=c++11', '-Wall', '-Wextra', '-Werror',
        '-fsanitize=undefined', '-I', path.join(root, 'include'), file, '-o', executable],
        { encoding: 'utf8' });
    if (compile.error) throw compile.error;
    if (compile.status !== 0) throw new Error(compile.stdout + compile.stderr);
    const run = spawnSync(executable, [], { encoding: 'utf8' });
    if (run.error) throw run.error;
    if (run.status !== 0) throw new Error(run.stdout + run.stderr);
    console.log('Routing page regression tests passed (custom/standard pages, all four sources, popup availability)');
} finally {
    for (const name of fs.readdirSync(directory)) fs.unlinkSync(path.join(directory, name));
    fs.rmdirSync(directory);
}
