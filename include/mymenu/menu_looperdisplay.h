#include "Config.h"

#if defined(ENABLE_LOOPER)

//#include "midi/midi_mpk49.h"
#include "behaviours/behaviour_mpk49.h"
#include "project.h"
#include "mymenu.h"
#include "menu.h"

#include "midi/midi_looper.h"

class LooperDisplay : public MenuItem {
    public:
    MIDITrack *loop_track;

    //GFXcanvas16 *canvas;

    LooperDisplay(const char *label, MIDITrack *loop_track) : MenuItem(label), loop_track(loop_track) {}

    /*float ticks_per_pixel = 1.0;
    virtual void on_add() override {
        //canvas = new GFXcanvas16(this->tft->width(), this->tft->getRowHeight()*2);
        ticks_per_pixel = (float)LOOP_LENGTH_TICKS / (float)tft->width();
    }*/


    /*
    /// this was supposed to be faster... actually slower than recalculating in loop!
    uint16_t precalculated_value_screen_x_for_tick[LOOP_LENGTH_TICKS];
    bool precalculated = false;
    virtual int get_screen_x_for_tick(int tick) {
        if (!precalculated) {
            static int ticks_per_pixel = (float)LOOP_LENGTH_TICKS / (float)tft->width();
            for (unsigned int i = 0 ; i < LOOP_LENGTH_TICKS ; i++) {
                precalculated_value_screen_x_for_tick[i] = i / ticks_per_pixel;
            }
            precalculated = true;
        }
        return precalculated_value_screen_x_for_tick[tick%LOOP_LENGTH_TICKS];
    }

    /// this was supposed to be faster... actually slower than recalculating in loop!
    uint16_t precalculated_value_tick_for_screen_x[240];
    bool precalculated_tick_for_screen_x = false;
    virtual int get_tick_for_screen_x(int screen_x) {
        if (!precalculated_tick_for_screen_x) {
            static int ticks_per_pixel = (float)LOOP_LENGTH_TICKS / (float)tft->width();
            for (unsigned int i = 0 ; i < tft->width() ; i++) {
                precalculated_value_tick_for_screen_x[i] = ticks_to_sequence_step((int)((float)screen_x * ticks_per_pixel));
            }
            precalculated_tick_for_screen_x = true;
        }
        return precalculated_value_tick_for_screen_x[screen_x];
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

        for (unsigned int i = 0 ; i < tft->width() ; i++) {
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

        //static float ticks_per_pixel = (float)LOOP_LENGTH_TICKS / (float)tft->width();
        const uint16_t base_row = pos.y;
        static float ticks_per_pixel = (float)LOOP_LENGTH_TICKS / (float)tft->width();

        // work out what range of pitches we need to draw on-screen
        int first_pitch = loop_track->first_pitch();
        int last_pitch = loop_track->last_pitch();
        uint8_t range = last_pitch - first_pitch;

        // draw a vertical line showing the cursor position, if we're playing, recording or overwriting
        if (this->loop_track->isPlaying() || this->loop_track->isOverwriting() || this->loop_track->isRecording()) {
            uint16_t indicatorcolour = 0xAAAA;
            if (this->loop_track->isOverwriting() && this->loop_track->isRecording()) 
                indicatorcolour = (RED + ORANGE)/2;
            else if (this->loop_track->isRecording())
                indicatorcolour = RED;
            else if (this->loop_track->isOverwriting())
                indicatorcolour = ORANGE;
            else if (this->loop_track->isPlaying()) 
                indicatorcolour = GREEN;
            const int current_screen_x = ticks%LOOP_LENGTH_TICKS / ticks_per_pixel;
            tft->drawFastVLine(current_screen_x, base_row, range, indicatorcolour);
        }

        // only draw over a range between the lowest and highest notes
        for (int pitch = first_pitch ; pitch <= last_pitch ; pitch++) {
            if (!loop_track->does_pitch_contain_notes(pitch)) 
                continue;
            //Serial.printf("for pitch %i\n", pitch);
            
            const int draw_at_y = base_row + (range - (pitch - first_pitch)); //last_pitch-first_pitch);
            //Serial.printf("for pitch %i with first_pitch %i and last_pitch %i, base_row = %i, drawing at: %i\n", pitch, first_pitch, last_pitch, base_row, draw_at_y);
            for (int screen_x = 0 ; screen_x < tft->width() ; screen_x++) {
                const uint16_t tick_for_screen_X = ticks_to_sequence_step((int)((float)screen_x * ticks_per_pixel)); // the tick corresponding to this screen position
                //const uint16_t tick_for_screen_X = this->get_tick_for_screen_x(screen_x);// ticks_to_sequence_step((int)((float)screen_x * ticks_per_pixel)); // the tick corresponding to this screen position
                uint16_t colour = C_WHITE - ((pitch%12) * 32);

                const int current_velocity = (*loop_track->piano_roll_bitmap)[tick_for_screen_X][pitch];
                if (current_velocity > 0) {                  
                    //Serial.printf("\tat %i with velocity %i\n", pitch, tick_for_screen_X, current_velocity);

                    /// hmmm, this (which should be optimised) actually seems to fuck up the underlying data somehow?
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

                    if (ticks%LOOP_LENGTH_TICKS==tick_for_screen_X || ((int)(ticks+ticks_per_pixel))%LOOP_LENGTH_TICKS==tick_for_screen_X)
                        colour = YELLOW;

                    tft->drawFastHLine(screen_x, draw_at_y, 2, colour);
                }
            }
            if (this->loop_track->track_playing[pitch].velocity>0) {
                // draw a little indicator displaying what notes are currently held, with brightness indicating velocity
                tft->drawFastHLine(0, draw_at_y, 2, C_WHITE + (this->loop_track->track_playing[pitch].velocity<<8));
            } else {
                //tft->drawLine(0, pos.y+(127-pitch), 2, pos.y+(127-pitch), BLACK); // + (this->loop_track->track_playing[pitch].velocity<<8));
            }
        }
        //Serial.printf("first_found = %i, last_found = %i, height = %i\n", first_found, last_found, first_found - last_found);

        return pos.y + 4 + (last_pitch-first_pitch); //row + 12; //pos.y + (first_found - last_found); //127;  
    }

};


#endif

