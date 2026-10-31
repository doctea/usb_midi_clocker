const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const libraries = path.resolve(__dirname, '..', '..', 'my_libraries');
const header = fs.readFileSync(path.join(libraries, 'mymenu', 'include', 'menu.h'), 'utf8');
const menuitemsHeader = fs.readFileSync(path.join(libraries, 'mymenu', 'include', 'menuitems.h'), 'utf8');
const allocator = fs.readFileSync(path.join(__dirname, '..', 'src', 'new.cpp'), 'utf8');
const gateMenus = fs.readFileSync(path.join(__dirname, '..', 'src', 'interfaces', 'interfaces.cpp'), 'utf8');
const debugSource = fs.readFileSync(path.join(__dirname, '..', 'src', 'debug.cpp'), 'utf8');
function extract(startMarker, endMarker, text = header) {
    const start = text.indexOf(startMarker);
    const end = text.indexOf(endMarker, start);
    if (start < 0 || end < 0) throw new Error(`Cannot locate ${startMarker}`);
    return text.slice(start, end);
}

const source = `
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <stdint.h>
#include <new>
#include <functional>
${extract('#if !defined(__EXCEPTIONS)', '// Replacement functions', allocator)}
static bool fail_next_allocation = false;
static unsigned array_allocations = 0;
static void *allocate_array(size_t bytes) {
    ++array_allocations;
    if (fail_next_allocation) {
        fail_next_allocation = false;
        return nullptr;
    }
    return std::malloc(bytes);
}
void *operator new[](size_t bytes) {
    return allocate_array(bytes);
}
void operator delete[](void *pointer) noexcept { std::free(pointer); }
${extract('void * operator new(size_t size, const std::nothrow_t &)', 'void operator delete(void * ptr)', allocator)}
#include "menu_arena.h"
#define MENU_ITEM_ARENA
MenuItemArena *menu_item_arena = nullptr;
#include "GenericList.h"
static constexpr uint16_t C_WHITE = 0xFFFF;
struct MenuItem {
${extract('        static void *operator new(size_t size)', '    #endif\n\n        bool debug', menuitemsHeader)}
    static unsigned destructor_count;
    ~MenuItem() { ++destructor_count; }
    bool is_openable() const { return true; }
    bool is_selectable() const { return true; }
    const char *get_label() const { return "test"; }
};
unsigned MenuItem::destructor_count = 0;
${extract('class MenuItemList {', '\nclass Menu {')}
struct Menu {
    GenericList<page_t*> *pages = new GenericList<page_t*>;
    GenericList<page_group_t*> *groups = new GenericList<page_group_t*>;
    page_t *selected_page = nullptr;
    void select_page(unsigned int position) {
        if (pages->get(position) != nullptr) selected_page = pages->get(position);
    }
${extract('        int add_page(const char *title,', '        FLASHMEM void setup_display()')}
    ~Menu() {
        for (auto *page : *pages) {
            std::free(const_cast<char*>(page->title));
            delete page->items;
            delete page;
        }
        for (auto *group : *groups) delete group;
        delete pages;
        delete groups;
    }
};
namespace gate_menu_test {
using byte = uint8_t;
static constexpr int MENU_C_MAX = 64;
static constexpr int BANK_CLOCK = 0;
struct SerialMock {
    void println(const char *) {}
    template<typename... Args> void printf(const char *, Args...) {}
} Serial;
#define Serial_flush() ((void)0)
static void debug_free_ram() {}
struct BankInterface { byte num_gates = 8; };
struct GateMultiToggleItem {
    int bank, gate;
    GateMultiToggleItem(const char *label, int bank, int gate) : bank(bank), gate(gate) {
        assert(label != nullptr);
        assert(std::strtol(label, nullptr, 10) == bank * 8 + gate + 1);
    }
};
static bool fail_control_allocation = false;
static unsigned control_constructions = 0;
struct ObjectMultiToggleControl {
    std::vector<GateMultiToggleItem *> items;
    ObjectMultiToggleControl(const char *, bool) { ++control_constructions; }
    ~ObjectMultiToggleControl() { for (auto *item : items) delete item; }
    void addItem(GateMultiToggleItem *item) { items.push_back(item); }
    static void *operator new(size_t bytes, const std::nothrow_t &) noexcept {
        return fail_control_allocation ? nullptr : ::operator new(bytes, std::nothrow);
    }
    static void operator delete(void *pointer) noexcept { ::operator delete(pointer); }
};
struct GatesDisplay { explicit GatesDisplay(const char *) {} };
struct Menu {
    std::vector<ObjectMultiToggleControl *> controls;
    ~Menu() { for (auto *control : controls) delete control; }
    void select_page_by_name(const char *) {}
    void add(GatesDisplay *display) { delete display; }
    void add(ObjectMultiToggleControl *control) { controls.push_back(control); }
};
struct GateManager {
    byte num_banks = 0;
    BankInterface *banks[4] = {};
    void create_controls(Menu *menu);
};
${extract('    const char *gate_labels', '    #include "mymenu/menu_gatedisplay.h"', gateMenus)}
${extract('    void GateManager::create_controls(', '\n#endif', gateMenus)}
static void check_gate_menus() {
    for (byte bank_count : {2, 4}) {
        BankInterface banks[4];
        GateManager manager;
        manager.num_banks = bank_count;
        for (byte bank = 0; bank < bank_count; ++bank) manager.banks[bank] = &banks[bank];
        Menu menu;
        manager.create_controls(&menu);
        assert(menu.controls.size() == bank_count);
        for (const auto *control : menu.controls) assert(control->items.size() == 8);
    }
    BankInterface bank;
    GateManager manager;
    manager.num_banks = 1;
    manager.banks[0] = &bank;
    Menu menu;
    const unsigned constructions_before = control_constructions;
    fail_control_allocation = true;
    manager.create_controls(&menu);
    fail_control_allocation = false;
    assert(menu.controls.empty());
    assert(control_constructions == constructions_before);
    std::cout << "Two/four-bank gate labels and checked allocation failure passed\\n";
}
}
namespace extmem_stats_test {
struct Pool {} extmem_smalloc_pool;
static uint32_t now_ms = 0;
static unsigned stats_calls = 0;
static int stats_result = 1;
static size_t available_bytes = 4096;
static uint32_t millis() { return now_ms; }
static int sm_malloc_stats_pool(Pool *pool, size_t *total, size_t *, size_t *free_bytes, int *) {
    assert(pool == &extmem_smalloc_pool);
    assert(total != nullptr && free_bytes != nullptr);
    ++stats_calls;
    if (stats_result < 0) return stats_result;
    *total = 16384 - available_bytes;
    *free_bytes = 16384 - *total;
    return stats_result;
}
${extract('    int freeExtRam() {', '  #endif', debugSource)}
static void check_extmem_stats() {
    assert(freeExtRam() == 4096 && stats_calls == 1);
    available_bytes = 2048;
    now_ms = 999;
    assert(freeExtRam() == 4096 && stats_calls == 1);
    now_ms = 1000;
    assert(freeExtRam() == 2048 && stats_calls == 2);
    stats_result = -1;
    now_ms = 2000;
    assert(freeExtRam() == 0 && stats_calls == 3);
    now_ms = 2500;
    assert(freeExtRam() == 0 && stats_calls == 3);
    stats_result = 0;
    available_bytes = 16384;
    now_ms = 3000;
    assert(freeExtRam() == 16384 && stats_calls == 4);
    stats_result = 1;
    available_bytes = 0;
    now_ms = 4000;
    assert(freeExtRam() == 0 && stats_calls == 5);
    std::cout << "EXTRAM stats output pointers, caching, and failed/empty/full pools passed\\n";
}
}
int main() {
    extmem_stats_test::check_extmem_stats();
    gate_menu_test::check_gate_menus();
    alignas(max_align_t) unsigned char arena_storage[256];
    MenuItemArena arena(reinterpret_cast<char*>(arena_storage), sizeof(arena_storage));
    menu_item_set_arena(&arena);
    arena.begin_allocations();
    MenuItem *arena_item = new MenuItem();
    assert(arena.owns(arena_item));
    assert(reinterpret_cast<uintptr_t>(arena_item) % alignof(max_align_t) == 0);
    const size_t arena_bytes_after_item = arena.bytes_used();
    delete arena_item;
    assert(MenuItem::destructor_count == 1);
    assert(arena.bytes_used() == arena_bytes_after_item);

    MenuItem *nothrow_arena_item = new (std::nothrow) MenuItem();
    assert(nothrow_arena_item != nullptr && arena.owns(nothrow_arena_item));
    delete nothrow_arena_item;
    assert(MenuItem::destructor_count == 2);

    MenuItem *arena_array = new MenuItem[2];
    assert(arena.owns(arena_array));
    delete[] arena_array;
    assert(MenuItem::destructor_count == 4);

    const uint32_t fallback_count = arena.heap_fallback_count();
    void *overflow_fallback = menu_item_allocate(sizeof(arena_storage) * 2);
    assert(overflow_fallback != nullptr && !arena.owns(overflow_fallback));
    assert(arena.heap_fallback_count() == fallback_count + 1);
    menu_item_deallocate(overflow_fallback);

    arena.close_allocations();
    MenuItem *runtime_item = new MenuItem();
    assert(runtime_item != nullptr && !arena.owns(runtime_item));
    delete runtime_item;
    assert(MenuItem::destructor_count == 5);
    assert(arena.heap_fallback_count() == fallback_count + 1);
    menu_item_set_arena(nullptr);

    MenuItem entries[20];
    {
        GenericList<MenuItem*> list;
        for (unsigned int index = 0; index < 12; ++index) list.add(&entries[index]);
        fail_next_allocation = true;
        list.add(4, &entries[12]);
        assert(list.size() == 12 && !fail_next_allocation);
        for (unsigned int index = 0; index < 12; ++index)
            assert(list.get(index) == &entries[index]);
        list.add(4, &entries[12]);
        assert(list.size() == 13 && list.get(4) == &entries[12]);
        list.remove(4);
        fail_next_allocation = true;
        list.shrink_to_fit();
        assert(list.size() == 12 && !fail_next_allocation);
        list.shrink_to_fit();
        for (unsigned int index = 0; index < 12; ++index)
            assert(list.get(index) == &entries[index]);
        list.clear();
        list.shrink_to_fit();
        list.add(&entries[0]);
        assert(list.size() == 1 && list.get(0) == &entries[0]);
    }
    {
        MenuItemList list;
        for (unsigned int index = 0; index < 8; ++index) list.add(&entries[index]);
        fail_next_allocation = true;
        list.add(&entries[8]);
        assert(list.size() == 8 && !fail_next_allocation);
        assert(list.openable_count() == 8 && list.selectable_count() == 8);
        list.add(&entries[8]);
        list.remove(8);
        fail_next_allocation = true;
        list.shrink_to_fit();
        assert(list.size() == 8 && !fail_next_allocation);
        list.shrink_to_fit();
        for (unsigned int index = 0; index < 8; ++index)
            assert(list.get(index) == &entries[index]);
        list.clear();
        list.shrink_to_fit();
        list.add(&entries[0]);
        assert(list.size() == 1 && list.openable_count() == 1 && list.selectable_count() == 1);
    }
    Menu menu;
    for (unsigned int index = 0; index < 12; ++index) {
        const int position = menu.add_page(index < 8 ? "Main" : "Debug", C_WHITE, true,
                                           index < 8 ? "Default" : "Debug");
        assert(position == static_cast<int>(index));
        assert(menu.selected_page == menu.pages->get(position));
        assert(menu.selected_page->items != nullptr);
        assert(menu.selected_page->group != nullptr);
    }
    auto *debug_group = menu.groups->get(1);
    assert(debug_group->pages.size() == 4);
    for (unsigned int index = 0; index < 4; ++index)
        assert(debug_group->pages.get(index) == menu.pages->get(index + 8));

    auto *previous_page = menu.selected_page;
    auto *previous_items = previous_page->items;
    fail_next_allocation = true;
    const int rejected = menu.add_page("Rejected", C_WHITE, true, "Default");
    assert(rejected == -1);
    assert(menu.pages->size() == 12);
    assert(menu.groups->get(0)->pages.size() == 8);
    assert(menu.selected_page == previous_page);
    assert(previous_page->items == previous_items);
    assert(!fail_next_allocation);
    assert(array_allocations > 0);
    assert(menu.add_page("Retry", C_WHITE, true, "Default") == 12);
    assert(menu.selected_page == menu.pages->get(12));
    assert(menu.groups->get(0)->pages.size() == 9);
    const auto *last_page = menu.pages->get(12);
    assert(menu.insert_page("Inserted", 2, C_WHITE, true, "Default") == 2);
    assert(menu.selected_page == menu.pages->get(2));
    assert(menu.pages->get(13) == last_page);
    std::cout << "MenuItem arena, list resizing, page registration, and allocation-failure checks passed\\n";
}
`;

const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'menu-page-host-'));
function checkArmLink() {
    const compiler = process.env.MENU_TEST_ARM_CXX;
    if (!compiler) return;
    function compilerPath(file) {
        if (process.platform === 'win32' || !compiler.endsWith('.exe')) return file;
        const result = spawnSync('wslpath', ['-w', file], { encoding: 'utf8' });
        if (result.status !== 0) throw new Error(result.stderr || 'Cannot convert ARM probe path');
        return result.stdout.trim();
    }
    const flags = ['-std=c++11', '-fno-exceptions', '-fno-rtti', '-Os',
        '-fno-unwind-tables', '-fno-asynchronous-unwind-tables',
        '-ffunction-sections', '-fdata-sections', '-mcpu=cortex-m7', '-mthumb',
        '-mfloat-abi=hard', '-mfpu=fpv5-d16', '-nostartfiles',
        '-Wl,--gc-sections', '-T', compilerPath(path.join(__dirname, '..', 'linker', 'imxrt1062_t41_alt.ld'))];
    for (const dependency of ['allocation', 'callback']) {
        for (const withOverrides of [false, true]) {
            const name = `arm-${dependency}-${withOverrides ? 'fixed' : 'original'}`;
            const file = path.join(directory, `${name}.cpp`);
            const executable = path.join(directory, `${name}.elf`);
            const callback = dependency === 'callback';
            const overrides = callback
                ? extract('#if !defined(__EXCEPTIONS)', '// Replacement functions', allocator)
                : extract('void * operator new(size_t size, const std::nothrow_t &)', 'void operator delete(void * ptr)', allocator);
            fs.writeFileSync(file, `
#include <stdlib.h>
#include <new>
${callback ? '#include <functional>' : ''}
${callback ? `
extern "C" {
    void *__dso_handle = nullptr;
    int _getpid() { return 1; }
    int _kill(int, int) { return -1; }
    [[noreturn]] void _exit(int) { for (;;) {} }
}
` : ''}
static unsigned char storage[64];
void *operator new(size_t) { return storage; }
void *operator new[](size_t) { return storage; }
${withOverrides ? overrides : ''}
${callback ? `
static std::function<void()> callback;
extern "C" void ImageVectorTable() {
    callback();
}
` : `
using Allocator = void *(*)(size_t, const std::nothrow_t &);
static Allocator volatile scalar_allocator = &::operator new;
static Allocator volatile array_allocator = &::operator new[];
extern "C" void ImageVectorTable() {
    scalar_allocator(16, std::nothrow);
    array_allocator(16, std::nothrow);
}
`}
`);
            const result = spawnSync(compiler, [...flags, compilerPath(file), '-o', compilerPath(executable)],
                { encoding: 'utf8', cwd: path.dirname(compiler) });
            if (result.error) throw result.error;
            const output = result.stdout + result.stderr;
            if (withOverrides) {
                if (result.status !== 0) throw new Error(output);
            } else if (result.status === 0 || !output.includes('.ARM.exidx has both ordered')) {
                throw new Error(`ARM ${dependency} probe did not reproduce the original linker error:\n${output}`);
            }
        }
    }
    console.log('ARM probes reproduced the allocation and callback linker errors; production runtime overrides link successfully');
}
try {
    const file = path.join(directory, 'test.cpp');
    const executable = path.join(directory, process.platform === 'win32' ? 'test.exe' : 'test');
    fs.writeFileSync(file, source);
    const sanitizers = process.env.MENU_TEST_SANITIZERS || 'undefined';
    const compile = spawnSync('g++', ['-std=c++11', '-fno-exceptions', '-Wall', '-Wextra', '-Werror',
        `-fsanitize=${sanitizers}`, '-fno-omit-frame-pointer',
        '-I', path.join(libraries, 'foundling', 'include'),
        '-I', path.join(libraries, 'mymenu', 'include'), file, '-o', executable],
        { encoding: 'utf8' });
    if (compile.error) throw compile.error;
    if (compile.status !== 0) throw new Error(compile.stdout + compile.stderr);
    const run = spawnSync(executable, [], { encoding: 'utf8' });
    process.stdout.write(run.stdout || '');
    process.stderr.write(run.stderr || '');
    if (run.error) throw run.error;
    process.exitCode = run.status === null ? 1 : run.status;
    if (process.exitCode === 0) checkArmLink();
} finally {
    fs.rmSync(directory, { recursive: true, force: true });
}