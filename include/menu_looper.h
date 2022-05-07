#include "Config.h"

// MPK49 loop indicator
#if defined(ENABLE_RECORDING)

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

 // todo: make this part of the LooperStatus object, ie allow sub-menus
class LooperQuantizeChanger : public MenuItem {
    #define NUM_QUANTIZE_VALUES 4
    int available_values[NUM_QUANTIZE_VALUES] = { 1, 2, 3, 4 };
    int selected_value = 3; //mpk49_loop_track.quantize_value();

    public:
        LooperQuantizeChanger() : MenuItem("Looper quantization") {};

        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft.setTextSize(2);

            for (int i = 0 ; i < NUM_QUANTIZE_VALUES ; i++) {
                int col;
                if (available_values[i]==mpk49_loop_track.get_quantization_value()) {
                    col = ST7735_GREEN;
                }
                colours(opened && selected_value==i, col, ST7735_BLACK);
                tft.printf("%i", available_values[i]);
                tft.setTextColor(ST77XX_BLACK);
                tft.printf(" ");
            }
            tft.println();

            return tft.getCursorY();
        }

        virtual bool knob_left() {
            selected_value--;
            if (selected_value < 0)
                selected_value = NUM_QUANTIZE_VALUES-1;
            //project.select_loop_number(selected_value);
            return true;
        }

        virtual bool knob_right() {
            selected_value++;
            if (selected_value >= NUM_QUANTIZE_VALUES)
                selected_value = 0;
            //project.select_loop_number(selected_value);
            return true;
        }

        virtual bool button_select() {
            mpk49_loop_track.set_quantization_value(available_values[selected_value]);
            char msg[20];
            sprintf(msg,"Set quant to %i", available_values[selected_value]);
            menu.set_last_message(msg);
            menu.set_message_colour(ST77XX_GREEN);
            return true;
        }
};

class NumberControl : public MenuItem {
    int *target_variable = nullptr;
    int internal_value = 0;
    int minimum_value = 0;
    int maximum_value = 4;

    public:
        NumberControl(const char* label, int in_start_value, int min_value, int max_value) : MenuItem(label) {
            internal_value = in_start_value;
            minimum_value = min_value;
            maximum_value = max_value;
        };
        NumberControl(const char* label, int *in_target_variable, int start_value, int min_value, int max_value) : MenuItem(label) {
            //NumberControl(label, start_value, min_value, max_value) {
            internal_value = start_value;
            minimum_value = min_value;
            maximum_value = max_value; 
            target_variable = in_target_variable;
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
            set_current_value(internal_value);
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
