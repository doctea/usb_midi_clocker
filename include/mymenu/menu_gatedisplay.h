#ifndef GATEDISPLAY__INCLUDED
#define GATEDISPLAY__INCLUDED
#include "Config.h"

#if defined(ENABLE_SEQUENCER)

//#include "midi/midi_mpk49.h"
#include "interfaces/interfaces.h"
#include "mymenu.h"
#include "menu.h"

extern GateManager *gate_manager;

class GatesDisplay : public MenuItem {
    public:
    
    GatesDisplay(const char *label) : MenuItem(label) {}

    static const int GATE_SIZE = 8;
    static const int GATE_PITCH = GATE_SIZE + 4;
    static const int COLUMN_GAP = 4;
    static const int COLUMNS = 2;

    virtual int display(Coord pos, bool selected, bool opened) override {
        pos.y = header(label, pos, selected, opened);

        const int row_h = tft->getRowHeight();
        const int char_w = tft->currentCharacterWidth();

        const int column_width = (tft->width() / COLUMNS) - COLUMN_GAP;
        const int label_chars = char_w > 0 ? max(0, (column_width / char_w) - 2) : 0;

        int row_top = pos.y;
        int row_height = 0;

        for (int i = 0 ; i < gate_manager->num_banks ; i++) {
            const int column = i % COLUMNS;
            const int column_x = column * (tft->width() / COLUMNS);

            int y = row_top;
            tft->setCursor(column_x, y);
            tft->printf("%i:%.*s", i, label_chars, gate_manager->banks[i]->get_label());
            y += row_h;

            int x = column_x;
            for (int g = 0 ; g < gate_manager->banks[i]->num_gates ; g++) {
                uint16_t color = gate_manager->banks[i]->check_gate(g) ? GREEN : RED;
                tft->fillRect(x, y, GATE_SIZE, GATE_SIZE, color);
                x += GATE_PITCH;
                if (x + GATE_SIZE > column_x + column_width) {
                    y += row_h;
                    x = column_x;
                }
            }
            y += row_h;

            if (y - row_top > row_height)
                row_height = y - row_top;

            if (column == COLUMNS - 1) {
                row_top += row_height;
                row_height = 0;
            }
        }

        return row_top + row_height;
    }

};

#endif

#endif

