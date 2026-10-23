const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const root = path.resolve(__dirname, '..');
const mymenuInclude = path.resolve(root, '..', 'my_libraries', 'mymenu', 'include');

const source = `
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include "menuitems_actionlist_core.h"

struct Row { std::string text; bool selectable; bool enabled; };

class TestSource : public ActionListSource {
public:
    std::vector<Row> rows;
    std::vector<int> selected;
    int enters = 0;
    ActionListResult result = ActionListResult::STAY;
    ActionListSource *push_on_select = nullptr;
    bool right_handled = true;
    int rights = 0;

    uint8_t count() override { return (uint8_t)rows.size(); }
    void row_text(uint8_t index, char *out, size_t size) override { snprintf(out, size, "%s", rows[index].text.c_str()); }
    bool row_selectable(uint8_t index) override { return rows[index].selectable; }
    bool row_enabled(uint8_t index) override { return rows[index].enabled; }
    void on_enter() override { ++enters; }
    ActionListResult on_select(uint8_t index, ActionListNavigator &nav) override {
        selected.push_back(index);
        if (push_on_select != nullptr) nav.push(push_on_select);
        return result;
    }
    bool on_right(uint8_t, ActionListNavigator &) override { ++rights; return right_handled; }
};

static TestSource make(std::initializer_list<Row> rows) { TestSource s; s.rows = rows; return s; }

static void cursor_skips_headings_and_wraps() {
    TestSource list = make({{"Inputs", false, true}, {"a", true, true}, {"b", true, true}, {"Outputs", false, true}, {"c", true, true}});
    ActionListNavigator nav;
    nav.reset(&list);
    assert(list.enters == 1);
    assert(nav.cursor() == 1);
    nav.move(1); assert(nav.cursor() == 2);
    nav.move(1); assert(nav.cursor() == 4);
    nav.move(1); assert(nav.cursor() == 1);
    nav.move(-1); assert(nav.cursor() == 4);
}

static void empty_and_unselectable_lists() {
    TestSource empty;
    ActionListNavigator nav;
    nav.reset(&empty);
    assert(nav.cursor() == ActionListNavigator::NO_ROW);
    assert(!nav.move(1));
    assert(!nav.select());
    assert(!nav.right());

    TestSource info = make({{"nothing here", false, true}});
    nav.reset(&info);
    assert(nav.cursor() == ActionListNavigator::NO_ROW);
    assert(!nav.select());
    assert(info.selected.empty());
}

static void disabled_rows_are_reachable_but_inert() {
    TestSource list = make({{"a", true, false}, {"b", true, true}});
    ActionListNavigator nav;
    nav.reset(&list);
    assert(nav.cursor() == 0);
    assert(!nav.select());
    assert(!nav.right());
    assert(list.selected.empty() && list.rights == 0);
    nav.move(1);
    assert(!nav.select());
    assert(list.selected.size() == 1 && list.selected[0] == 1);
    assert(nav.right() && list.rights == 1);
}

static void push_pop_and_back() {
    TestSource parent = make({{"a", true, true}, {"b", true, true}});
    TestSource child = make({{"Heading", false, true}, {"x", true, true}});
    parent.push_on_select = &child;
    ActionListNavigator nav;
    nav.reset(&parent);
    nav.move(1);
    assert(nav.cursor() == 1);
    assert(!nav.select());
    assert(nav.get_depth() == 2 && nav.current() == &child);
    assert(nav.cursor() == 1 && child.enters == 1);

    assert(nav.back());
    assert(nav.get_depth() == 1 && nav.current() == &parent);
    assert(nav.cursor() == 1);
    assert(parent.enters == 2);
    assert(!nav.back());
    assert(nav.get_depth() == 1);
}

static void select_results() {
    TestSource parent = make({{"a", true, true}});
    TestSource child = make({{"x", true, true}});
    child.result = ActionListResult::POP;
    parent.push_on_select = &child;
    ActionListNavigator nav;
    nav.reset(&parent);
    assert(!nav.select());
    assert(nav.get_depth() == 2);
    assert(!nav.select());
    assert(nav.get_depth() == 1 && nav.current() == &parent);

    // POP at the root closes the item
    parent.push_on_select = nullptr;
    parent.result = ActionListResult::POP;
    assert(nav.select());

    parent.result = ActionListResult::CLOSE;
    assert(nav.select());

    // a source that pushes and returns POP keeps the newly pushed level
    parent.push_on_select = &child;
    parent.result = ActionListResult::POP;
    assert(!nav.select());
    assert(nav.get_depth() == 2 && nav.current() == &child);
}

static void depth_limit() {
    TestSource list = make({{"a", true, true}});
    ActionListNavigator nav;
    nav.reset(&list);
    for (uint8_t i = 1; i < ActionListNavigator::MAX_DEPTH; ++i) assert(nav.push(&list));
    assert(!nav.push(&list));
    assert(!nav.push(nullptr));
    assert(nav.get_depth() == ActionListNavigator::MAX_DEPTH);
}

static void clamp_after_rows_change() {
    TestSource list = make({{"H", false, true}, {"a", true, true}, {"b", true, true}, {"c", true, true}});
    ActionListNavigator nav;
    nav.reset(&list);
    nav.move(-1);
    assert(nav.cursor() == 3);
    list.rows.pop_back();
    list.rows.pop_back();
    nav.clamp();
    assert(nav.cursor() == 1);
    list.rows.pop_back();
    nav.clamp();
    assert(nav.cursor() == ActionListNavigator::NO_ROW);
    list.rows.clear();
    nav.clamp();
    assert(nav.cursor() == ActionListNavigator::NO_ROW && nav.scroll() == 0);
}

static void clamp_after_select_removes_row() {
    struct Removing : TestSource {
        ActionListResult on_select(uint8_t index, ActionListNavigator &) override {
            rows.erase(rows.begin() + index);
            return ActionListResult::STAY;
        }
    } list;
    list.rows = {{"a", true, true}, {"b", true, true}};
    ActionListNavigator nav;
    nav.reset(&list);
    nav.move(1);
    assert(nav.cursor() == 1);
    assert(!nav.select());
    assert(nav.cursor() == 0);
}

static void scrolling() {
    TestSource list;
    for (int i = 0; i < 20; ++i) list.rows.push_back({"r", true, true});
    ActionListNavigator nav;
    nav.reset(&list);
    nav.ensure_visible(5);
    assert(nav.scroll() == 0);
    for (int i = 0; i < 12; ++i) nav.move(1);
    nav.ensure_visible(5);
    assert(nav.cursor() == 12 && nav.scroll() == 8);
    for (int i = 0; i < 10; ++i) nav.move(-1);
    nav.ensure_visible(5);
    assert(nav.cursor() == 2 && nav.scroll() == 0);
    nav.move(-1); nav.move(-1); nav.move(-1);
    nav.ensure_visible(5);
    assert(nav.cursor() == 19 && nav.scroll() == 15);
    list.rows.resize(3);
    nav.clamp();
    nav.ensure_visible(5);
    assert(nav.cursor() == 2 && nav.scroll() == 0);
}

int main() {
    cursor_skips_headings_and_wraps();
    empty_and_unselectable_lists();
    disabled_rows_are_reachable_but_inert();
    push_pop_and_back();
    select_results();
    depth_limit();
    clamp_after_rows_change();
    clamp_after_select_removes_row();
    scrolling();
    return 0;
}
`;

const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'actionlist-test-'));
try {
    const file = path.join(directory, 'test.cpp');
    const executable = path.join(directory, process.platform === 'win32' ? 'test.exe' : 'test');
    fs.writeFileSync(file, source);
    const compile = spawnSync('g++', ['-std=c++11', '-Wall', '-Wextra', '-Werror',
        '-fsanitize=undefined', '-I', mymenuInclude, file, '-o', executable],
    { encoding: 'utf8' });
    if (compile.error) throw compile.error;
    if (compile.status !== 0) throw new Error(compile.stdout + compile.stderr);
    const run = spawnSync(executable, [], { encoding: 'utf8' });
    if (run.error) throw run.error;
    if (run.status !== 0) throw new Error(run.stdout + run.stderr);
    console.log('ActionList navigator passed: heading skip/wrap, empty lists, disabled rows, push/pop/back, select results, depth limit, clamp, scrolling');
} finally {
    for (const name of fs.readdirSync(directory)) fs.unlinkSync(path.join(directory, name));
    fs.rmdirSync(directory);
}
