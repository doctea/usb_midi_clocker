#include "Config.h"

// MPK49 loop indicator
#if defined(ENABLE_LOOPER)

#include "midi_mpk49.h"
#include "project.h"
#include "menu.h"

extern bool mpk49_recording;
extern bool mpk49_playing;
class LooperRecStatus : public MenuItem {   
    public:
        LooperRecStatus() : MenuItem("mpk49_looper") {};

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

class LooperQuantizeChanger : public SelectorControl {
    midi_track *target;

    int quantizer_available_values[5] = { 0, 4, 3, 2, 1 };

    public:
        LooperQuantizeChanger(const char *label, midi_track *target) : SelectorControl(label) {
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
            return mpk49_loop_track.get_quantization_value();
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
            if (value==0) {
                strcpy(l, "N");
            } else if (value==1) {
                //return "¼";
                strcpy(l, "q");
            } else if (value==2) {
                strcpy(l, "8"); //return "8";
            } else if (value==3) {
                strcpy(l, "16"); //return "16";
            } else if (value==4) {
                strcpy(l, "32"); //return "32";
            } 
            //Serial.printf("get_label_for_value(%i) returning '%s'\n", value, l);
            return l;
        }

};



class TransposeControl : public NumberControl {
    midi_track *target;

    public:
        TransposeControl(const char* label, midi_track *target, int start_value, int min_value, int max_value) : NumberControl(label, start_value, min_value, max_value) {
            this->target = target;
        };
        TransposeControl(const char* label, midi_track *target) : TransposeControl(label, target, 0, -127, 127) {};
        
        virtual int get_current_value() override {
            return target->transpose;
        }

        virtual void set_current_value(int value) override { 
            target->set_transpose(value);
        }
};

// todo: merge functionality with SequencerStatus to share selection code
class LooperStatus : public MenuItem {
    int ui_selected_loop_number = 0;
    LooperRecStatus lrs = LooperRecStatus();
    public: 
        LooperStatus() : MenuItem("Looper") {};

        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = lrs.display(pos,selected,opened);
            tft.setCursor(pos.x,pos.y);
            //colours(selected);
            //header(label, pos, selected, opened);
            int x = pos.x, y = tft.getCursorY(); //.y;

            int button_size = 13;   // odd number to avoid triggering https://github.com/PaulStoffregen/ST7735_t3/issues/30
            x = 2;
            y++;

            for (int i = 0 ; i < NUM_LOOPS_PER_PROJECT ; i++) {
                int col = (project.loaded_loop_number==i) ?  ST77XX_GREEN :    // if currently loaded 
                             (ui_selected_loop_number==i)  ? ST77XX_YELLOW :   // if selected
                                                             ST77XX_BLUE;        

                if (i==ui_selected_loop_number) {
                    tft.drawRoundRect(x-1, y-1, button_size+2, button_size+2, 3, ST77XX_WHITE);
                } else {
                    tft.fillRoundRect(x-1, y-1, button_size+2, button_size+2, 3, ST77XX_BLACK);
                }
                if (project.is_selected_loop_number_empty(i)) {
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
            ui_selected_loop_number--;
            if (ui_selected_loop_number < 0)
                ui_selected_loop_number = NUM_LOOPS_PER_PROJECT-1;
            project.select_loop_number(ui_selected_loop_number);
            return true;
        }

        virtual bool knob_right() {
            ui_selected_loop_number++;
            if (ui_selected_loop_number >= NUM_LOOPS_PER_PROJECT)
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
                menu.set_message_colour(ST77XX_GREEN);
                menu.set_last_message(msg);
            } else {
                char msg[20] = "";
                sprintf(msg, "Error loading loop %i", ui_selected_loop_number);
                menu.set_message_colour(ST77XX_RED);
                menu.set_last_message(msg);
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
                menu.set_message_colour(ST77XX_GREEN);
                menu.set_last_message(msg);
            } else {
                char msg[20] = "";
                sprintf(msg, "Error saving loop %i", ui_selected_loop_number);
                menu.set_message_colour(ST77XX_RED);
                menu.set_last_message(msg);
            }

            return true;
        }
};
#endif