#include "Config.h"

#ifdef ENABLE_SEQUENCER

#include "mymenu/menu_slotcontroller.h"

#include "midi/midi_apcmini.h"
#include "midi/midi_apcmini_display.h"

#include "sequencer.h"
#include "storage.h"
#include "project.h"
extern Menu *menu;

// display&edit apcmini sequence+clocks
// todo: tidy this up a bit
class ClockSequencerDisplay : public MenuItem {

    enum openlevel_t {
        NONE = 0,
        ROW = 1,
        MULTIPLIER = 2,
        DELAY = 3
    };

    int8_t selected_column = 0, selected_row = 0;
    openlevel_t opened_level = NONE;

    public: 
        ClockSequencerDisplay(const char *label) : MenuItem(label) {
            //this->selectable = false;
        };

        virtual int display(Coord pos, bool selected, bool opened) override {
            int y_after_header = pos.y = header(label, pos, selected, opened);
            const int cell_width = tft->width() / APCMINI_DISPLAY_WIDTH;
            const int cell_height = cell_width / 2;

            if (opened_level==ROW)
                tft->printf("Select clock row...\n", selected_row);
            else if (opened_level==MULTIPLIER)
                tft->printf("Editing multiplier...\n");
            else if (opened_level==DELAY) 
                tft->printf("Editing delay...\n");
            y_after_header = pos.y = tft->getCursorY();

            for (int y = 0 ; y < NUM_CLOCKS ; y++) {
                for (int x = 0 ; x < APCMINI_DISPLAY_WIDTH ; x++) {                
                    //int16_t colour = get_sequencer_cell_apc_colour(y, x) ? GREEN : BLACK;
                    const int16_t colour = get_sequencer_cell_565_colour(y, x);
                    const int16_t cache_colour = get_565_colour_for_apc_note(apc_note_last_sent[(64-((y+1)*APCMINI_DISPLAY_WIDTH)) + x]);
                    if (colour==BLACK) {
                        tft->drawRect(pos.x + (x * cell_width), pos.y, cell_width-2, cell_height-2, GREY);
                    } else {
                        tft->fillRect(pos.x + (x * cell_width), pos.y, cell_width-2, cell_height-2, colour);
                        tft->fillRect(pos.x + (x * cell_width), pos.y, cell_width/4, cell_height/4, cache_colour);  // draw the currently cached colour as a little inset rectangle so that we can see what's cached etc
                    }
                }
                pos.y += cell_height;
            }

            // draw selection box
            if (opened) {
                int y = y_after_header + (selected_row * cell_height);
                //if (opened_level == ROW) {
                    // draw a box around the entire row to show what is selected
                    tft->drawRect(pos.x, y-1, tft->width()-1, cell_height, C_WHITE);
                //} else if (opened_level == MULTIPLIER || opened_level == CELL) {
                //    // draw a box around selected cell only
                //    tft->drawRect(pos.x + (selected_column*cell_width), y, cell_width, cell_height-1, C_WHITE);
                //}
            }

            pos.y += 3;
            tft->setCursor(pos.x, pos.y);

            return pos.y;
        }

        virtual bool button_back() override {
            if (opened_level==ROW)
                opened_level = NONE;
            else if (opened_level==MULTIPLIER)
                opened_level = ROW;
            else if (opened_level==DELAY)
                opened_level = MULTIPLIER;
            return opened_level;                
        }

        virtual bool action_opened() override {
            opened_level = ROW;
            return true;
        }

        virtual bool button_select() override {
            if (opened_level==NONE)
                opened_level = ROW;
            else if (opened_level==ROW)
                opened_level = MULTIPLIER;
            else if (opened_level==MULTIPLIER)
                opened_level = DELAY;
            else if (opened_level==DELAY)
                opened_level = MULTIPLIER;
            return false;
        }

        /*bool button_right() override { }*/
        virtual bool knob_left() override {
            if (opened_level==MULTIPLIER) {
                 decrease_clock_multiplier(selected_row);
            } else if (opened_level==DELAY) {
                 decrease_clock_delay(selected_row);
            } else if (opened_level==ROW) {
                selected_row--;
                if (selected_row < 0)
                    selected_row = NUM_CLOCKS - 1;
                else if (selected_row >= NUM_CLOCKS)
                    selected_row = 0;
            } 
            return true;
        }
        virtual bool knob_right() override {
            if (opened_level==MULTIPLIER) {
                increase_clock_multiplier(selected_row);
            } else if (opened_level==DELAY) {
                increase_clock_delay(selected_row);
            } else if (opened_level==ROW) {
                selected_row++;
                if (selected_row < 0)
                    selected_row = NUM_CLOCKS - 1;
                else if (selected_row >= NUM_CLOCKS)
                    selected_row = 0;
            } 
            return true;
        }

};


class TriggerSequencerDisplay : public MenuItem {

    enum openlevel_t {
        NONE = 0,
        ROW = 1,
        COLUMN = 2,
        CELL = 3
    };

    int8_t selected_column = 0, selected_row = 0;
    openlevel_t opened_level = NONE;

    public: 
        TriggerSequencerDisplay(const char *label) : MenuItem(label) {
            //this->selectable = false;
        };

        virtual int display(Coord pos, bool selected, bool opened) override {
            int y_after_header = pos.y = header(label, pos, selected, opened);
            const int cell_width = tft->width() / APCMINI_DISPLAY_WIDTH;
            const int cell_height = cell_width / 2;

            if (opened_level==ROW)
                tft->printf("Select trigger sequencer row...\n");
            else if (opened_level==COLUMN)
                tft->printf("Select cell from row %i\n", selected_row);
            else if (opened_level==CELL) 
                tft->printf("Edit trigger sequencer %i, %i\n", selected_row, selected_column);
            y_after_header = pos.y = tft->getCursorY();

            for (int y = 0 ; y < NUM_SEQUENCES ; y++) {
                for (int x = 0 ; x < APCMINI_DISPLAY_WIDTH ; x++) {
                    //int16_t colour = get_sequencer_cell_apc_colour(y+4, x) ? RED : BLACK;
                    const int16_t colour = get_sequencer_cell_565_colour(y+NUM_CLOCKS, x);
                    const int16_t cache_colour = get_565_colour_for_apc_note(apc_note_last_sent[(32-((y+1)*APCMINI_DISPLAY_WIDTH)) + x]);
                    if (colour==BLACK) {
                        tft->drawRect(pos.x + (x * cell_width), pos.y, cell_width-2, cell_height-2, GREY);
                    } else {
                        tft->fillRect(pos.x + (x * cell_width), pos.y, cell_width-2, cell_height-2, colour);
                        tft->fillRect(pos.x + (x * cell_width), pos.y, cell_width/4, cell_height/4, cache_colour); // draw the currently cached colour as a little inset rectangle so that we can see what's cached etc
                    }
                }
                pos.y += cell_height;
            }

            // draw selection box
            if (opened) {
                int y = y_after_header + (selected_row * cell_height);
                if (opened_level == ROW) {
                    // draw a box around the entire row to show what is selected
                    tft->drawRect(pos.x, y-1, tft->width()-1, cell_height, C_WHITE);
                } else if (opened_level == COLUMN || opened_level == CELL) {
                    // draw a box around selected cell only
                    tft->drawRect(constrain((pos.x + (selected_column*cell_width)) - 1, 0, tft->width()), y-1, cell_width, cell_height, C_WHITE);
                }
            }

            pos.y += 3;
            tft->setCursor(pos.x, pos.y);

            return pos.y;
        }

        virtual bool button_back() override {
            if (opened_level==ROW)
                opened_level = NONE;
            else if (opened_level==COLUMN)
                opened_level = ROW;
            else if (opened_level==CELL)
                opened_level = COLUMN;
            return opened_level;                
        }

        virtual bool action_opened() override {
            opened_level = ROW;
            return true;
        }

        virtual bool button_select() override {
            if (opened_level==NONE)
                opened_level = ROW;
            else if (opened_level==ROW)
                opened_level = COLUMN;
            else if (opened_level==COLUMN)
                opened_level = CELL;
            else if (opened_level==CELL)
                opened_level = COLUMN;
            return false;
        }

        /*bool button_right() override { }*/
        virtual bool knob_left() override {
            if (opened_level==CELL) {
                sequencer_press(selected_row, selected_column, true);
            } else if (opened_level==ROW) {
                selected_row--;
                if (selected_row < 0)
                    selected_row = NUM_SEQUENCES - 1;
                else if (selected_row >= NUM_SEQUENCES)
                    selected_row = 0;
            } else if (opened_level==COLUMN) {
                selected_column--;
                if (selected_column < 0)
                    selected_column = APCMINI_DISPLAY_WIDTH - 1;
                else if (selected_column >= APCMINI_DISPLAY_WIDTH)
                    selected_column = 0;    
            } 
            return true;
        }
        virtual bool knob_right() override {
            if (opened_level==CELL) {
                sequencer_press(selected_row, selected_column, false);
            } else if (opened_level==ROW) {
                selected_row++;
                if (selected_row < 0)
                    selected_row = NUM_SEQUENCES - 1;
                else if (selected_row >= NUM_SEQUENCES)
                    selected_row = 0;
            } else if (opened_level==COLUMN) {
                selected_column++;
                if (selected_column < 0)
                    selected_column = APCMINI_DISPLAY_WIDTH - 1;
                else if (selected_column >= APCMINI_DISPLAY_WIDTH)
                    selected_column = 0;    
            } 
            return true;
        }

};


#endif