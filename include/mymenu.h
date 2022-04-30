#ifndef MENU__INCLUDED
#define MENU__INCLUDED

#include "Config.h"

#include <LinkedList.h>
#include <Adafruit_GFX.h>
#include "ST7789_t3.h"
#include "Vector.h"

//#include "midi_helpers.h"

//#include "tft.h"
#include "bpm.h"

void setup_menu();


// basic line
class MenuItem {
    public:
        char label[20];

        MenuItem(const char *in_label) {
            strcpy(label, in_label);
        }
        virtual int display(ST7789_t3 *tft, int x, int y, bool selected) {
            //Serial.printf("base display for %s\n", label);
            // display this item however that may be
            tft->setCursor(x,y);
            //tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
            colours(tft,selected);
            tft->print(label);
            return (tft->getTextSizeY() * 8) +2;
        }

        void colours(ST7789_t3 *tft, bool selected, uint32_t fg = ST77XX_WHITE, uint32_t bg = ST77XX_BLACK) {
            if (!selected) {
                tft->setTextColor(fg, bg);
            } else {
                tft->setTextColor(bg, fg) ;//ST77XX_BLACK, ST77XX_WHITE);
            }
        }

        int header(ST7789_t3 *tft, const char *text, int x, int y, bool selected = false) {
            tft->setTextColor(0xFFFFFF,0);
            tft->setTextSize(0);
            tft->println(text);
            return 8;
        }
};

// MPK49 loop indicator
#if defined(ENABLE_SCREEN) && defined(ENABLE_RECORDING)
#include "midi_mpk49.h"

extern bool mpk49_recording;
extern bool mpk49_playing;

class LooperStatus : public MenuItem {   
    public:
        LooperStatus() : MenuItem("mpk49_looper") {
            //MenuItem(in_label);
        }

        int display(ST7789_t3 *tft, int x, int y, bool selected) {
            tft->setCursor(x,y);
            y = 0;
            y+= header(tft, "mpk49:", x, y);

            tft->setTextSize(2);
            if (mpk49_recording) {
                //tft->setTextColor(rgb(0xFF,0,0),0);
                colours(tft, selected, ST77XX_RED);
                tft->print("[Rec]");
            } else {
                colours(tft, selected, ST77XX_WHITE);
                tft->print("     ");
            }
            if (mpk49_playing) {
                //tft->setTextColor(rgb(0x00,0xFF,0x00),0);
                colours(tft, selected, ST77XX_GREEN);
                tft->print("[>>]");
            } else {
                //tft->setTextColor(rgb(0x00,0x00,0xFF),0);
                colours(tft, selected, ST77XX_BLUE);
                tft->print("[##]");
            }
            //tft->print("\n");
            y += tft->getTextSizeY() * 8;

            return y;
        }
};
#endif


// BEATSTEP NOTES 
#include "midi_beatstep.h"
String get_note_name(int pitch);
class HarmonyStatus : public MenuItem {
    public:
        HarmonyStatus() : MenuItem("beatstep_harmony") {
            //MenuItem(in_label);
        }
        virtual int display(ST7789_t3 *tft, int x, int y, bool selected) override {
            tft->setCursor(x,y);
            //tft.setTextColor(rgb(0xFFFFFF),0);
            tft->setTextSize(2);
            colours(tft, selected);

            tft->printf("%4s : %4s", 
                get_note_name(last_beatstep_note).c_str(), 
                get_note_name(current_beatstep_note).c_str()
            );

            return tft->getTextSizeY() * 8;
        }
};


// BPM indicator
class PositionIndicator : public MenuItem {
    public:
        PositionIndicator() : MenuItem("position") {
            //MenuItem(in_label);
        }
        virtual int display(ST7789_t3 *tft, int x, int y, bool selected) override {
            //Serial.printf("positionindicator display for %s\n", label);
            tft->setCursor(x,y);
            tft->setTextSize(2);
            if (playing) {
                colours(tft, selected, ST77XX_GREEN, ST77XX_BLACK);
            } else {
                colours(tft, selected, ST77XX_RED, ST77XX_BLACK);
            }
            tft->printf("%04i:%02i:%02i @ %03.2f\n", 
                (ticks / (PPQN*4*4)) + 1, 
                (ticks % (PPQN*4*4) / (PPQN*4)) + 1,
                (ticks % (PPQN*4) / PPQN) + 1,
                bpm_current
            );

            return tft->getTextSizeY() * (2*8);
        }
};


class Menu {
    int currently_selected = -1;
    LinkedList<MenuItem*> items = LinkedList<MenuItem*>();

    public:
        
        void add(MenuItem *m) {
            items.add(m);
        }

        virtual int display(ST7789_t3 *tft) {
            
            int y = 0;
            for (int i = 0 ; i < items.size() ; i++) {
                MenuItem *item = items.get(i);
                y += item->display(tft, 0, y, i==currently_selected) + 1;
            }

            return y;
        }

        void knob_left() {
            currently_selected--;
        }
        void knob_right() {
            currently_selected++;
            Serial.printf("selected %i\n", currently_selected);
        }
        void button_select() {

        }
        void button_back() {

        }
};


#endif