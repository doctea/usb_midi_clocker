#pragma once

#include "Config.h"

#include "menuitems.h"
#include "midi_helpers.h"

#include "menuitems.h"
#include "submenuitem_bar.h"

class ProgressionPinnedMenuItem : /*virtual public SubMenuItemBar,*/ virtual public PinnedPanelMenuItem {
    public:
        SubMenuItemBar *progression_bar = nullptr;
        ProgressionPinnedMenuItem(const char *label) : /*SubMenuItemBar(label),*/ PinnedPanelMenuItem(label) {
            //this->setup_progression_bar();
        };

        //void setup_progression_bar();

        //using SubMenuItemBar::display;
        /*virtual int display(Coord pos, bool selected, bool opened) override {
            return PinnedPanelMenuItem::display(pos, selected, opened);
        }*/
        //using PinnedPanelMenuItem::tft;
        //using SubMenuItemBar::on_add;

        /*virtual int display(Coord pos, bool selected, bool opened) override {
            SubMenuItemBar::tft->println("wtf?");
            return SubMenuItemBar::tft->getCursorY();
        }*/

        virtual int display(Coord pos, bool selected, bool opened) override;
};
