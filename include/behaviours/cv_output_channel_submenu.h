#pragma once

// Per-channel sub-menu for DeviceBehaviour_CVOutput.
// Each instance shows all per-channel controls for one CV output channel:
//   Pooled toggle, Gate toggle, current note display, per-channel note limits,
//   Slew toggle, Slew rate, CV polarity/invert settings, and modulation (LowMemory controls).
//
// Shared lowmemory controls (parameter_amount_controls + slot_rows) must be initialised
// ONCE in make_menu_items() before any CVOutputChannelSubMenuItem is constructed,
// then the same pointers are added to every channel SubMenuItem.

#ifdef ENABLE_SCREEN

#include "submenuitem.h"
#include "submenuitem_bar.h"
#include "menuitems_lambda.h"

#include "mymenu/menuitems_scale.h"
#include "mymenu/menuitem_notelimit.h"
#include "mymenu/menuitems_harmony.h"
#include "mymenu_items/ParameterMenuItems_lowmemory.h"

#include "midi_helpers.h"
#include "parameters/CVOutputParameter.h"

// Two-line column item for SubMenuItemBar: renders a header-label value line, then a second
// smaller annotation line below it (e.g. note range on line 1, mode names on line 2).
class TwoLineCallbackMenuItem : public CallbackMenuItem {
    using label_cb  = vl::Func<const char*(void)>;
    using colour_cb = vl::Func<uint16_t(void)>;
    label_cb  line2_cb_;
    colour_cb colour2_cb_;
public:
    TwoLineCallbackMenuItem(
        const char *label,
        label_cb  line1_cb,  colour_cb colour1_cb,
        label_cb  line2_cb,  colour_cb colour2_cb
    ) : CallbackMenuItem(label, line1_cb, colour1_cb),
        line2_cb_(line2_cb), colour2_cb_(colour2_cb) {}

    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        int saved_x = tft->getCursorX();  // remember column X so line 2 aligns correctly
        CallbackMenuItem::renderValue(selected, opened, max_character_width);
        const char *txt2 = line2_cb_();
        if (txt2 != nullptr && txt2[0] != '\0') {
            tft->setCursor(saved_x, tft->getCursorY());  // restore column X after println from line 1
            this->colours(selected, colour2_cb_(), BLACK);
            int ts = tft->get_textsize_for_width(txt2, max_character_width * tft->characterWidth());
            tft->setTextSize(ts);
            tft->println(txt2);
        }
        return tft->getCursorY();
    }
};

// Ensure shared lowmemory controls (parameter_amount_controls + slot_rows) exist.
// Safe to call multiple times; creates missing controls on first call only.
// Does NOT add them to any menu — the caller does that per-channel.
inline void ensure_shared_lowmemory_controls() {
    if (lowmemory_controls.parameter_amount_controls == nullptr)
        lowmemory_controls.parameter_amount_controls = new ParameterMenuItem("Amounts", &lowmemory_controls.parameter, false);

    for (uint_fast8_t i = 0; i < MAX_SLOT_CONNECTIONS; i++) {
        if (lowmemory_controls.slot_rows[i] == nullptr) {
            char slot_label[8];
            snprintf(slot_label, sizeof(slot_label), "Slt%i", (int)(i + 1));
            lowmemory_controls.slot_rows[i] = new ParameterModSlotRow(slot_label, &lowmemory_controls.parameter, i);
        }
    }
}

template<class DACClass>
class CVOutputChannelSubMenuItem : public SubMenuItem {
    public:
        CVOutputChannelSubMenuItem(
            const char *label,
            CVOutputParameter<DACClass> *output,
            bool *pool_flag,
            int8_t *last_note,
            int8_t *lowest_note,
            int8_t *highest_note,
            NOTE_LIMIT_MODE *lowest_mode,
            NOTE_LIMIT_MODE *highest_mode,
            ParameterList *per_channel_params,
            int8_t *effective_lowest_note,
            int8_t *effective_highest_note,
            float  *slew_base_normal, 
            bool show_header = true,
            bool scrollable = true
        ) : SubMenuItem(label, show_header, scrollable),
            output_(output), pool_flag_(pool_flag), last_note_(last_note),
            lowest_note_(lowest_note), highest_note_(highest_note),
            lowest_mode_(lowest_mode), highest_mode_(highest_mode),
            per_channel_params_(per_channel_params),
            effective_lowest_note_(effective_lowest_note),
            effective_highest_note_(effective_highest_note),
            slew_base_normal_(slew_base_normal)
        {
            build_items();
            build_summary_bar();
        }

        virtual void on_add() override {
            SubMenuItem::on_add();
            // Propagate tft and behaviour colour to the summary bar and its items (not in main item tree)
            if (summary_bar_ != nullptr) {
                if (tft != nullptr) {
                    summary_bar_->tft = tft;
                    for (uint_fast8_t j = 0; j < summary_bar_->items->size(); j++)
                        summary_bar_->items->get(j)->tft = tft;
                }
                summary_bar_->set_default_colours(this->default_fg, this->default_bg);
                for (uint_fast8_t j = 0; j < summary_bar_->items->size(); j++)
                    summary_bar_->items->get(j)->set_default_colours(this->default_fg, this->default_bg);
            }
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            if (opened || this->always_show)
                return SubMenuItem::display(pos, selected, opened);

            // Collapsed: render channel header then delegate to the summary bar
            tft->setCursor(pos.x, pos.y);
            int y = header(this->label, pos, selected, opened);
            if (summary_bar_ != nullptr)
                return summary_bar_->display(Coord(0, y), selected, false);
            return y;
        }

    private:
        CVOutputParameter<DACClass> *output_   = nullptr;
        bool            *pool_flag_    = nullptr;
        int8_t          *last_note_    = nullptr;
        int8_t          *lowest_note_  = nullptr;
        int8_t          *highest_note_ = nullptr;
        NOTE_LIMIT_MODE *lowest_mode_  = nullptr;
        NOTE_LIMIT_MODE *highest_mode_ = nullptr;
        ParameterList   *per_channel_params_   = nullptr;
        int8_t          *effective_lowest_note_  = nullptr;
        int8_t          *effective_highest_note_ = nullptr;
        float           *slew_base_normal_       = nullptr;
        SubMenuItemBar  *summary_bar_  = nullptr;

        // Build a headerless SubMenuItemBar of CallbackMenuItem columns shown when collapsed.
        void build_summary_bar() {
            summary_bar_ = new SubMenuItemBar("", true, false);  // show_sub_headers=true, show_header=false

            // Col 1: current note if playing, else last note (dim), else "---"
            summary_bar_->add(new CallbackMenuItem("Note",
                [=]() -> const char* {
                    if (is_valid_note(output_->current_pitch_note))
                        return get_note_name_c(output_->current_pitch_note);
                    if (last_note_ && is_valid_note(*last_note_))
                        return get_note_name_c(*last_note_);
                    return "---";
                },
                [=]() -> uint16_t {
                    if (output_->is_note_active()) return YELLOW;
                    if (last_note_ && is_valid_note(*last_note_))   return DARK_YELLOW;
                    return GREY;
                }
            ));

            // Col 2: pool membership
            summary_bar_->add(new CallbackMenuItem("Pool",
                [=]() -> const char* { return (pool_flag_ && *pool_flag_) ? "On" : "Off"; },
                [=]() -> uint16_t    { return (pool_flag_ && *pool_flag_) ? GREEN : GREY;  }
            ));

            // Col 3: gate output
            summary_bar_->add(new CallbackMenuItem("Gate",
                [=]() -> const char* { return output_->get_gate_output_enabled() ? "On" : "Off"; },
                [=]() -> uint16_t    { return output_->get_gate_output_enabled() ? GREEN : GREY;  }
            ));

            // Col 4: slew — shows post-modulation effective slew rate
            summary_bar_->add(new CallbackMenuItem("Slew",
                [=]() -> const char* {
                    static char buf[8];
                    if (output_->slew_enabled)
                        snprintf(buf, sizeof(buf), "%3d%%", (int)(output_->effective_slew_rate_normal * 100.0f + 0.5f));
                    else
                        strncpy(buf, "Off", sizeof(buf));
                    return buf;
                },
                [=]() -> uint16_t { return output_->slew_enabled ? ORANGE : GREY; }
            ));

            // Col 5: note limits — line 1 = NNN>NNN range, line 2 = per-limit wrap/drop modes
            // Shows post-modulation (effective) values.
            // Format: 3-char left-aligned note name (or "---" for open-ended) separated by ">".
            summary_bar_->add(new TwoLineCallbackMenuItem("Limit",
                [=]() -> const char* {
                    static char buf[12];
                    const int8_t lo = effective_lowest_note_  ? *effective_lowest_note_  : (lowest_note_  ? *lowest_note_  : MIDI_MIN_NOTE);
                    const int8_t hi = effective_highest_note_ ? *effective_highest_note_ : (highest_note_ ? *highest_note_ : MIDI_MAX_NOTE);
                    const char *ls = (lo > MIDI_MIN_NOTE) ? get_note_name_c(lo) : "---";
                    const char *hs = (hi < MIDI_MAX_NOTE) ? get_note_name_c(hi) : "---";
                    snprintf(buf, sizeof(buf), "%-3s>%-3s", ls, hs);
                    return buf;
                },
                [=]() -> uint16_t {
                    const int8_t lo = effective_lowest_note_  ? *effective_lowest_note_  : MIDI_MIN_NOTE;
                    const int8_t hi = effective_highest_note_ ? *effective_highest_note_ : MIDI_MAX_NOTE;
                    return (lo > MIDI_MIN_NOTE || hi < MIDI_MAX_NOTE) ? ORANGE : GREY;
                },
                [=]() -> const char* {
                    static char buf[12];
                    const int8_t lo = effective_lowest_note_  ? *effective_lowest_note_  : MIDI_MIN_NOTE;
                    const int8_t hi = effective_highest_note_ ? *effective_highest_note_ : MIDI_MAX_NOTE;
                    const bool has_lo = (lo > MIDI_MIN_NOTE);
                    const bool has_hi = (hi < MIDI_MAX_NOTE);
                    if (has_lo || has_hi) {
                        const char *lm = (lowest_mode_  && *lowest_mode_  == NOTE_LIMIT_MODE::TRANSPOSE) ? "Wrp" : "Drp";
                        const char *hm = (highest_mode_ && *highest_mode_ == NOTE_LIMIT_MODE::TRANSPOSE) ? "Wrp" : "Drp";
                        snprintf(buf, sizeof(buf), "%s %s", lm, hm);
                    } else {
                        buf[0] = '\0';
                    }
                    return buf;
                },
                [=]() -> uint16_t {
                    const int8_t lo = effective_lowest_note_  ? *effective_lowest_note_  : MIDI_MIN_NOTE;
                    const int8_t hi = effective_highest_note_ ? *effective_highest_note_ : MIDI_MAX_NOTE;
                    return (lo > MIDI_MIN_NOTE || hi < MIDI_MAX_NOTE) ? ORANGE : GREY;
                }
            ));

            // Col 6: modulation — lights up if any per-channel parameter has an active modulation slot
            summary_bar_->add(new CallbackMenuItem("Mod",
                [=]() -> const char* {
                    if (per_channel_params_) {
                        for (auto* p : *per_channel_params_)
                            if (p && p->has_active_modulation()) return "Y";
                    }
                    return "N";
                },
                [=]() -> uint16_t {
                    if (per_channel_params_) {
                        for (auto* p : *per_channel_params_)
                            if (p && p->has_active_modulation()) return PURPLE;
                    }
                    return GREY;
                }
            ));
        }

        void build_items() {
            SubMenuItemBar *settings_bar = new SubMenuItemBar("Settings");

            // 1. Pool membership toggle ("Pooled" = included in auto-voice assign)
            settings_bar->add(new LambdaToggleControl("Pooled",
                [=](bool v) { *pool_flag_ = v; },
                [=]() -> bool { return *pool_flag_; }
            ));

            // 2. Gate output toggle
            settings_bar->add(new LambdaToggleControl("Gate",
                [=](bool v) { output_->set_gate_output_enabled(v); },
                [=]() -> bool  { return output_->get_gate_output_enabled(); }
            ));

            // 3. Park: re-quantise CV voltage to scale/chord when channel is idle
            #ifdef ENABLE_SCALES
            settings_bar->add(new LambdaToggleControl("Park",
                [=](bool v) { output_->set_park_enabled(v); },
                [=]() -> bool  { return output_->get_park_enabled(); }
            ));
            #endif

            // -- Slew - combine with the Settings bar
            // 5. Slew enable toggle
            settings_bar->add(new LambdaToggleControl("Slew",
                [=](bool v) { output_->slew_enabled = v; },
                [=]() -> bool  { return output_->slew_enabled; }
            ));

            // 6. Slew rate
            settings_bar->add(new LambdaNumberControl<float>(
                "Slew Rate",
                [=](float v) { if (slew_base_normal_) *slew_base_normal_ = v; output_->effective_slew_rate_normal = v; },
                [=]() -> float { return slew_base_normal_ ? *slew_base_normal_ : output_->effective_slew_rate_normal; },
                nullptr,
                0.0f, 1.0f,
                0.01f, true
            ));

            this->add(settings_bar);

            // 3. Note display: last note played vs current output note
            this->add(new HarmonyStatus("Note", last_note_, &output_->current_pitch_note));

            // 4. Per-channel note limits bar
            SubMenuItemBar *limits_bar = new SubMenuItemBar("Limits");
            limits_bar->add(new LambdaScaleNoteMenuItem<int8_t>(
                "Lo",
                [=](int8_t v) { *lowest_note_ = v; if (effective_lowest_note_) *effective_lowest_note_ = v; },
                [=]() -> int8_t { return *lowest_note_; },
                nullptr,
                (int8_t)MIDI_MIN_NOTE, (int8_t)MIDI_MAX_NOTE,
                true, true
            ));
            limits_bar->add(new LambdaScaleNoteMenuItem<int8_t>(
                "Hi",
                [=](int8_t v) { *highest_note_ = v; if (effective_highest_note_) *effective_highest_note_ = v; },
                [=]() -> int8_t { return *highest_note_; },
                nullptr,
                (int8_t)MIDI_MIN_NOTE, (int8_t)MIDI_MAX_NOTE,
                true, true
            ));
            limits_bar->add(new NoteLimitModeControl<>(
                "Lo Mode",
                [=](NOTE_LIMIT_MODE v) { *lowest_mode_ = v; },
                [=]() -> NOTE_LIMIT_MODE { return *lowest_mode_; },
                nullptr, true, true
            ));
            limits_bar->add(new NoteLimitModeControl<>(
                "Hi Mode",
                [=](NOTE_LIMIT_MODE v) { *highest_mode_ = v; },
                [=]() -> NOTE_LIMIT_MODE { return *highest_mode_; },
                nullptr, true, true
            ));
            this->add(limits_bar);

            // 7. CV polarity / invert controls (unique per output channel, from addCustomTypeControls)
            this->add(new SeparatorMenuItem("Output settings"));
            LinkedList<MenuItem *> *custom_controls = new LinkedList<MenuItem *>();
            output_->addCustomTypeControls(custom_controls);
            this->add(custom_controls);   // transfers ownership; deletes the list wrapper

            // Modulation sub-page: per-channel parameter selector + shared lowmemory controls.
            // ParameterMenuItemSelector lets the user pick which per-channel parameter to modulate;
            // LowMemorySwitcherMenuItem updates lowmemory_controls.parameter to the selection;
            // the shared amount/slot controls then operate on the selected parameter.
            // auto *mod = new SubMenuItem("Mod");
            this->add(new SeparatorMenuItem("Modulation"));
            auto *param_sel = new ParameterMenuItemSelector("Param", per_channel_params_);
            this->add(param_sel);
            // mod->add(param_sel);
            this->add(new LowMemorySwitcherMenuItem((char *)"", per_channel_params_, param_sel, C_WHITE));
            if (lowmemory_controls.parameter_amount_controls != nullptr)
                this->add(lowmemory_controls.parameter_amount_controls);
            for (uint_fast8_t j = 0; j < MAX_SLOT_CONNECTIONS; j++) {
                if (lowmemory_controls.slot_rows[j] != nullptr)
                    this->add(lowmemory_controls.slot_rows[j]);
            }
            // this->add(mod);
        }
};

#endif // ENABLE_SCREEN
