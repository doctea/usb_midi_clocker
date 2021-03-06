#include "Config.h"

#if defined(ENABLE_LOOPER)

//#include "midi_mpk49.h"
#include "behaviour_mpk49.h"
#include "project.h"
#include "mymenu.h"
#include "menu.h"

#include "menu_slotcontroller.h"

class LooperRecStatus : public MenuItem {   
    public:
        MIDITrack *loop_track = nullptr;
        LooperRecStatus(const char *label, MIDITrack *loop_track) : MenuItem(label) {
            this->loop_track = loop_track;
        };

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft->setCursor(pos.x,pos.y);
            header(label, pos, selected, opened);

            tft->setTextSize(2);
            if (this->loop_track->isRecording()) {
                colours(opened, RED);
                tft->print((char*)"[Rec]");
            } else {
                colours(opened, C_WHITE);
                tft->print((char*)"     ");
            }
            //tft->print("\n");
            colours(C_WHITE, BLACK);
            tft->print((char*)"  ");
            if (this->loop_track->isPlaying()) {
                colours(opened, GREEN);
                tft->print((char*)"[>>]");
            } else {
                colours(opened, BLUE);
                tft->print((char*)"[##]");
            }
            tft->print((char*)"\n");
            return tft->getCursorY();// + 10;
        }
};

class LooperQuantizeControl : public SelectorControl {
    MIDITrack *loop_track = nullptr;

    // TODO: add -1 and -2 for half-bar and bar respectively; maybe add -3 for two-bar and -4 for phrase too?
    //       to do this, will need to scroll thru selectors horizontally
    int quantizer_available_values[5] = { 0, 4, 3, 2, 1 };

    public:
        LooperQuantizeControl(const char *label, MIDITrack *target) : SelectorControl(label) {
            //num_values = sizeof(*available_values);
            this->loop_track = target;
            available_values = &quantizer_available_values[0];
            
            num_values = sizeof(quantizer_available_values)/sizeof(int);
        }

        virtual void setter (int new_value) {
            Serial.printf("LooperQuantizerChanger setting quantize value to %i\n", new_value);
            loop_track->set_quantization_value(new_value); //available_values[selected_value]);
            Serial.printf("did set!");
        }
        virtual int getter () {
            return loop_track->get_quantization_value();
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
                strcpy(l, "q"); //return "??";
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
    MIDITrack *loop_track = nullptr;

    public:
        LooperTransposeControl(const char* label, MIDITrack *target, int start_value, int min_value, int max_value) : NumberControl(label, start_value, min_value, max_value) {
            this->loop_track = target;
        };
        LooperTransposeControl(const char* label, MIDITrack *target) : LooperTransposeControl(label, target, 0, -127, 127) {};
        
        virtual int get_current_value() override {
            return loop_track->transpose;
        }

        virtual void set_current_value(int value) override { 
            loop_track->set_transpose(value);
        }
};

class LooperStatus : public SlotController {
    int ui_selected_loop_number = 0;

    LooperRecStatus *lrs = nullptr; //LooperRecStatus();
    public: 
        LooperStatus(const char *label, MIDITrack *loop_track) : SlotController(label) {
            this->lrs = new LooperRecStatus("Looper status", loop_track);
        }

        virtual void on_add() override {
            lrs->set_tft(this->tft);
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
            pos.y = lrs->display(pos,selected,opened);
            show_header = false;
            return SlotController::display(pos, selected, opened);
        }
};
#endif
