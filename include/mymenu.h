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

#include "tft.h"
#include "bpm.h"

#include "project.h"


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
            tft.setCursor(pos.x, pos.y);
            tft.setTextColor(0xFFFFFF,ST77XX_BLACK);
            tft.setTextSize(0);
            if (opened) 
                tft.print(">>>");
            tft.printf("%-15s\n",text);
            return (tft.getTextSize()+1)*8;
        }

        virtual bool button_select() {
            return false;
        }
        
        virtual bool button_back() {
            return false;
        }

        virtual bool knob_left() {
            return false;
        }

        virtual bool knob_right() {
            return false;
        }
};

class Menu {
    int currently_selected  = -1;
    int currently_opened    = -1;
    LinkedList<MenuItem*> items = LinkedList<MenuItem*>();

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

    public:
        char last_message[20] = "started up";
        uint32_t message_colour = ST77XX_WHITE;

        void add(MenuItem *m) {
            items.add(m);
        }

        void set_message_colour(uint32_t colour) {
            message_colour = colour;
        }
        void set_last_message(const char *msg) {
            strcpy(last_message, msg);
        }

        virtual int display() {
            int y = 0;
            tft.setCursor(0,0);
            tft.setTextColor(message_colour,ST77XX_BLACK);
            tft.setTextSize(0);
            tft.printf("[%-18s]\n",last_message);
            y = tft.getCursorY();

            for (int i = 0 ; i < items.size() ; i++) {
                MenuItem *item = items.get(i);
                y = item->display(Coord(0,y), i==currently_selected, i==currently_opened) + 1;
                //tft.printf("position: %i\n", y);
                y = tft.getCursorY();
            }
            //controlPanel.display(Coord(0,y), currently_selected>=items.size(), currently_opened>=items.size()) + 1;
            //header("ControlDebug", pos, selected);
            //colours(selected);
            tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
            tft.println();
            tft.setTextSize(2);
            tft.printf("K:%2i B:%2i\n", last_knob_position, button_count);
            tft.printf("S:%2i O:%2i\n", currently_selected, currently_opened);
            //return tft.getCursorY();


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
                    //button_pressed(PIN_BUTTON_A);
                    button_select();
                }
            }
            if (pushButtonB.update()) {
                if (pushButtonB.fallingEdge()) {
                    button_count++;
                    //button_pressed(PIN_BUTTON_B);
                    button_back();
                }
            }
            return y;
        }

};


#include "sequencer.h"
#include "storage.h"
extern Menu menu;
class SequencerStatus : public MenuItem {
    int selected_sequence_number = 0;
    int loaded_sequence_number = -1;
    public: 
        SequencerStatus() : MenuItem("Sequencer") {}

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft.setCursor(pos.x,pos.y);
            colours(selected);
            header(label, pos, selected, opened);
            int x = pos.x, y = tft.getCursorY(); //.y;

            int button_size = 12;
            x = 2;
            for (int i = 0 ; i < NUM_STATES_PER_PROJECT ; i++) {
                tft.fillRoundRect(x, y, button_size, button_size, 3, 
                    loaded_sequence_number==i ? 
                        ST77XX_GREEN : 
                    selected && selected_sequence_number==i ? 
                        ST77XX_YELLOW : 
                        ST77XX_RED
                );
                x += button_size+2;
            }
            y += 6;
            return y; //tft.getCursorY() + 8;
        }

        virtual bool knob_left() {
            selected_sequence_number--;
            if (selected_sequence_number<0)
                selected_sequence_number = NUM_STATES_PER_PROJECT;
            return true;
        }

        virtual bool knob_right() {
            selected_sequence_number++;
            if (selected_sequence_number>= NUM_STATES_PER_PROJECT)
                selected_sequence_number = 0;
            return true;
        }

        virtual bool button_select() {
            project.select_sequence_number(selected_sequence_number);
            bool success = project.load_state(); //selected_sequence_number);
            if (success) {
                loaded_sequence_number = selected_sequence_number;
                char msg[20] = "";
                sprintf(msg, "Loaded %i", loaded_sequence_number);
                menu.set_message_colour(ST77XX_GREEN);
                menu.set_last_message(msg);
            } else {
                char msg[20] = "";
                sprintf(msg, "Error loading %i", selected_sequence_number);
                menu.set_message_colour(ST77XX_RED);
                menu.set_last_message(msg);
            }
            return true;
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

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft.setCursor(pos.x,pos.y);
            header(label, pos, selected, opened);

            tft.setTextSize(2);
            if (mpk49_recording) {
                //tft.setTextColor(rgb(0xFF,0,0),0);
                colours(selected, ST77XX_RED);
                tft.print("[Rec]");
            } else {
                colours(selected, ST77XX_WHITE);
                tft.print("     ");
            }
            if (mpk49_playing) {
                //tft.setTextColor(rgb(0x00,0xFF,0x00),0);
                colours(selected, ST77XX_GREEN);
                tft.print("[>>]");
            } else {
                //tft.setTextColor(rgb(0x00,0x00,0xFF),0);
                colours(selected, ST77XX_BLUE);
                tft.print("[##]");
            }
            //tft.print("\n");
            //y += (1+tft.getTextSizeY()) * 8;
            tft.println();
            return tft.getCursorY();// + 10;
        }
};
#endif


// BEATSTEP NOTES 
#include "midi_beatstep.h"
String get_note_name(int pitch);
class HarmonyStatus : public MenuItem {
    public:
        HarmonyStatus() : MenuItem("beatstep harmony") {
            //MenuItem(in_label);
        }
        virtual int display(Coord pos, bool selected, bool opened) override {
            tft.setCursor(pos.x, pos.y);

            header(label, pos, selected, opened);
            //tft.setTextColor(rgb(0xFFFFFF),0);
            tft.setTextSize(2);
            colours(selected);

            tft.printf("%4s : %4s\n", 
                get_note_name(last_beatstep_note).c_str(), 
                get_note_name(current_beatstep_note).c_str()
            );

            //return tft.getTextSizeY() * 8;
            return tft.getCursorY();
        }
};


// BPM indicator
class PositionIndicator : public MenuItem {
    public:
        PositionIndicator() : MenuItem("position") {
            //MenuItem(in_label);
        }
        virtual int display(Coord pos, bool selected, bool opened) override {
            //Serial.printf("positionindicator display for %s\n", label);
            tft.setCursor(pos.x,pos.y);

            header("position", pos, selected, opened);
            tft.setTextSize(2);
            if (playing) {
                colours(selected, ST77XX_GREEN, ST77XX_BLACK);
            } else {
                colours(selected, ST77XX_RED, ST77XX_BLACK);
            }
            tft.printf("%04i:%02i:%02i @ %03.2f\n", 
                (ticks / (PPQN*4*4)) + 1, 
                (ticks % (PPQN*4*4) / (PPQN*4)) + 1,
                (ticks % (PPQN*4) / PPQN) + 1,
                bpm_current
            );

            //return tft.getTextSizeY() * (2*8);
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


#include "multi_usb_handlers.h"
class USBDevicesPanel : public MenuItem {
    public:
        USBDevicesPanel() : MenuItem("USB Devices") {
            //MenuItem(in_label);
        }
        virtual int display(Coord pos, bool selected, bool opened) override {
            tft.setCursor(pos.x,pos.y);
            header("USB devices:", pos, selected, opened);
            colours(selected);
            tft.setTextSize(1);
            int connected = 0;
            for (int i = 0 ; i < NUM_USB_DEVICES ; i++) {
                if (usb_midi_connected[i] && usb_midi_device[i] && usb_midi_device[i]->idVendor()>0) {
                    connected++;
                    tft.printf("%i %19s\n", i, usb_midi_device[i]->product());
                }
            //tft->printf("%08x\n", usb_midi_connected[i]);
            }
            for (int i = 0 ; i < (NUM_USB_DEVICES - connected) ; i++) {
                tft.printf("%21s\n","");
            }
            //return ((NUM_USB_DEVICES - connected) + 2) * 8;
            return tft.getCursorY();
        }
};



#endif