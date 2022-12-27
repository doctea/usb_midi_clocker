#ifndef MENU_ARRANGEMENT_TRACK__INCLUDED
#define MENU_ARRANGEMENT_TRACK__INCLUDED

#include "Config.h"

//#include "midi/midi_mpk49.h"
#include "behaviours/behaviour_mpk49.h"
#include "project.h"
#include "mymenu.h"
#include "menu.h"

#include "submenuitem_bar.h"

#include "arrangement/arrangement.h"
#include "clips/clip_manager.h"
#include "clips/clips.h"

class ArrangementHeader : public MenuItem {
    public:
    ArrangementHeader(const char *label) : MenuItem(label) {
        this->selectable = false;
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        pos.y += 3;
        //pos.y = 
        header(label, pos, selected, opened);

        int x = 12 * tft->characterWidth(); //->getCursorX();
        int view_port_width = (tft->width()/tft->characterWidth())-12;
        int view_port_start = max(0, (arrangement->current_song_phrase - (view_port_width/2)));

        for (int phrase = view_port_start ; phrase < view_port_start+view_port_width ; phrase++) {
            tft->setCursor(
                x + ((phrase-view_port_start)*tft->characterWidth()), 
                pos.y
            );
            colours(phrase==arrangement->current_song_phrase); //);
            tft->printf("%i",phrase%8);
        }
        pos.y += tft->getRowHeight();

        return pos.y;
    }

};

class ArrangementTrackEditor : public SubMenuItem {
    public:
        ArrangementTrackBase *track = nullptr;

        int32_t test_number = 0;

        ArrangementTrackEditor(const char *label, ArrangementTrackBase * track) : SubMenuItem(label) {
            this->track = track;
            this->always_show = false;
            //this->allow_takeover = false;
            this->add(new NumberControl<int32_t>("test", &this->test_number, 0, 0, 32, false));
        }

        virtual bool allow_takeover() override {
            return false;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            //Serial.println("ArrangementEditor#display start");
            tft->setCursor(pos.x, pos.y);
            colours(selected, track->colour);
            char label[MAX_TRACK_NAME];
            sprintf(label, "%-10s", track->label);
            label[10] = '\0';
            tft->print(label);
            colours(false, track->colour);
            tft->print((char*)": ");

            int start_x = tft->getCursorX();
            int start_y = tft->getCursorY();

            int bottom_y = start_y;

            int view_port_width = (tft->width()/tft->characterWidth())-12;
            int view_port_start = max(0, (arrangement->current_song_phrase - (view_port_width/2)));

            int lanes_count = track->get_num_lanes();
            for (int phrase = view_port_start ; phrase < view_port_start + view_port_width ; phrase++) {
                for (int lane = 0 ; lane < lanes_count ; lane++) {
                    tft->setCursor(
                        start_x + ((phrase-view_port_start) * tft->characterWidth()), 
                        start_y + lane*tft->getRowHeight()
                    );
                    clip_instance_t *c_i = track->get_clip_at_time_lane(phrase, lane);
                    if (c_i==nullptr) 
                        continue;
                    Clip *clip = clip_manager->get_clip_for_id(c_i->clip_id);
                    tft->printc(clip->get_name());
                    bottom_y = start_y + tft->getRowHeight();
                }
            }
            bottom_y = start_y + (tft->getRowHeight() * lanes_count);

            tft->setCursor(pos.x, bottom_y);
            tft->printf("selected test = %i\n", this->test_number);
            bottom_y = tft->getCursorY();

            if (opened)
                return SubMenuItem::display(Coord(0, bottom_y), selected, opened);
            else
                return bottom_y;

            //return bottom_y;
        }
};


#endif