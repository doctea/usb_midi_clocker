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
// todo: add controls to allow editing
class SequencerDisplay : public MenuItem {
    //int ui_selected_sequence_number = 0;
    public: 
        SequencerDisplay(const char *label) : MenuItem(label) {
            this->selectable = false;
        };

        int display(Coord pos, bool selected, bool opened) {
            pos.y = header(label, pos, selected, opened);
            const int cell_width = tft->width() / APCMINI_DISPLAY_WIDTH;
            const int cell_height = cell_width / 2;

            for (int y = 0 ; y < NUM_CLOCKS ; y++) {
                for (int x = 0 ; x < APCMINI_DISPLAY_WIDTH ; x++) {                
                    //int16_t colour = get_sequencer_cell_apc_colour(y, x) ? GREEN : BLACK;
                    const int16_t colour = get_sequencer_cell_565_colour(y, x);
                    const int16_t cache_colour = get_565_colour_for_apc_note(apc_note_last_sent[(64-((y+1)*APCMINI_DISPLAY_WIDTH)) + x]);
                    tft->fillRect(pos.x + (x * cell_width), pos.y, cell_width-2, cell_height-2, colour);
                    tft->fillRect(pos.x + (x * cell_width), pos.y, cell_width/4, cell_height/4, cache_colour);  // draw the currently cached colour as a little inset rectangle so that we can see what's cached etc
                }
                pos.y += cell_height;
            }

            for (int y = 0 ; y < NUM_SEQUENCES ; y++) {
                for (int x = 0 ; x < APCMINI_DISPLAY_WIDTH ; x++) {
                    //int16_t colour = get_sequencer_cell_apc_colour(y+4, x) ? RED : BLACK;
                    const int16_t colour = get_sequencer_cell_565_colour(y+NUM_CLOCKS, x);
                    const int16_t cache_colour = get_565_colour_for_apc_note(apc_note_last_sent[(32-((y+1)*APCMINI_DISPLAY_WIDTH)) + x]);
                    tft->fillRect(pos.x + (x * cell_width), pos.y, cell_width-2, cell_height-2, colour);
                    tft->fillRect(pos.x + (x * cell_width), pos.y, cell_width/4, cell_height/4, cache_colour); // draw the currently cached colour as a little inset rectangle so that we can see what's cached etc
                }
                pos.y += cell_height;
            }

            pos.y += 3;

            tft->setCursor(pos.x, pos.y);

            return pos.y;
        }

};

#endif