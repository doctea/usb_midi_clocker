#ifndef MENU_MIDI_MATRIX__INCLUDED
#define MENU_MIDI_MATRIX__INCLUDED

#include "Config.h"
#include "midi/midi_outs.h"
#include "midi/midi_out_wrapper.h"
#include "midi/midi_mapper_matrix_manager.h"

#include "menuitems.h"

class MidiMatrixSelectorControl : /*virtual*/ public SelectorControl<int> {

    // https://ankiewicz.com/color/hex/00cccc <- colour palette picker
    const uint16_t target_colours[MAX_NUM_TARGETS] = {
        0xF800,        //#define ST77XX_RED        
        0xDCDD,        //#define ST77XX_GREEN      
        0x0679,        //#define ST77XX_BLUE       
        0x07FF,        //#define ST77XX_CYAN       
        0xF81F,        //#define ST77XX_MAGENTA   
        0xFFE0,        //#define ST77XX_YELLOW     
        0xFC00,        //#define ST77XX_ORANGE     
        0xF710,        //#define ST77XX_PINK       250 + 250 + 
        (0xF800 + 0x001F)/2,
        (0x07E0 + 0x07FF)/2,
        (0xF81F + 0xFC00)/2,
        (0xFA1F + 0xF800)/2,
        (0x07E0 + 0x001F)/2,
        (0xF800 + 0x07FF)/2,
        (0xF81F + 0xFC00)/2,
        (0xA81F + 0xF877)/2,
        (0x07E0 + 0xFC00)/2,
        (0xF710 + 0xFC00)/2,
        (0xF710 + 0xF877)/2,
        (0xF800 + 0xFC00)/2,
        (0x07E0 + 0x001F)/2,
        (0xF81F + 0xFC00)/3,
        250 + (0x07E0 + 0x001F)/3,
        250 + (0xF800 + 0x07FF)/3,
        0xCE60,     // yellowy
        0xCF1D,     // bluey
        0xA578,     // grey-blue
        0x9E66,     // yellow-green
        0xA670,     // nicey green
        0x8351,     // purprey
    }; 

    uint16_t get_colour_for_target_id(target_id_t target_id) {
        return target_colours[target_id % (sizeof(target_colours)/sizeof(target_colours[0]))];
    }

    // Context panel rows (inline controls to avoid menu-diving)
    enum class PopupRow : uint8_t {
        CONNECT_TOGGLE = 0,
        CHANNEL,
        JUMP_SOURCE,
        JUMP_TARGET,
        DISCONNECT_OTHER_SOURCES_TO_TARGET,
        DISCONNECT_OTHER_TARGETS_FROM_SOURCE,
        _COUNT
    };

    enum class Mode : uint8_t {
        SOURCE_SELECT = 0,
        TARGET_SELECT,
        CONTEXT_PANEL,
    };
    Mode mode = Mode::SOURCE_SELECT;

    // Separate index variables per side so switching modes doesn't cause jumps
    int selected_source_value_index = 0;  // cursor within the source list
    int selected_target_value_index = 0;  // cursor within the target list
    int selected_context_index = 0;       // cursor within context panel

    // These are set when we enter TARGET_SELECT / CONTEXT_PANEL
    source_id_t sel_source = -1;
    target_id_t sel_target = -1;

    char ctx_scratch[40];

    int wrap_index(int idx, int count) const {
        if (count <= 0) return 0;
        while (idx < 0) idx += count;
        while (idx >= count) idx -= count;
        return idx;
    }

    bool popup_row_enabled(PopupRow row) const {
        switch (row) {
            case PopupRow::CONNECT_TOGGLE:
            case PopupRow::CHANNEL:
                return sel_source >= 0 && sel_target >= 0;
            case PopupRow::JUMP_SOURCE:
                return sel_source >= 0 && midi_matrix_manager->get_source_page_index(sel_source) >= 0;
            case PopupRow::JUMP_TARGET:
                return sel_target >= 0 && midi_matrix_manager->get_target_page_index(sel_target) >= 0;
            case PopupRow::DISCONNECT_OTHER_SOURCES_TO_TARGET:
                return sel_target >= 0 && midi_matrix_manager->connected_to_target_count(sel_target) > 1;
            case PopupRow::DISCONNECT_OTHER_TARGETS_FROM_SOURCE:
                return sel_source >= 0 && midi_matrix_manager->connected_to_source_count(sel_source) > 1;
            default:
                return false;
        }
    }

    const char *popup_row_label(PopupRow row) {
        const bool conn = (sel_source >= 0 && sel_target >= 0) ? midi_matrix_manager->is_connected(sel_source, sel_target) : false;
        const auto *pol = midi_matrix_manager->get_connection_policy(sel_source, sel_target);
        switch (row) {
            case PopupRow::CONNECT_TOGGLE:
                return conn ? "Connected: yes" : "Connected: no";
            case PopupRow::CHANNEL:
                if (pol) {
                    if (pol->fixed_channel > 0)
                        snprintf(ctx_scratch, sizeof(ctx_scratch), "Channel: %2u", pol->fixed_channel);
                    else
                        snprintf(ctx_scratch, sizeof(ctx_scratch), "Channel: --");
                    return ctx_scratch;
                }
                return "Channel: --";
            case PopupRow::JUMP_SOURCE:
                return "Jump to source page";
            case PopupRow::JUMP_TARGET:
                return "Jump to target page";
            case PopupRow::DISCONNECT_OTHER_SOURCES_TO_TARGET:
                return "Disconnect other sources->target";
            case PopupRow::DISCONNECT_OTHER_TARGETS_FROM_SOURCE:
                return "Disconnect other targets<-source";
            default:
                return "";
        }
    }

    void popup_disconnect_others_to_target() {
        if (sel_target < 0) return;
        for (source_id_t s = 0; s < midi_matrix_manager->sources_count; s++) {
            if (s == sel_source) continue;
            if (midi_matrix_manager->is_connected(s, sel_target))
                midi_matrix_manager->disconnect(s, sel_target);
        }
    }

    void popup_disconnect_others_from_source() {
        if (sel_source < 0) return;
        for (target_id_t t = 0; t < midi_matrix_manager->targets_count; t++) {
            if (t == sel_target) continue;
            if (midi_matrix_manager->is_connected(sel_source, t))
                midi_matrix_manager->disconnect(sel_source, t);
        }
    }

    void enter_context_panel() {
        selected_context_index = 0;
        selected_value_index = 0;
        mode = Mode::CONTEXT_PANEL;
    }

    // Returns true when button_right is meaningful given current state
    bool right_available() const {
        if (mode == Mode::SOURCE_SELECT) {
            // Can go to source page for the currently highlighted source.
            int src = selected_source_value_index;
            return src >= 0 && src < (int)midi_matrix_manager->sources_count
                && midi_matrix_manager->get_source_page_index((source_id_t)src) >= 0;
        }
        if (mode == Mode::TARGET_SELECT) {
            // popup is always useful in target mode (connect/policy/disconnect-others)
            return sel_source >= 0;
        }
        return false;
    }

public:
    int actual_value_index = 0;
    int selected_source_index = -1;  // kept for display() compat; -1 = in source-select mode

    MidiMatrixSelectorControl(const char *label) : SelectorControl(label, 0) {};

    virtual const char* get_label_for_index(int index) {
        if (selected_source_index==-1)    
            return midi_matrix_manager->get_label_for_source_id(index);
        else    
            return midi_matrix_manager->get_label_for_target_id(index);
    }

    virtual void setter(int new_value) override {
        if (mode == Mode::SOURCE_SELECT) {
            selected_source_value_index = new_value;
            actual_value_index = new_value;
            selected_value_index = new_value;
            // Commit: enter target-select mode
            sel_source = (source_id_t)new_value;
            selected_source_index = new_value;
            mode = Mode::TARGET_SELECT;
            // Restore target cursor, clamped
            int tgt_count = (int)midi_matrix_manager->targets_count;
            if (selected_target_value_index >= tgt_count && tgt_count > 0)
                selected_target_value_index = tgt_count - 1;
            selected_value_index = selected_target_value_index;
        } else if (mode == Mode::TARGET_SELECT) {
            selected_target_value_index = new_value;
            selected_value_index = new_value;
            sel_target = (target_id_t)new_value;
            // Toggle the connection
            char msg[MENU_MESSAGE_MAX];
            if (midi_matrix_manager->toggle_connect(selected_source_index, new_value)) {
                snprintf(msg, MENU_MESSAGE_MAX, "%-5.18s <-> %-5.18s",
                    midi_matrix_manager->get_label_for_source_id(selected_source_index),
                    midi_matrix_manager->get_label_for_target_id(new_value));
            } else {
                snprintf(msg, MENU_MESSAGE_MAX, "%-5.18s </> %-5.18s",
                    midi_matrix_manager->get_label_for_source_id(selected_source_index),
                    midi_matrix_manager->get_label_for_target_id(new_value));
            }
            menu_set_last_message(msg, GREEN);
        } else if (mode == Mode::CONTEXT_PANEL) {
            selected_context_index = new_value;
        }
    }

    virtual int getter() override {
        return selected_value_index;
    }

    virtual int get_num_available() {
        if (mode == Mode::SOURCE_SELECT)  return midi_matrix_manager->sources_count;
        if (mode == Mode::TARGET_SELECT)  return midi_matrix_manager->targets_count;
        if (mode == Mode::CONTEXT_PANEL)  return (int)PopupRow::_COUNT;
        return 0;
    }

    virtual bool knob_left() override {
        if (mode == Mode::SOURCE_SELECT) {
            int count = get_num_available();
            selected_source_value_index = wrap_index(selected_source_value_index - 1, count);
            selected_value_index = selected_source_value_index;
            actual_value_index = selected_source_value_index;
            return true;
        }
        if (mode == Mode::TARGET_SELECT) {
            int count = get_num_available();
            selected_target_value_index = wrap_index(selected_target_value_index - 1, count);
            selected_value_index = selected_target_value_index;
            return true;
        }
        if (mode == Mode::CONTEXT_PANEL) {
            int count = get_num_available();
            selected_context_index = wrap_index(selected_context_index - 1, count);
            selected_value_index = selected_context_index;
            return true;
        }
        return true;
    }

    virtual bool knob_right() override {
        if (mode == Mode::SOURCE_SELECT) {
            int count = get_num_available();
            selected_source_value_index = wrap_index(selected_source_value_index + 1, count);
            selected_value_index = selected_source_value_index;
            actual_value_index = selected_source_value_index;
            return true;
        }
        if (mode == Mode::TARGET_SELECT) {
            int count = get_num_available();
            selected_target_value_index = wrap_index(selected_target_value_index + 1, count);
            selected_value_index = selected_target_value_index;
            return true;
        }
        if (mode == Mode::CONTEXT_PANEL) {
            int count = get_num_available();
            selected_context_index = wrap_index(selected_context_index + 1, count);
            selected_value_index = selected_context_index;
            return true;
        }
        return true;
    }

    virtual bool button_select() override {
        if (mode == Mode::SOURCE_SELECT) {
            setter(selected_value_index);
            return false;  // don't close
        }
        if (mode == Mode::TARGET_SELECT) {
            setter(selected_value_index);
            return false;
        }
        if (mode == Mode::CONTEXT_PANEL) {
            selected_context_index = wrap_index(selected_value_index, get_num_available());
            if (selected_context_index < 0 || selected_context_index >= get_num_available())
                return false;
            PopupRow row = (PopupRow)selected_context_index;
            auto *pol = midi_matrix_manager->get_connection_policy_mut(sel_source, sel_target);
            if (pol == nullptr)
                return false;

            switch (row) {
                case PopupRow::CONNECT_TOGGLE:
                    midi_matrix_manager->toggle_connect(sel_source, sel_target);
                    break;
                case PopupRow::CHANNEL:
                    // Cycle: 0 (passthru) -> 1 -> 2 -> ... -> 16 -> 0
                    pol->fixed_channel = (pol->fixed_channel >= 16) ? 0 : pol->fixed_channel + 1;
                    break;
                case PopupRow::JUMP_SOURCE: {
                    if (!popup_row_enabled(row)) break;
                    int pg = midi_matrix_manager->get_source_page_index(sel_source);
                    if (pg >= 0) menu->jump_to_page_with_return(pg);
                    break;
                }
                case PopupRow::JUMP_TARGET: {
                    if (!popup_row_enabled(row)) break;
                    int pg = midi_matrix_manager->get_target_page_index(sel_target);
                    if (pg >= 0) menu->jump_to_page_with_return(pg);
                    break;
                }
                case PopupRow::DISCONNECT_OTHER_SOURCES_TO_TARGET:
                    if (popup_row_enabled(row)) popup_disconnect_others_to_target();
                    break;
                case PopupRow::DISCONNECT_OTHER_TARGETS_FROM_SOURCE:
                    if (popup_row_enabled(row)) popup_disconnect_others_from_source();
                    break;
                default: break;
            }
            return false;
        }
        return flags.go_back_on_select;
    }

    virtual bool button_right() override {
        if (mode == Mode::SOURCE_SELECT) {
            // Update sel_source to the currently highlighted source
            sel_source = (source_id_t)selected_value_index;
            sel_target = -1;
            enter_context_panel();
            return true;
        }
        if (mode == Mode::TARGET_SELECT) {
            sel_target = (target_id_t)selected_value_index;
            enter_context_panel();
            return true;
        }
        return false;
    }

    virtual bool button_back() override {
        if (mode == Mode::CONTEXT_PANEL) {
            mode = Mode::TARGET_SELECT;
            selected_value_index = selected_target_value_index;
            return true;
        }
        if (mode == Mode::TARGET_SELECT) {
            mode = Mode::SOURCE_SELECT;
            sel_source = -1;
            sel_target = -1;
            selected_source_index = -1;
            selected_value_index = selected_source_value_index;
            actual_value_index = selected_source_value_index;
            return true;
        }
        return false;  // exit to top menu
    }

    // ---- display helpers ----
    void display_context_panel(Coord pos) {
        tft->setTextSize(1);
        tft->setTextColor(C_WHITE, BLACK);
        const char *src_lbl = midi_matrix_manager->get_label_for_source_id(sel_source);
        const char *tgt_lbl = (sel_target >= 0) ? midi_matrix_manager->get_label_for_target_id(sel_target) : "--";
        tft->printf("%.20s ", src_lbl);
        tft->setTextColor(C_WHITE, BLACK);
        tft->printf("-> %.17s\n", tgt_lbl);
        tft->setTextColor(this->get_colour_for_target_id(sel_target), BLACK);
        tft->printf("-----\n");
        selected_context_index = wrap_index(selected_value_index, get_num_available());

        for (int i = 0; i < get_num_available(); i++) {
            PopupRow row = (PopupRow)i;
            const bool enabled = popup_row_enabled(row);
            uint16_t fg = enabled ? C_WHITE : tft->halfbright_565(C_WHITE);
            colours(selected_context_index == i, fg, BLACK);
            tft->printf("  %-36.36s\n", popup_row_label(row));
        }
        colours(false, C_WHITE, BLACK);
        tft->setTextColor(tft->halfbright_565(C_WHITE), BLACK);
        tft->println("[turn]=scroll  [sel]=act/cycle  [back]=close");
    }

    // classic fixed display version
    virtual int display(Coord pos, bool selected, bool opened) override {
        pos.y = header(label, pos, selected, opened);
        num_values = this->get_num_available();
        tft->setTextSize(1);

        // ---- Full-screen panel modes ----
        if (opened && mode == Mode::CONTEXT_PANEL) {
            display_context_panel(pos);
            // [>] hint not needed in panel — physical button closes it (via button_back)
            return tft->getCursorY();
        }

        // ---- Normal matrix view ----
        int source_position[midi_matrix_manager->sources_count];
        int target_position[midi_matrix_manager->targets_count];

        byte source_processed_count[midi_matrix_manager->sources_count];
        byte target_processed_count[midi_matrix_manager->targets_count];
        memset(source_processed_count, 0, midi_matrix_manager->sources_count);
        memset(target_processed_count, 0, midi_matrix_manager->targets_count);

        bool opened_on_source = opened && mode == Mode::SOURCE_SELECT;
        bool opened_on_target = opened && mode == Mode::TARGET_SELECT;
        const source_id_t relevant_source_id = opened_on_source ? selected_source_value_index
                                                                 : selected_source_index >= 0 ? (source_id_t)selected_source_index : 0;

        int available_rows = (tft->height() - pos.y) / tft->getRowHeight();

        // Draw current/last note for the highlighted target
        if (opened_on_target) {
            MIDIOutputWrapper *w = midi_matrix_manager->get_target_for_id((target_id_t)selected_target_value_index);
            if (w) {
                tft->setTextColor(this->get_colour_for_target_id(selected_target_value_index), BLACK);
                tft->printf("Current: %3s   Last: %3s\n", 
                    (char*)get_note_name_c(w->current_note, w->default_channel),
                    (char*)get_note_name_c(w->last_note, w->default_channel)
                );
            }
            pos.y = tft->getCursorY();
        }

        int lowest_y = pos.y;
        uint16_t halfbright_white = tft->halfbright_565(C_WHITE);

        // Source column (left)
        for (source_id_t source_id = 0; source_id < midi_matrix_manager->sources_count; source_id++) {
            source_id_t source_id_actual = (source_id + relevant_source_id + (available_rows*2)) % midi_matrix_manager->sources_count;
            const bool is_highlighted = opened_on_source && source_id_actual == selected_source_value_index;

            int col = !opened || opened_on_source ? C_WHITE : halfbright_white;
            if (opened_on_target && source_id_actual == (source_id_t)selected_source_index)
                col = GREEN;
            else if (opened_on_target && midi_matrix_manager->is_connected(source_id_actual, (target_id_t)selected_target_value_index))
                col = this->get_colour_for_target_id(selected_target_value_index);

            bool show_right_hint = is_highlighted && right_available();
            colours(is_highlighted, col, BLACK);
            tft->printf("%13s", (char*)midi_matrix_manager->get_label_for_source_id(source_id_actual));
            tft->println();
            source_position[source_id_actual] = tft->getCursorY() - (tft->getRowHeight()/2);
        }
        lowest_y = tft->getCursorY();

        // Target column (right)
        tft->setCursor(0, pos.y);
        target_id_t selected_target_scroll = opened_on_target ? (target_id_t)selected_target_value_index : 0;

        int y = pos.y;
        for (target_id_t target_id = 0; target_id < midi_matrix_manager->targets_count; target_id++) {
            target_id_t target_id_actual = (target_id + selected_target_scroll + (available_rows*2)) % midi_matrix_manager->targets_count;

            const uint16_t target_colour = this->get_colour_for_target_id(target_id_actual);
            const uint16_t half_target_colour = tft->halfbright_565(target_colour);
            const bool connected_to_relevant = (opened_on_source || opened_on_target)
                                              && midi_matrix_manager->is_connected(relevant_source_id, target_id_actual);
            const bool is_highlighted_target = opened_on_target && target_id_actual == (target_id_t)selected_target_value_index;

            bool show_right_hint_tgt = is_highlighted_target && right_available();

            colours(
                is_highlighted_target,
                !opened || connected_to_relevant || is_highlighted_target ? target_colour : half_target_colour, 
                BLACK
            ); 
            
            tft->setCursor((tft->width()/2), y);
            {
                const char *tgt_base = midi_matrix_manager->get_label_for_target_id(target_id_actual);
                // Policy indicator: only when connected, fixed-channel only, shown LEFT of label
                const bool connected_here = selected_source_index >= 0
                    && midi_matrix_manager->is_connected(selected_source_index, target_id_actual);
                if (connected_here && selected_source_index >= 0) {
                    const auto *pol = midi_matrix_manager->get_connection_policy(selected_source_index, target_id_actual);
                    if (pol && pol->fixed_channel > 0)
                        snprintf(ctx_scratch, sizeof(ctx_scratch), "[%2u]%-14.14s", pol->fixed_channel, tgt_base);
                    else
                        snprintf(ctx_scratch, sizeof(ctx_scratch), "%-19.19s", tgt_base);
                    tgt_base = ctx_scratch;
                }
                tft->printf("%-19.19s", tgt_base);
            }

            target_position[target_id_actual] = tft->getCursorY();
            tft->println();

            // Draw connection lines
            for (source_id_t source_id = 0; source_id < midi_matrix_manager->sources_count; source_id++) {
                const source_id_t source_id_actual = (source_id + relevant_source_id + (available_rows*2)) % midi_matrix_manager->sources_count;
                if (!midi_matrix_manager->is_connected(source_id_actual, target_id_actual)) continue;

                const bool is_opened_target_and_relevant = opened_on_target && relevant_source_id == source_id_actual;
                uint16_t line_colour;
                if (is_opened_target_and_relevant)
                    line_colour = GREEN;
                else {
                    const bool use_full = !opened ||
                        (opened_on_source && relevant_source_id == source_id_actual) ||
                        (opened_on_target && target_id_actual == (target_id_t)selected_target_value_index);
                    line_colour = use_full ? target_colour : half_target_colour;
                }

                int pixels_per_source = source_processed_count[source_id_actual] % tft->getRowHeight();
                int pixels_per_target = target_processed_count[target_id_actual] % tft->getRowHeight();
                int source_y_offset = pixels_per_source + (tft->getRowHeight()/4);
                int target_y_offset = pixels_per_target + (tft->getRowHeight()/4);
                tft->drawLine(tft->characterWidth()*13, source_position[source_id_actual]+source_y_offset,
                              tft->characterWidth()*14, source_position[source_id_actual]+source_y_offset, line_colour);
                tft->drawLine(tft->characterWidth()*14, source_position[source_id_actual]+source_y_offset,
                              (tft->width()/2)-(tft->characterWidth()*2), target_position[target_id_actual]+target_y_offset, line_colour);
                tft->drawLine((tft->width()/2)-(tft->characterWidth()*2), target_position[target_id_actual]+target_y_offset,
                              (tft->width()/2)-(tft->characterWidth()*1), target_position[target_id_actual]+target_y_offset, line_colour);

                source_processed_count[source_id_actual]++;
                target_processed_count[target_id_actual]++;
            }
            y = tft->getCursorY();
        }
        if (y > lowest_y) lowest_y = y;
        if (tft->getCursorX() > 0) tft->println((char*)"");

        if (opened && right_available() && (mode == Mode::SOURCE_SELECT || mode == Mode::TARGET_SELECT)) {
            tft->setTextSize(1);
            const int hint_w = tft->characterWidth() * 3;
            // getRowHeight() = singleRowHeight*(1+size) but actual rendered height = singleRowHeight*size,
            // so use the latter to land on the true last row.
            const int hint_h = tft->getSingleRowHeight() * max(1, tft->getTextSize());
            tft->setCursor(tft->width() - hint_w, tft->height() - hint_h);
            tft->setTextColor(tft->halfbright_565(C_WHITE), BLACK);
            tft->print("[>]");
            colours(false, C_WHITE, BLACK);
        }

        return lowest_y;
    }
};

#endif
