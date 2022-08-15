#include "Config.h"

#if defined(ENABLE_LOOPER)

//#include "midi_mpk49.h"
#include "behaviour_mpk49.h"
#include "project.h"
#include "mymenu.h"
#include "menu.h"

class LooperDisplay : public MenuItem {
    public:
    MIDITrack *loop_track;

    //GFXcanvas16 *canvas;

    LooperDisplay(const char *label, MIDITrack *loop_track) : MenuItem(label) {
        this->loop_track = loop_track;
    }

    /*virtual void on_add() override {
        //canvas = new GFXcanvas16(this->tft->width(), this->tft->getRowHeight()*2);
    }*/
    virtual int display(Coord pos, bool selected, bool opened) override {
        //Serial.println("LooperDisplay#display!");
        //pos.y += MenuItem::display(pos, selected, opened);
        pos.y = header(label, pos, selected, opened);
        /*this->canvas->setCursor(20,10);
        this->canvas->setTextColor(C_WHITE);
        this->canvas->setTextSize(2);
        this->canvas->drawRect(5,5,10,20, RED);
        this->canvas->println("drawn to canvas");*/
        //Serial.println("menu_looperdisplay display()");

        tft->setCursor(pos.x, pos.y);
        //tft->printf("display %i?", ticks);

        /*if (is_bpm_on_beat(ticks)) {
            //Serial.println("rendering piano roll bitmap");
            loop_track->draw_piano_roll_bitmap();
        }*/
        //DisplayTranslator_STeensy *a = (DisplayTranslator_STeensy*) tft;
        //a->drawBitmap(pos.x, pos.y, this->canvas);

        //int ticks_per_pixel = round(tft->width() / LOOP_LENGTH);
        /*int ticks_per_pixel = round((float)LOOP_LENGTH / (float)tft->width());
        Serial.printf("ticks_per_pixel = %i / %i = %i\n", tft->width(), LOOP_LENGTH, ticks_per_pixel);

        for (int i = 0 ; i < tft->width() ; i++) {
            //Serial.printf("for real pixel %i: \n", i);
            // for each pixel, process ticks_per_pixel ticks
            for (int pitch = 127 ; pitch > 0 ; pitch--) {
                bool held = false;
                //Serial.printf("\tfor pitch %i:\n", pitch);
                for (int t = 0 ; t < ticks_per_pixel && i+t < LOOP_LENGTH ; t++) {
                    //Serial.printf("\t\tfor pixel column %i and row %i, scanning tick %i..", i, pitch, t+i);
                    if (loop_track->piano_roll_bitmap[i+t][pitch]) {
                        //Serial.println("HELD!");
                        held = true;
                        tft->drawLine(i, pos.y+(127-pitch)+1, i+1, pos.y+(127-pitch)+1, YELLOW + pitch*16); //(pitch<<8) || (0b000000001111111 && C_WHITE));
                        break;
                    } else {
                        //Serial.println();
                    }
                }
            }
        }*/

        static float ticks_per_pixel = (float)LOOP_LENGTH / (float)tft->width();
        int row = pos.y;

        // TODO: centering/resizing of piano roll based on what notes are in it
        for (int i = 0 ; i < tft->width() ; i++) {
            //Serial.printf("for real pixel %i: \n", i);
            // for each pixel, process ticks_per_pixel ticks
            for (int pitch = 0 ; pitch < 127 ; pitch++) {

                bool held = false;
                //Serial.printf("\tfor pitch %i:\n", pitch);
                int tick_for_pixel = (float)i * ticks_per_pixel;
                int colour = YELLOW + pitch*16;
                if (ticks%LOOP_LENGTH==tick_for_pixel || ((int)(ticks+ticks_per_pixel))%LOOP_LENGTH==tick_for_pixel)
                    colour = YELLOW;
                if (loop_track->piano_roll_bitmap[tick_for_pixel][pitch]) {
                    row = pos.y + (127-pitch);

                    tft->drawLine(
                        i, 
                        row, //tft->getRowHeight() + pos.y+(first_found-pitch)+1, 
                        i+1, 
                        row, //tft->getRowHeight() + pos.y+(first_found-pitch)+1, 
                        colour
                    );
                }

                if (this->loop_track->track_playing[pitch].velocity>0) {
                    tft->drawLine(0, pos.y+(127-pitch), 2, pos.y+(127-pitch), C_WHITE + (this->loop_track->track_playing[pitch].velocity<<8));
                } else {
                    //tft->drawLine(0, pos.y+(127-pitch), 2, pos.y+(127-pitch), BLACK); // + (this->loop_track->track_playing[pitch].velocity<<8));
                }

            }
        }
        //Serial.printf("first_found = %i, last_found = %i, height = %i\n", first_found, last_found, first_found - last_found);

        return pos.y + 127; //row + 12; //pos.y + (first_found - last_found); //127;  
    }

};


#endif

