#include "Config.h"

#ifdef ENABLE_SEQUENCER

#include "mymenu/menu_slotcontroller.h"

#include "midi/midi_apcmini.h"
#include "midi/midi_apcmini_display.h"

#include "sequencer.h"
#include "storage.h"
#include "project.h"
extern Menu *menu;

// display sequence+clocks
// todo: rename to something more representative
// todo: tidy this tf up, it is UGLEEEEE

// todo: fix controls...
//       CLOCKS have two parameters per *row* - DELAY and MULTIPLY
//          don't need to be able to select the cell; do need to be able to switch between adjusting delay or multiply
//       SEQUENCES have one parameter per *cell*, with 8 cells per row
//          need to be able to select the cell
//  .. maybe split this into two separate controls, one that displays/edits clocks, one that displays/edits sequences...
//          
class SequencerDisplay : public MenuItem {

    enum openlevel_t {
        NONE = 0,
        ROW = 1,
        COLUMN = 2,
        CELL = 3
    };

    int8_t selected_column = 0, selected_row = 0;
    openlevel_t opened_level = NONE;

    public: 
        SequencerDisplay(const char *label) : MenuItem(label) {
            //this->selectable = false;
        };


        bool is_row_clock(int8_t row) {
            return row < NUM_CLOCKS;
        }
        bool is_row_sequence(int8_t row) {
            return row >= NUM_CLOCKS && row < NUM_CLOCKS + NUM_SEQUENCES;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            int y_after_header = pos.y = header(label, pos, selected, opened);
            const int cell_width = tft->width() / APCMINI_DISPLAY_WIDTH;
            const int cell_height = cell_width / 2;

            if (opened_level==ROW)
                tft->printf("ROW:\tROW %i selected\n", selected_row);
            else if (opened_level==COLUMN)
                tft->printf("COLUMN:\tROW %i, COLUMN %i selected\n", selected_row, selected_column);
            else if (opened_level==CELL) 
                tft->printf("CELL:\tROW %i, COLUMN %i opened\n");
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
                    tft->drawRect(pos.x, y, tft->width(), cell_height-1, C_WHITE);
                } else if (opened_level == COLUMN || opened_level == CELL) {
                    // draw a box around selected cell only
                    tft->drawRect(pos.x + (selected_column*cell_width), y, cell_width, cell_height-1, C_WHITE);
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
                if (is_row_clock(selected_row)) {
                    //decrease_clock_delay(selected_column);
                    decrease_clock_multiplier(selected_column);
                } else
                    sequencer_press(selected_row-NUM_SEQUENCES, selected_column, true);
            } else if (opened_level==ROW) {
                selected_row--;
                if (selected_row < 0)
                    selected_row = APCMINI_DISPLAY_WIDTH - 1;
                else if (selected_row >= APCMINI_DISPLAY_WIDTH)
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
                if (is_row_clock(selected_row)) {
                    //increase_clock_delay(selected_row);
                    increase_clock_multiplier(selected_row);
                } else
                    sequencer_press(selected_row-NUM_SEQUENCES, selected_column, false);
            } else if (opened_level==ROW) {
                selected_row++;
                if (selected_row < 0)
                    selected_row = APCMINI_DISPLAY_WIDTH - 1;
                else if (selected_row >= APCMINI_DISPLAY_WIDTH)
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