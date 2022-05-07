#ifndef MENU__INCLUDED
#define MENU__INCLUDED

#include "Config.h"

#include <LinkedList.h>
#include <Adafruit_GFX.h>
#include "ST7789_t3.h"
#include "Vector.h"
//#include "menu.h"

//#include "midi_helpers.h"
//#include "menu.h"

#include "debug.h"

#include "tft.h"
#include "bpm.h"

//#include "project.h"

class Coord {
    public:
        int x, y;
        Coord(int in_x, int in_y) {
            x = in_x;
            y = in_y;
        }
};


// basic line
class MenuItem {
    public:
        char label[20];

        MenuItem(const char *in_label) {
            strcpy(label, in_label);
        }
        virtual int display(Coord pos, bool selected, bool opened) {
            //Serial.printf("base display for %s\n", label);
            // display this item however that may be
            tft.setCursor(pos.x,pos.y);
            //tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
            colours(selected);
            tft.print(label);
            //return (tft.getTextSizeY() * 8) + 2;
            return tft.getCursorY();
        }

        void colours(bool selected, int fg = ST77XX_WHITE, int bg = ST77XX_BLACK) {
            if (!selected) {
                tft.setTextColor(fg, bg);
            } else {
                tft.setTextColor(bg, fg) ;//ST77XX_BLACK, ST77XX_WHITE);
            }
        }

        int header(const char *text, Coord pos, bool selected = false, bool opened = false) {
            tft.drawLine(pos.x, pos.y, tft.width(), pos.y, ST7735_WHITE);
            tft.setCursor(pos.x, pos.y+1);
            colours(selected);
            tft.setTextSize(0);
            if (opened) {
                tft.print(">>>");
                tft.printf("%-19s",text);   // \n not needed as reaching to edge
            } else {
                tft.printf("%-22s",text);   // \n not needed as reaching to edge
            }
            //return (tft.getTextSize()+1)*6;
            return tft.getCursorY();
        }

        virtual bool button_select() {
            return false;
        }
        
        virtual bool button_back() {
            return false;
        }
        
        virtual bool button_right() {
            return false;
        }

        virtual bool knob_left() {
            return false;
        }

        virtual bool knob_right() {
            return false;
        }

        virtual bool allow_takeover() {
            return false;
        }
};

class Menu {
    int currently_selected  = -1;
    int currently_opened    = -1;
    LinkedList<MenuItem*> items = LinkedList<MenuItem*>();

    int panel_height[20];

    int last_knob_position;
    int button_count;

    void knob_turned(int knob_position) {
        if (knob_position < last_knob_position) {
            //set_bpm(bpm_current-1);
            knob_left();
        } else if (knob_position > last_knob_position) {
            //set_bpm(bpm_current+1);
            knob_right();
        }
        last_knob_position = knob_position;
        // do some action when knob is turned
    }
    bool knob_left() {
        Serial.println("knob_left()");
        if (currently_opened!=-1) { // && items.get(currently_opened)->knob_left()) {
            Serial.printf("knob_left on currently_opened %i\n", currently_opened);
            items.get(currently_opened)->knob_left();
        } else {
            currently_selected--;
            if (currently_selected<0) 
                currently_selected = items.size()-1;
        }
        return true;
    }
    bool knob_right() {
        Serial.println("knob_right()");
        if (currently_opened!=-1) { //&& items.get(currently_opened)->knob_right()) {
            Serial.printf("knob_right on currently_opened %i\n", currently_opened);
            items.get(currently_opened)->knob_right();
        } else {
            currently_selected++;
            if (currently_selected >= items.size())
                currently_selected = 0;
        }
        return true;
    }
    bool button_select() {
        Serial.println("button_select()");
        if (currently_opened==-1) {
            Serial.printf("button_select with currently_opened -1 - selecting %i\n", currently_selected);
            currently_opened = currently_selected;
        } else {
            Serial.printf("button_select subselect on %i\n", currently_opened);
            items.get(currently_opened)->button_select();
        } 
        return true;
    }
    bool button_back() {
        Serial.println("button_back()");
        if (currently_opened!=-1 && !items.get(currently_opened)->button_back()) {
            Serial.printf("back with currently_opened %i and no subhandling, setting to -1\n", currently_opened);
            currently_selected = currently_opened;
            currently_opened = -1;
        } else {
            Serial.printf("back with currently_opened %i, handled by selected"); //setting to -1\n", currently_opened);
        }
        return true;
    }
    bool button_right() {
        Serial.println("button_right()");
        if (currently_opened!=-1) {
            if (items.get(currently_opened)->button_right()) {
                Serial.printf("right with currently_opened %i subhandled!\n", currently_opened);
            } else {
                Serial.printf("right with currently_opened %i not subhandled!\n", currently_opened);
            }
        } else {
            Serial.printf("right with nothing currently_opened\n"); //setting to -1\n", currently_opened);
        }
        return true;
    }

    public:
        char last_message[20] = "...started up...";
        uint32_t message_colour = ST77XX_WHITE;

        void add(MenuItem *m) {
            items.add(m);
        }

        // set the colour of the message (ie red / green for error / success)
        void set_message_colour(uint32_t colour) {
            message_colour = colour;
        }
        // set the message to display at top of display
        void set_last_message(const char *msg) {
            strcpy(last_message, msg);
        }

        // draw the menu display
        virtual int display() {
            int y = 0;
            tft.setCursor(0,0);

            // draw the last status message
            tft.setTextColor(message_colour,ST77XX_BLACK);
            tft.setTextSize(0);
            tft.printf("[%-20s]",last_message);
            y = tft.getCursorY();
            
            // now draw the menu
            if (currently_opened>=0 && items.get(currently_opened)->allow_takeover()) {
                // let the currently opened item take care of drawing all of the display
                items.get(currently_opened)->display(Coord(0,y), true, true);
            } else {
                static int panel_bottom[20];
                static bool bottoms_computed = false;

                // find number of panels to offset in order to ensure that selected panel is on screen?
                int start_panel = 0;
                if (currently_selected>0 && panel_bottom[currently_selected] > tft.height()) {
                    start_panel = currently_selected - 1;
                    // count backwards to find number of panels we have to go to fit currently_selected on screen...
                    int count_y = panel_bottom[currently_selected];
                    for (int i = currently_selected ; i > 0 ; i--) {
                        count_y -= panel_bottom[i];
                        if (count_y + panel_bottom[currently_selected] < tft.height()) {
                            start_panel = i;
                            break;
                        }
                    }
                    //tft.fillWindow(ST77XX_BLACK);
                    tft.fillRect(0, 0, tft.width(), tft.height(), ST77XX_BLACK);
                    start_panel = constrain(start_panel, 0, items.size()-1);
                } else {
                    start_panel = 0;
                    tft.fillRect(0, 0, tft.width(), tft.height(), ST77XX_BLACK);
                }

                // draw each menu item's panel
                int start_y = 0;
                for (int i = start_panel ; i < items.size() ; i++) {
                    MenuItem *item = items.get(i);
                    //int time = millis();
                    y = item->display(Coord(0,y), i==currently_selected, i==currently_opened) + 1;

                    if (!bottoms_computed) {
                        panel_bottom[i] = y;// - start_y;
                        start_y = y;
                    }
                    //Serial.printf("menuitem %i took %i to refresh\n", i, millis()-time);
                }
                bottoms_computed = true;

                // control debug output (knob positions / button presses)
                tft.setCursor(0, y);
                tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
                //tft.println();
                tft.setTextSize(2);
                tft.printf("K:%2i B:%2i\n", last_knob_position, button_count);
                tft.printf("S:%2i O:%2i\n", currently_selected, currently_opened);
                tft.printf("Mem:%i\n", freeRam());
            }

            tft.updateScreenAsync(false);

            return y;
        }

        void update_inputs() {
            //static int button_count = 0;
            static int last_knob_read = 0, new_knob_read;
            //int new_knob_read;
            new_knob_read = knob.read();///4;
            if (new_knob_read!=last_knob_read) {
                last_knob_read = new_knob_read/4;
                //if (last_knob_read<0) 
                //    last_knob_read = MAX_KNOB;
                knob_turned(last_knob_read);
            }
            if (pushButtonA.update()) {
                if (pushButtonA.fallingEdge()) {
                    button_count++;
                    button_select();
                }
            }
            if (pushButtonB.update()) {
                if (pushButtonB.fallingEdge()) {
                    button_count++;
                    button_back();
                }
            }
            if (pushButtonC.update()) {
                if (pushButtonC.fallingEdge()) {
                    button_count++;
                    button_right();
                }
            }
        }

};

#ifdef ENABLE_BEATSTEP
    // BEATSTEP NOTES 
    #include "midi_beatstep.h"
#endif
String get_note_name(int pitch);
class HarmonyStatus : public MenuItem {
    int *last_note;
    int *current_note;

    public:
        HarmonyStatus() : MenuItem("Harmony") {};
        HarmonyStatus(const char *label, int *last_note, int *current_note) : MenuItem(label) {
            //MenuItem(label);
            this->last_note = last_note;
            this->current_note = current_note;
        }
        virtual int display(Coord pos, bool selected, bool opened) override {
            tft.setCursor(pos.x, pos.y);
            header(label, pos, selected, opened);
            //tft.setTextColor(rgb(0xFFFFFF),0);
            tft.setTextSize(2);
            colours(opened);

            if (!last_note || !current_note) {
                tft.printf("[not set]\n");
            } else {
                tft.printf("%4s : %4s",     // \n not needed because already fills row..
                    get_note_name(*last_note).c_str(), 
                    get_note_name(*current_note).c_str()
                );
            }
            return tft.getCursorY();
        }
};

// BPM indicator
class PositionIndicator : public MenuItem {
    public:
        PositionIndicator() : MenuItem("position") {};

        virtual int display(Coord pos, bool selected, bool opened) override {
            //Serial.printf("positionindicator display for %s\n", label);
            tft.setCursor(pos.x,pos.y);
            header("position", pos, selected, opened);
            tft.setTextSize(2);
            if (playing) {
                colours(opened, ST77XX_GREEN, ST77XX_BLACK);
            } else {
                colours(opened, ST77XX_RED, ST77XX_BLACK);
            }
            tft.printf("%04i:%02i:%02i @ %03.2f\n", 
                BPM_CURRENT_PHRASE + 1, 
                BPM_CURRENT_BAR_OF_PHRASE + 1,
                BPM_CURRENT_BEAT_OF_BAR + 1,
                bpm_current
            );

            return tft.getCursorY();
        }

        virtual bool knob_left() {
            set_bpm(bpm_current-1);
            return true;
        }
        virtual bool knob_right() {
            set_bpm(bpm_current+1);
            return true;
        }
};


#ifdef ENABLE_USB
    #include "multi_usb_handlers.h"
    class USBDevicesPanel : public MenuItem {
        public:
            USBDevicesPanel() : MenuItem("USB Devices") {}

            virtual int display(Coord pos, bool selected, bool opened) override {
                tft.setCursor(pos.x,pos.y);
                header("USB devices:", pos, selected, opened);
                colours(opened);
                tft.setTextSize(1);
                int connected = 0;
                for (int i = 0 ; i < NUM_USB_DEVICES ; i++) {
                    if (usb_midi_connected[i] && usb_midi_device[i] && usb_midi_device[i]->idVendor()>0) {
                        connected++;
                        tft.printf("%i %19s\n", i, usb_midi_device[i]->product());
                        //tft.printf("%08x\n", usb_midi_connected[i]);  // packed usb vendor+product id
                    }            
                }
                /*for (int i = 0 ; i < (NUM_USB_DEVICES - connected) ; i++) { // blank unused rows
                    tft.printf("%21s\n","");
                }*/
                return tft.getCursorY();
            }
    };
#endif


#endif