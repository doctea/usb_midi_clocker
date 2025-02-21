#pragma once

#include "Config.h"

#include "menuitems.h"
#include "midi_helpers.h"

#include "menuitems.h"
#include "submenuitem_bar.h"

class ProgressionPinnedMenuItem : public PinnedPanelMenuItem {
    public:
        SubMenuItemBar *progression_bar = nullptr;
        ProgressionPinnedMenuItem(const char *label) : PinnedPanelMenuItem(label) {
            this->progression_bar = new SubMenuItemBar(label, true, true);

            this->setup_progression_bar();

        };

        void setup_progression_bar();

        virtual int display(Coord pos, bool selected, bool opened) override {
            int y = pos.y;
            if (this->progression_bar!=nullptr) {
                y += this->progression_bar->display(Coord(pos.x, y), selected, opened);
            }
            return y;
        }

        virtual void on_add() override {
            if (this->progression_bar!=nullptr) {
                this->progression_bar->on_add();
            }
        }

        /*virtual int display(Coord pos, bool selected, bool opened) override {
            int y = pos.y;
            int x = 0;
            tft->setCursor(0, y);
            tft->setTextSize(1);
            tft->setTextColor(C_WHITE, BLACK);
            // show playlist position
            tft->printf("[%i/%i]", behaviour_progression->playlist_position, 8);
            // show current section
            // show current lock status
            return y;
        }*/
};
