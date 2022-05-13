#ifndef MENU__INCLUDED
#define MENU__INCLUDED

#include "Config.h"

#include <LinkedList.h>
#include <Adafruit_GFX.h>
#include "ST7789_t3.h"
//#include "Vector.h"
//#include "menu.h"

#include "midi_helpers.h"
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

        virtual int draw_message() {
            //tft.setCursor(0,0);
            // draw the last status message
            tft.setTextColor(message_colour,ST77XX_BLACK);
            tft.setTextSize(0);
            tft.printf("[%-20s]",last_message);
            return tft.getCursorY();
        }

        virtual int draw_loop_markers() { //Coord pos) {
            //tft.setCursor(pos.x,pos.y);
            int LOOP_LENGTH = PPQN * BEATS_PER_BAR * BARS_PER_PHRASE;
            int y = 0;
            //y+=2;
            float percent = float(ticks % LOOP_LENGTH) / (float)LOOP_LENGTH;
            //tft.drawFastHLine(0, tft.width(), 3, ST77XX_WHITE);
            //tft.drawFastHLine(0, tft.width() * percent, 2, ST77XX_RED);
            tft.fillRect(0, y, (percent*(float)tft.width()), 6, ST77XX_RED);

            for (int i = 0 ; i < tft.width() ; i+=(tft.width()/(BEATS_PER_BAR*BARS_PER_PHRASE))) {
                tft.drawLine(i, y, i, y+2, ST7735_WHITE);
                //if (i%BEATS_PER_BAR==0)
                    //tft.drawLine(i, y, i, y+4, ST7735_CYAN);
            }

            for (int i = 0 ; i < tft.width() ; i+=(tft.width()/4)) {
                //tft.drawLine(i, y, i, y+4, ST7735_WHITE);
                tft.fillRect(i, y, 2, 5, ST7735_WHITE);
            }

            //Serial.printf("percent %f, width %i\n", percent, tft.width());
            y += 6;
            return y;
        }

        // draw the menu display
        virtual int display() {
            int y = 0;
            
            // now draw the menu
            if (currently_opened>=0 && items.get(currently_opened)->allow_takeover()) {
                y = draw_loop_markers();
                y = draw_message();
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

                tft.setCursor(0,0);

                y = draw_loop_markers();
                tft.setCursor(0, y);
                y = draw_message();

                // draw each menu item's panel
                //int start_y = 0;
                for (int i = start_panel ; i < items.size() ; i++) {
                    MenuItem *item = items.get(i);
                    //int time = millis();
                    y = item->display(Coord(0,y), i==currently_selected, i==currently_opened) + 1;

                    if (!bottoms_computed) {
                        panel_bottom[i] = y;// - start_y;
                        //start_y = y;
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

extern Menu menu;

// generic control for selecting a number
class NumberControl : public MenuItem {
    int *target_variable = nullptr;
    int internal_value = 0;
    int minimum_value = 0;
    int maximum_value = 4;

    void (*on_change_handler)(int last_value, int new_value);

    public:
        NumberControl(const char* label, int in_start_value, int min_value, int max_value) : MenuItem(label) {
            internal_value = in_start_value;
            minimum_value = min_value;
            maximum_value = max_value;
        };
        NumberControl(const char* label, int *in_target_variable, int start_value, int min_value, int max_value, void (*on_change_handler)(int last_value, int new_value)) 
            : MenuItem(label) {
            //NumberControl(label, start_value, min_value, max_value) {
            internal_value = start_value;
            minimum_value = min_value;
            maximum_value = max_value; 
            target_variable = in_target_variable;
            this->on_change_handler = on_change_handler;
        };

        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft.setCursor(pos.x,pos.y);

            colours(opened, opened?ST77XX_GREEN : ST77XX_WHITE, ST77XX_BLACK);
            tft.setTextSize(2);
            if (opened) {
                tft.printf("%*i\n", 4, internal_value);
            } else {
                tft.printf("%*i\n", 4, get_current_value()); //*target_variable); //target->transpose);
            }

            return tft.getCursorY();
        }

        virtual bool knob_left() {
            internal_value--;
            if (internal_value < minimum_value)
                internal_value = minimum_value; // = NUM_LOOPS_PER_PROJECT-1;
            //project.select_loop_number(ui_selected_loop_number);
            return true;
        }
        virtual bool knob_right() {
            internal_value++;
            if (internal_value >= maximum_value)
                internal_value = maximum_value;
            //project.select_loop_number(internal_value);
            return true;
        }
        virtual bool button_select() {
            //this->target->set_transpose(internal_value);
            int last_value = get_current_value();
            set_current_value(internal_value);
            if (on_change_handler!=nullptr) {
                Serial.println("calling on_change_handler");
                on_change_handler(last_value, internal_value);
            }
            return true;
        }

        // override in subclass if need to do something special eg getter/setter
        virtual int get_current_value() {
            if (target_variable!=nullptr)
                //return 0;
                return *target_variable;
            else
                return 0;
        }

        // override in subclass if need to do something special eg getter/setter
        virtual void set_current_value(int value) { 
            if (target_variable!=nullptr)
                *target_variable = value;
        }
};

// generic control for selecting one option from a selection of values
// TODO: keep currently selected option centred in display and scroll through the rest
class SelectorControl : public MenuItem {
    public:
        int num_values;
        int selected_value_index;
        int *available_values;

        virtual void setter (int new_value) {
        }
        virtual int getter () {
            return 0;
        }
        /*int on_change() {
            Serial.printf("SelectorControl %s changed to %i!\n", label, available_values[selected_value_index]);
        }*/
        virtual const char*get_label_for_value(int value) {
            static char value_label[20];
            sprintf(value_label, "%i", value);
            return value_label;
        }

        SelectorControl(const char *label) : MenuItem(label) {};
        SelectorControl(const char *label, byte initial_selected_value_index) : 
            SelectorControl(label) {
            this->selected_value_index = initial_selected_value_index;
        }

        // classic fixed display version
        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft.setTextSize(2);

            int current_value = this->getter();

            for (int i = 0 ; i < num_values ; i++) {
                //bool is_current_value_selected = selected_value_index==i; //available_values[i]==current_value;
                bool is_current_value_selected = available_values[i]==current_value; //getter();
                int col = is_current_value_selected ? ST7735_GREEN : ST7735_WHITE;
                colours(opened && selected_value_index==i, col, ST7735_BLACK);
                //colours(true, col, ST7735_BLUE);
                //Serial.printf("for item %i/%i, printing %s\n", i, num_values, get_label_for_value(available_values[i]));
                tft.printf("%s", get_label_for_value(available_values[i])); //available_values[i]);
                //tft.printf("%i", available_values[i]);
                tft.setTextColor(ST77XX_BLACK);
                if (i<num_values-1) 
                    tft.printf(" ");
            }
            if (tft.getCursorX()>0) // if we haven't wrapped onto next line then do it manually
                tft.println();
            return tft.getCursorY();
        }

        // cool, non-working rotating-option version
        /*virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft.setTextSize(2);

            int start_item = selected_value_index - 2;

            int current_value = this->getter();

            for (int i = start_item ; i < selected_value_index ; i++) {
                int actual_item = i;
                if (i<0) actual_item = num_values - i;
                if (i>=num_values) actual_item = selected_value_index + (num_values - i);

                bool is_current_value_selected = available_values[actual_item]==current_value; //getter();
                int col = is_current_value_selected ? ST7735_GREEN : ST7735_WHITE;
                colours(opened && selected_value_index==actual_item, col, ST7735_BLACK);
                tft.printf("%s", get_label_for_value(available_values[actual_item]));
                tft.setTextColor(ST77XX_BLACK);
                if (actual_item<num_values-1) 
                    tft.printf(" ");
                if (tft.getCursorX()>=tft.width()) 
                    break;
            }

            if (tft.getCursorX()>0) // if we haven't wrapped onto next line then do it manually
                tft.println();
            return tft.getCursorY();
        }*/

        virtual bool knob_left() {
            selected_value_index--;
            if (selected_value_index < 0)
                selected_value_index = num_values-1;
            return true;
        }

        virtual bool knob_right() {
            selected_value_index++;
            if (selected_value_index >= num_values)
                selected_value_index = 0;
            return true;
        }

        virtual bool button_select() {
            //Serial.printf("button_select with selected_value_index %i\n", selected_value_index);
            //Serial.printf("that is available_values[%i] of %i\n", selected_value_index, available_values[selected_value_index]);
            this->setter(available_values[selected_value_index]);

            char msg[255];
            //Serial.printf("about to build msg string...\n");
            sprintf(msg, "Set %s to %s (%i)", label, get_label_for_value(available_values[selected_value_index]), available_values[selected_value_index]);
            //Serial.printf("about to set_last_message!");
            msg[20] = '\0'; // limit the string so we don't overflow set_last_message
            menu.set_last_message(msg);
            menu.set_message_colour(ST77XX_GREEN);
            return true;
        }

};


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