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
    
    //GFXcanvas16 *canvas;

    GatesDisplay(const char *label) : MenuItem(label) {}

    virtual int display(Coord pos, bool selected, bool opened) override {
        pos.y = header(label, pos, selected, opened);

        //tft->setCursor(pos.x, pos.y);

        for (int i = 0 ; i < gate_manager->num_banks ; i++) {
            int x = 0;
            tft->setCursor(x, pos.y);
            tft->printf("Bank %i: %i gates", i, gate_manager->banks[i]->num_gates);
            tft->println();
            pos.y += tft->getRowHeight(); //= tft->getCursorY();
            for (int g = 0 ; g < gate_manager->banks[i]->num_gates ; g++) {
                uint16_t color = gate_manager->banks[i]->check_gate(g) ? GREEN : RED;
                tft->fillRect(x, pos.y, 8, 8, color);
                x += 8 + 4;
                if (x > tft->width()) {
                    pos.y += tft->getRowHeight();
                    x = 0;
                }                
            }
            pos.y += tft->getRowHeight(); 
        }

        return pos.y;
    }

};

#endif

#endif

