#include "Config.h"

//#include "midi/midi_mpk49.h"
#include "behaviours/behaviour_mpk49.h"
#include "project.h"
#include "mymenu.h"
#include "menu.h"

#include "arrangement/arrangement.h"
#include "clips/clip_manager.h"
#include "clips/clips.h"

class ArrangementEditor : public MenuItem {
    public:
        //Arrangement *arrangement = nullptr;
        /*ArrangementEditor(char *label, Arrangement *arrangment) : MenuItem(label){
            this->arrangement = arrangement;
        }*/
        ArrangementEditor(char *label) : MenuItem(label){
        }

        int view_port_position_start = 0;

        virtual int display(Coord pos, bool selected, bool opened) override {
            //Serial.println("ArrangementEditor#display start");
            pos.y = header(label, pos, selected, opened);
            int x = 12 * tft->characterWidth(); //->getCursorX();
            int view_port_width = (tft->width()/tft->characterWidth())-12;
            for (int i = 0 ; i < view_port_width ; i++) {
                tft->setCursor(x + (i*tft->characterWidth()),pos.y);
                colours(i==BPM_CURRENT_PHRASE);
                tft->printf("%i",i%8);
            }
            tft->println();
            pos.y = tft->getCursorY();

            int bottom_y = pos.y;

            LinkedList<ArrangementTrackBase *> *tracks = arrangement->get_tracks();
            for (int i = 0 ; i < tracks->size() ; i++) {
                ArrangementTrackBase *track = tracks->get(i);
                tft->printf("%-10s: ", track->label);
                int start_x = tft->getCursorX();
                int start_y = tft->getCursorY();
                for (int phrase = view_port_position_start ; phrase < view_port_position_start + view_port_width ; phrase++) {
                    LinkedList<clip_instance_t*> *clip_instances = track->get_clips_at_time(phrase);
                    for (int c = 0 ; c < clip_instances->size() ; c++) {
                        tft->setCursor(
                            start_x + ((phrase-view_port_position_start) * tft->characterWidth()), 
                            start_y + c*tft->getRowHeight()
                        );
                        //tft->printf("%1x\n", clip_instances->get(c)->clip_id);
                        if (clip_instances->get(c)->clip!=nullptr) {
                            //tft->printc((char)clip_instances->get(c)->clip->get_name());
                            tft->printf("%1x\n", clip_instances->get(c)->clip_id);
                            //tft->println();
                        } else {
                            bottom_y = start_y + tft->getRowHeight();
                        }
                    }
                    //tft->setCursor(start_x + tft->characterWidth(), bottom_y);
                    if (tft->getCursorY() > bottom_y) 
                        bottom_y = tft->getCursorY();

                    tft->setCursor(0, bottom_y);
                }
            }
          
            /*for (int i = 0 ; i < clip_manager->clips->size() ; i++) {
                Clip *c = clip_manager->clips->get(i);
                if (c!=nullptr)
                    tft->printf("%i:\n", c->clip_id);
            }
            int bottom_y = tft->getCursorY();

            //pos.y += tft->getRowHeight();

            Serial.println("ArrangementEditor#display about ot attempt to draw clips");
            for (int phrase = view_port_position_start ; phrase < view_port_position_start + view_port_width ; phrase++) {
                int x2 = x + (phrase*tft->characterWidth());
                Serial.printf("ArrangementEditor#display about to attempt to get clips at position %i\n", phrase);

                LinkedList<clip_instance_t*> *clips_at_time = arrangement->get_clips_at_time(phrase);
                Serial.printf("ArrangementEditor#display got %i clips at position %i\n", clips_at_time->size(), phrase);
                for (int z = 0 ; z < clips_at_time->size() ; z++) {
                    clip_instance_t *c = clips_at_time->get(z);
                    if (c==nullptr) break;
                    int y2 = pos.y + (c->clip_id*(tft->getRowHeight()-4));
                    tft->setCursor(x2, y2);
                    colours(phrase==BPM_CURRENT_PHRASE);
                    tft->print("X");
                }
            }*/

            return bottom_y;
        }
};