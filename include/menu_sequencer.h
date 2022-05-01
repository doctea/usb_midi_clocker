#include "Config.h"

#ifdef ENABLE_SEQUENCER

#include "sequencer.h"
#include "storage.h"
#include "project.h"
extern Menu menu;
class SequencerStatus : public MenuItem {
    int ui_selected_sequence_number = 0;
    public: 
        SequencerStatus() : MenuItem("Sequencer") {};

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft.setCursor(pos.x,pos.y);
            //colours(selected);
            header(label, pos, selected, opened);
            int x = pos.x, y = tft.getCursorY(); //.y;

            int button_size = 13;   // odd number to avoid triggering https://github.com/PaulStoffregen/ST7735_t3/issues/30
            x = 2;
            y++;
            #define ROUNDED yes
            for (int i = 0 ; i < NUM_STATES_PER_PROJECT ; i++) {
                int col = (project.loaded_sequence_number==i) ? ST77XX_GREEN :    // if currently loaded 
                             (ui_selected_sequence_number==i) ? ST77XX_YELLOW :   // if selected
                                                                ST77XX_BLUE;        

                if (i==ui_selected_sequence_number) {
                    #ifdef ROUNDED
                        tft.drawRoundRect(x-1, y-1, button_size+2, button_size+2, 1, ST77XX_WHITE);
                    #else
                        tft.drawRect(x-1, y-1, button_size+2, button_size+2, ST77XX_WHITE);
                    #endif
                } else {
                    #ifdef ROUNDED
                        tft.fillRoundRect(x-1, y-1, button_size+2, button_size+2, 1, ST77XX_BLACK);
                    #else  
                        tft.fillRect(x-1, y-1, button_size+2, button_size+2, ST77XX_BLACK);
                    #endif
                }
                if (project.is_selected_sequence_number_empty(i)) {
                    #ifdef ROUNDED
                        tft.drawRect(x, y, button_size, button_size, col);
                    #else  
                        tft.drawRoundRect(x, y, button_size, button_size, 3, col);
                    #endif
                } else {
                    #ifdef ROUNDED
                        tft.fillRect(x, y, button_size, button_size, col);
                    #else  
                        tft.fillRoundRect(x, y, button_size, button_size, 3, col);
                    #endif
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

#endif