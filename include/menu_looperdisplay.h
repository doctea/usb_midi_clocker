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
        int base_row = pos.y;

        // we're going to use direct access to the underlying Adafruit library here
        DisplayTranslator_STeensy *tft2 = (DisplayTranslator_STeensy*)tft;
        ST7789_t3 *actual = tft2->tft;

        if (this->loop_track->isPlaying() || this->loop_track->isOverwriting() || this->loop_track->isRecording()) {
            uint16_t indicatorcolour = 0xAAAA;
            if (this->loop_track->isOverwriting() && this->loop_track->isRecording()) 
                indicatorcolour = (RED + ORANGE)/2;
            else if (this->loop_track->isRecording())
                indicatorcolour = RED;
            else if (this->loop_track->isOverwriting())
                indicatorcolour = ORANGE;
            //actual->drawFastVLine(screen_x, base_row, 127, indicatorcolour);
            actual->drawLine(ticks%LOOP_LENGTH / ticks_per_pixel, base_row, ticks%LOOP_LENGTH / ticks_per_pixel, base_row+127, indicatorcolour);
        }

        // TODO: centering/resizing of piano roll based on what notes are in it
        for (int pitch = 0 ; pitch < 127 ; pitch++) {
            //Serial.printf("for pitch %i\n", pitch);
            for (int screen_x = 0 ; screen_x < tft->width() ; screen_x++) {
                //Serial.printf("for real pixel %i: \n", i);
                // for each pixel, process ticks_per_pixel ticks
                //bool held = false;
                //Serial.printf("\tfor pitch %i:\n", pitch);
                uint16_t tick_for_screen_X = (float)screen_x * ticks_per_pixel; // the tick corresponding to this screen position
                uint16_t colour = YELLOW + pitch*16;

                uint16_t draw_at_y = base_row + (127-pitch);

                /*if (ticks%LOOP_LENGTH==tick_for_pixel || ((int)(ticks+ticks_per_pixel))%LOOP_LENGTH==tick_for_pixel)
                    colour = YELLOW;*/
                // draw a vertical line showing the cursor position, if we're playing, recording or overwriting

                if (loop_track->piano_roll_bitmap[tick_for_screen_X][pitch] > 0) {
                    int current_velocity = loop_track->piano_roll_bitmap[tick_for_screen_X][pitch];
                    //Serial.printf("\tat %i with velocity %i\n", pitch, tick_for_screen_X, current_velocity);
                    /*for (int temp_tick = tick_for_screen_X ; temp_tick < LOOP_LENGTH ; temp_tick++) {
                        if (loop_track->piano_roll_bitmap[temp_tick][pitch] != current_velocity) {
                            uint16_t length_in_ticks = temp_tick - tick_for_screen_X;
                            uint16_t length = (float)(length_in_ticks) / ticks_per_pixel;
                            //Serial.printf("\t\tfound run %i long to draw at %i,%i\n", length, screen_x,draw_at_y);

                            // highlight the currently-playing note
                            if (this->loop_track->track_playing[pitch].velocity>0 && ticks%LOOP_LENGTH>=tick_for_screen_X && ticks%LOOP_LENGTH<=temp_tick) {
                                //Serial.printf("\t\t\tis caught playing at tick %i!", ticks);
                                colour = YELLOW;
                            }

                            //actual->drawFastHLine(screen_x, draw_at_y, length+1, colour);
                            actual->drawLine(screen_x, draw_at_y, screen_x+length, draw_at_y, colour);
                            screen_x = temp_tick;
                            break;
                        }
                    }*/
                    tft->drawLine(
                        screen_x, 
                        draw_at_y, //tft->getRowHeight() + pos.y+(first_found-pitch)+1, 
                        screen_x+1, 
                        draw_at_y, //tft->getRowHeight() + pos.y+(first_found-pitch)+1, 
                        colour
                    );
                }

                if (this->loop_track->track_playing[pitch].velocity>0) {
                    // draw a little indicator displaying what notes are played with brightness indicating velocity
                    //int draw_at_y = pos.y+(127-pitch);
                    //tft->drawLine(0, draw_at_y, 2, draw_at_y, C_WHITE + (this->loop_track->track_playing[pitch].velocity<<8));
                    actual->drawFastHLine(0, draw_at_y, 2, C_WHITE + (this->loop_track->track_playing[pitch].velocity<<8));
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

