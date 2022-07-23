#include "Config.h"

// MPK49 loop indicator
#if defined(ENABLE_LOOPER)

//#include "midi_mpk49.h"
#include "behaviour_mpk49.h"
#include "project.h"
#include "mymenu.h"
#include "menu.h"

#include "menu_slotcontroller.h"

class LooperRecStatus : public MenuItem {   
    public:
        LooperRecStatus() : MenuItem("Loop status / slot") {};

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft->setCursor(pos.x,pos.y);
            header(label, pos, selected, opened);

            tft->setTextSize(2);
            if (behaviour_mpk49->recording) {
                colours(opened, RED);
                tft->print((char*)"[Rec]");
            } else {
                colours(opened, C_WHITE);
                tft->print((char*)"     ");
            }
            //tft->print("\n");
            colours(C_WHITE, BLACK);
            tft->print((char*)"  ");
            if (behaviour_mpk49->playing) {
                colours(opened, GREEN);
                tft->print((char*)"[>>]");
            } else {
                colours(opened, BLUE);
                tft->print((char*)"[##]");
            }
            tft->print("\n");
            return tft->getCursorY();// + 10;
        }
};

class LooperQuantizeControl : public SelectorControl {
    MIDITrack *target;

    // TODO: add -1 and -2 for half-bar and bar respectively; maybe add -3 for two-bar and -4 for phrase too?
    //       to do this, will need to scroll thru selectors horizontally
    int quantizer_available_values[5] = { 0, 4, 3, 2, 1 };

    public:
        LooperQuantizeControl(const char *label, MIDITrack *target) : SelectorControl(label) {
            //num_values = sizeof(*available_values);
            this->target = target;
            available_values = &quantizer_available_values[0];
            
            num_values = sizeof(quantizer_available_values)/sizeof(int);
        }

        virtual void setter (int new_value) {
            Serial.printf("LooperQuantizerChanger setting quantize value to %i\n", new_value);
            target->set_quantization_value(new_value); //available_values[selected_value]);
            Serial.printf("did set!");
        }
        virtual int getter () {
            return target->get_quantization_value();
        }
        /*int on_change() {
            Serial.printf("SelectorControl %s changed to %i!\n", label, available_values[selected_value_index]);
        }*/
        virtual const char*get_label_for_value(int value) {
            //static char value_label[20];
            //sprintf(value_label, "%i", value);
            //Serial.printf("get_label_for_value(%i) returning '%s'\n", value, value_label);
            //return value_label;
            static char l[3] = "?";
            // TODO: add -1 and -2 for half-bar and bar respectively; maybe add -3 for two-bar and -4 for phrase too?
            if (value==0) {
                strcpy(l, "N");
            } else if (value==1) {
                strcpy(l, "q"); //return "¼";
            } else if (value==2) {
                strcpy(l, "8");
            } else if (value==3) {
                strcpy(l, "16");
            } else if (value==4) {
                strcpy(l, "32");
            } 
            //Serial.printf("get_label_for_value(%i) returning '%s'\n", value, l);
            return l;
        }

};



class LooperTransposeControl : public NumberControl {
    MIDITrack *target;

    public:
        LooperTransposeControl(const char* label, MIDITrack *target, int start_value, int min_value, int max_value) : NumberControl(label, start_value, min_value, max_value) {
            this->target = target;
        };
        LooperTransposeControl(const char* label, MIDITrack *target) : LooperTransposeControl(label, target, 0, -127, 127) {};
        
        virtual int get_current_value() override {
            return target->transpose;
        }

        virtual void set_current_value(int value) override { 
            target->set_transpose(value);
        }
};

// todo: merge functionality with SequencerStatus to share selection code
//          maybe even make subclasses of SelectorControl?
class LooperStatus : public SlotController {
    int ui_selected_loop_number = 0;
    LooperRecStatus lrs = LooperRecStatus();
    public: 
        LooperStatus(char *label) : SlotController(label) {}

        virtual void on_add() override {
            lrs.set_tft(this->tft);
        };

        virtual int get_max_slots() override {
            return NUM_LOOP_SLOTS_PER_PROJECT;
        };
        virtual int get_loaded_slot() override {
            return project.loaded_loop_number;
        };
        virtual int get_selected_slot() override {
            return project.selected_loop_number;
        };
        virtual bool is_slot_empty(int i) override {
            return project.is_selected_loop_number_empty(i);
        };
        virtual bool move_to_slot_number(int i) override {
            return project.select_loop_number(i);
        };
        virtual bool load_slot_number(int i) override {
            return project.load_loop(i);
        };
        virtual bool save_to_slot_number(int i) override {
            return project.save_loop(i);
        };

        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = lrs.display(pos,selected,opened);
            show_header = false;
            return SlotController::display(pos, selected, opened);
        }

        /*virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = lrs.display(pos,selected,opened);
            tft->setCursor(pos.x,pos.y);
            //colours(selected);
            //header(label, pos, selected, opened);
            int x = pos.x, y = tft->getCursorY(); //.y;

            int button_size = 13;   // odd number to avoid triggering https://github.com/PaulStoffregen/ST7735_t3/issues/30
            x = 2;
            y++;

            const int radius = 3, offset_leftup = -1, offset_downright = 2;

            for (int i = 0 ; i < NUM_LOOP_SLOTS_PER_PROJECT ; i++) {
                int col = (project.loaded_loop_number==i) ?  GREEN :    // if currently loaded 
                             (ui_selected_loop_number==i)  ? YELLOW :   // if selected
                                                             BLUE;        

                if (i==ui_selected_loop_number) {
                    tft->drawRoundRect(x-offset_leftup, y-offset_leftup, button_size+offset_downright, button_size+offset_downright, radius, C_WHITE);
                } else {
                    tft->fillRoundRect(x-offset_leftup, y-offset_leftup, button_size+offset_downright, button_size+offset_downright, radius, BLACK);
                }
                if (project.is_selected_loop_number_empty(i)) {
                    tft->drawRoundRect(x, y, button_size, button_size, 3, col);
                } else {
                    tft->fillRoundRect(x, y, button_size, button_size, 3, col);
                }
                x += button_size + 2;
            }
            y += button_size + 4;
            return y; //tft.getCursorY() + 8;
        }

        virtual bool knob_left() {
            ui_selected_loop_number--;
            if (ui_selected_loop_number < 0)
                ui_selected_loop_number = NUM_LOOP_SLOTS_PER_PROJECT-1;
            project.select_loop_number(ui_selected_loop_number);
            return true;
        }

        virtual bool knob_right() {
            ui_selected_loop_number++;
            if (ui_selected_loop_number >= NUM_LOOP_SLOTS_PER_PROJECT)
                ui_selected_loop_number = 0;
            project.select_loop_number(ui_selected_loop_number);
            return true;
        }

        virtual bool button_select() {
            project.select_loop_number(ui_selected_loop_number);
            bool success = project.load_loop(ui_selected_loop_number, &mpk49_loop_track);
            if (success) {
                //loaded_sequence_number = ui_selected_sequence_number;
                char msg[20] = "";
                sprintf(msg, "Loaded loop %i", project.loaded_loop_number);
                menu->set_message_colour(GREEN);
                menu->set_last_message(msg);
            } else {
                char msg[20] = "";
                sprintf(msg, "Error loading loop %i", ui_selected_loop_number);
                menu->set_message_colour(RED);
                menu->set_last_message(msg);
            }
            return true;
        }

        virtual bool button_right() {
            project.select_loop_number(ui_selected_loop_number);
            bool success = project.save_loop(ui_selected_loop_number, &mpk49_loop_track);
            if (success) {
                //loaded_sequence_number = ui_selected_sequence_number;
                char msg[20] = "";
                sprintf(msg, "Saved loop %i", project.loaded_loop_number);
                menu->set_message_colour(GREEN);
                menu->set_last_message(msg);
            } else {
                char msg[20] = "";
                sprintf(msg, "Error saving loop %i", ui_selected_loop_number);
                menu->set_message_colour(RED);
                menu->set_last_message(msg);
            }

            return true;
        }*/
};
#endif
