#ifndef MENU_SLOTCONTROLLER__INCLUDED
#define MENU_SLOTCONTROLLER__INCLUDED

#include "Config.h"

#include "mymenu.h"

extern Menu *menu;
//template<class TargetObject>
class SlotController : public MenuItem {
    int ui_selected_number = 0;
    /*using SelectCallback =      bool(*TargetClass::*)(int);
    using SlotEmptyCallback =   bool(TargetClass::*a)(int);
    using LoadButtonCallback =  bool(TargetClass::*a)();
    using SaveButtonCallback =  bool(TargetClass::*a)();
    using GetterCallback =      int(TargetClass::*a)(void);
    */
    public:
        SlotController(const char *label) : MenuItem(label) {};

        virtual int get_max_slots();
        virtual int get_loaded_slot();
        virtual int get_selected_slot();
        virtual bool is_slot_empty(int);
        virtual bool move_to_slot_number(int);
        virtual bool load_slot_number(int);
        virtual bool save_to_slot_number(int);

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft->setCursor(pos.x,pos.y);
            //colours(selected);
            header(label, pos, selected, opened);
            int x = pos.x, y = tft->getCursorY(); //.y;

            const unsigned int max_slots = this->get_max_slots(); //this->target->*get_max_callback();
            const int loaded_slot = this->get_loaded_slot(); //this->target->*getter_callback();

            unsigned int button_size = 13;   // odd number to avoid triggering https://github.com/PaulStoffregen/ST7735_t3/issues/30
            x = 2;
            y++;
            #define ROUNDED yes
            for (unsigned int i = 0 ; i < max_slots ; i++) {
                uint16_t colour = (loaded_slot==(int)i) ? GREEN :    // if currently loaded 
                                  (ui_selected_number==(int)i) ? YELLOW :   // if selected
                                                                BLUE;        

                if (((int)i)==ui_selected_number) {
                    #ifdef ROUNDED
                        tft->drawRoundRect(x-1, y-1, button_size+2, button_size+2, 1, C_WHITE);
                    #else
                        tft->drawRect(x-1, y-1, button_size+2, button_size+2, ST77XX_WHITE);
                    #endif
                } else {
                    #ifdef ROUNDED
                        tft->fillRoundRect(x-1, y-1, button_size+2, button_size+2, 1, BLACK);
                    #else  
                        tft->fillRect(x-1, y-1, button_size+2, button_size+2, ST77XX_BLACK);
                    #endif
                }
                //if (project.is_selected_pattern_number_empty(i)) {
                if (this->is_slot_empty(i)) { //this->target->*is_slot_empty_callback(i)) {
                    #ifdef ROUNDED
                        tft->drawRect(x, y, button_size, button_size, colour);
                    #else  
                        tft->drawRoundRect(x, y, button_size, button_size, 3, col);
                    #endif
                } else {
                    #ifdef ROUNDED
                        tft->fillRect(x, y, button_size, button_size, colour);
                    #else  
                        tft.fillRoundRect(x, y, button_size, button_size, 3, col);
                    #endif
                }
                if(this->get_selected_slot()==(int)i) {
                    tft->fillRect(x+(button_size/2), y+(button_size/2), 1+button_size/2, 1+button_size/2, ORANGE);
                }
                x += button_size + 2;
            }
            y += button_size + 4;
            return y; //tft.getCursorY() + 8;
        }

        virtual bool knob_left() override {
            ui_selected_number--;
            if (ui_selected_number < 0)
                ui_selected_number = this->get_max_slots()-1;
            //project.select_pattern_number(ui_selected_number);
            this->move_to_slot_number(ui_selected_number);
            return true;
        }

        virtual bool knob_right() override {
            ui_selected_number++;
            if (ui_selected_number >= this->get_max_slots())
                ui_selected_number = 0;
            //project.select_pattern_number(ui_selected_number);
            this->move_to_slot_number(ui_selected_number);
            return true;
        }

        // load
        virtual bool button_select() override {
            //project.select_pattern_number(ui_selected_number);
            //this->target->*move_to_slot_callback(ui_selected_number);
            //this->select_slot_number(ui_selected_number);
            //bool success = project.load_pattern(); //selected_pattern_number);
            //bool success = this->target->*select_callback(ui_selected_number);
            bool success = this->load_slot_number(ui_selected_number);
            uint8_t max_length = tft->get_c_max();
            if (success) {
                //loaded_pattern_number = ui_selected_number;
                char msg[tft->get_c_max()] = "";
                snprintf(msg, max_length, "Loaded %i", this->get_loaded_slot()); //this->target->*getter_callback());
                menu->set_message_colour(GREEN);
                menu->set_last_message(msg);
            } else {
                char msg[tft->get_c_max()] = "";
                snprintf(msg, max_length, "Error loading %i", ui_selected_number);
                menu->set_message_colour(RED);
                menu->set_last_message(msg);
            }

            return go_back_on_select;
        }

        // save 
        virtual bool button_right() override {
            //project.select_pattern_number(ui_selected_number);
            //this->target->*move_to_slot_callback(ui_selected_number);
            //bool success = project.save_pattern(ui_selected_number); //, &mpk49_loop_track);
            this->move_to_slot_number(ui_selected_number);
            bool success = this->save_to_slot_number(ui_selected_number); //this->target->*right_button_callback(ui_selected_number);
            uint8_t max_length = tft->get_c_max();
            if (success) {
                //loaded_pattern_number = ui_selected_number;
                char msg[tft->get_c_max()] = "";
                snprintf(msg, max_length, "Saved %i", this->get_loaded_slot()); //->target->*getter_callback()); //project.loaded_pattern_number);
                menu->set_message_colour(GREEN);
                menu->set_last_message(msg);
            } else {
                char msg[tft->get_c_max()] = "";
                snprintf(msg, max_length, "Error saving %i", ui_selected_number);
                menu->set_message_colour(RED);
                menu->set_last_message(msg);
            }

            return true;
        }
};

#endif 