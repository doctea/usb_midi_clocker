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
            colours(selected);
            tft.setTextSize(0);
            if (opened) {
                tft.print(">>>");
                tft.printf("%-19s",text);   // \n not needed as reaching to edge
            } else {
                tft.printf("%-22s",text);   // \n not needed as reaching to edge
            }
            return (tft.getTextSize()+1)*6;
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

        virtual bool allow_takeover() {
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
            tft.printf("[%-19s]\n",last_message);
            y = tft.getCursorY();
            
            // now draw the menu
            if (currently_opened>=0 && items.get(currently_opened)->allow_takeover()) {
                // let the currently opened item take care of drawing all of the display
                items.get(currently_opened)->display(Coord(0,y), true, true);
            } else {
                // draw each menu item's panel
                for (int i = 0 ; i < items.size() ; i++) {
                    MenuItem *item = items.get(i);
                    //int time = millis();
                    y = item->display(Coord(0,y), i==currently_selected, i==currently_opened) + 1;
                    //Serial.printf("menuitem %i took %i to refresh\n", i, millis()-time);
                }

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
        }

};


#include "sequencer.h"
#include "storage.h"
extern Menu menu;
class SequencerStatus : public MenuItem {
    int ui_selected_sequence_number = 0;
    public: 
        SequencerStatus() : MenuItem("Sequencer") {}

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft.setCursor(pos.x,pos.y);
            //colours(selected);
            header(label, pos, selected, opened);
            int x = pos.x, y = tft.getCursorY(); //.y;

            int button_size = 12;
            x = 2;
            y++;
            for (int i = 0 ; i < NUM_STATES_PER_PROJECT ; i++) {
                int col = (project.loaded_sequence_number==i) ? ST77XX_GREEN :    // if currently loaded 
                             (ui_selected_sequence_number==i) ? ST77XX_YELLOW :   // if selected
                                                                ST77XX_BLUE;        

                if (i==ui_selected_sequence_number) {
                    tft.drawRoundRect(x-1, y-1, button_size+2, button_size+2, 1, ST77XX_WHITE);
                } else {
                    tft.fillRoundRect(x-1, y-1, button_size+2, button_size+2, 1, ST77XX_BLACK);
                }
                if (project.is_selected_sequence_number_empty(i)) {
                    tft.drawRoundRect(x, y, button_size, button_size, 3, col);
                } else {
                    tft.fillRoundRect(x, y, button_size, button_size, 3, col);
                }
                x += button_size + 2;
            }
            y += button_size + 4;
            return y; //tft.getCursorY() + 8;
        }

        virtual bool knob_left() {
            ui_selected_sequence_number--;
            if (ui_selected_sequence_number < 0)
                ui_selected_sequence_number = NUM_STATES_PER_PROJECT-1;
            project.select_sequence_number(ui_selected_sequence_number);
            return true;
        }

        virtual bool knob_right() {
            ui_selected_sequence_number++;
            if (ui_selected_sequence_number >= NUM_STATES_PER_PROJECT)
                ui_selected_sequence_number = 0;
            project.select_sequence_number(ui_selected_sequence_number);
            return true;
        }

        virtual bool button_select() {
            project.select_sequence_number(ui_selected_sequence_number);
            bool success = project.load_state(); //selected_sequence_number);
            if (success) {
                //loaded_sequence_number = ui_selected_sequence_number;
                char msg[20] = "";
                sprintf(msg, "Loaded %i", project.loaded_sequence_number);
                menu.set_message_colour(ST77XX_GREEN);
                menu.set_last_message(msg);
            } else {
                char msg[20] = "";
                sprintf(msg, "Error loading %i", ui_selected_sequence_number);
                menu.set_message_colour(ST77XX_RED);
                menu.set_last_message(msg);
            }
            return true;
        }
};

// MPK49 loop indicator
#if defined(ENABLE_RECORDING)
#include "midi_mpk49.h"

extern bool mpk49_recording;
extern bool mpk49_playing;
class LooperStatus : public MenuItem {   
    public:
        LooperStatus() : MenuItem("mpk49_looper") {};

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft.setCursor(pos.x,pos.y);
            header(label, pos, selected, opened);

            tft.setTextSize(2);
            if (mpk49_recording) {
                colours(opened, ST77XX_RED);
                tft.print("[Rec]");
            } else {
                colours(opened, ST77XX_WHITE);
                tft.print("     ");
            }
            colours(ST77XX_WHITE,ST77XX_BLACK);
            tft.print("  ");
            if (mpk49_playing) {
                colours(opened, ST77XX_GREEN);
                tft.print("[>>]");
            } else {
                colours(opened, ST77XX_BLUE);
                tft.print("[##]");
            }
            return tft.getCursorY();// + 10;
        }
};
#endif

// BEATSTEP NOTES 
#include "midi_beatstep.h"
String get_note_name(int pitch);
class HarmonyStatus : public MenuItem {
    public:
        HarmonyStatus() : MenuItem("beatstep harmony") {};

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft.setCursor(pos.x, pos.y);
            header(label, pos, selected, opened);
            //tft.setTextColor(rgb(0xFFFFFF),0);
            tft.setTextSize(2);
            colours(opened);

            tft.printf("%4s : %4s",     // \n not needed because already fills row..
                get_note_name(last_beatstep_note).c_str(), 
                get_note_name(current_beatstep_note).c_str()
            );
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