#include "Config.h"

#if defined(ENABLE_LOOPER)

//#include "midi/midi_mpk49.h"
#include "behaviours/behaviour_mpk49.h"
#include "project.h"
#include "mymenu.h"
#include "menu.h"

#include "mymenu/menuitems_harmony.h"

#ifdef ENABLE_LOOPER_PIANOROLL
    #include "mymenu/menu_looperdisplay.h"
#endif
#include "mymenu/menu_slotcontroller.h"

class LooperRecStatus : public MenuItem {   
    public:
        MIDITrack *loop_track = nullptr;
        LooperRecStatus(const char *label, MIDITrack *loop_track) : MenuItem(label), loop_track(loop_track) {};

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft->setCursor(pos.x,pos.y);
            header(label, pos, selected, opened);

            tft->setTextSize(2);
            if (this->loop_track->isOverwriting()        && !this->loop_track->isRecording()) {
                colours(opened, ORANGE);
                tft->print((char*)"[Wip]");
            } else if (this->loop_track->isOverwriting() &&  this->loop_track->isRecording()) {
                colours(opened, RED);
                tft->print((char*)"[OvR]");
            } else if (this->loop_track->isRecording()) {
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
                if (this->loop_track->isOverwriting()) {
                    colours(opened, ORANGE);
                } else {
                    colours(opened, GREEN);
                }
                tft->print((char*)"[>>]");
            } else {
                colours(opened, BLUE);
                tft->print((char*)"[##]");
            }
            tft->print((char*)"\n");
            return tft->getCursorY();// + 10;
        }
};

class LooperQuantizeControl : public SelectorControl<int> {
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
            //static char value_label[MENU_C_MAX];
            //sprintf(value_label, "%i", value);
            //Serial.printf("get_label_for_value(%i) returning '%s'\n", value, value_label);
            //return value_label;
            static char l[3] = "?";
            // TODO: add -1 and -2 for half-bar and bar respectively; maybe add -3 for two-bar and -4 for phrase too?
            if (value==0) {
                strncpy(l, "N", 3);
            } else if (value==1) {
                strncpy(l, "q", 3); //return "¼";
            } else if (value==2) {
                strncpy(l, "8", 3);
            } else if (value==3) {
                strncpy(l, "16", 3);
            } else if (value==4) {
                strncpy(l, "32", 3);
            } 
            //Serial.printf("get_label_for_value(%i) returning '%s'\n", value, l);
            return l;
        }

};

//template<class DataType = int>
class LooperTransposeControl : public NumberControl<int> {
    MIDITrack *loop_track = nullptr;

    public:
        LooperTransposeControl(const char* label, MIDITrack *target, int start_value, int min_value, int max_value) 
            : NumberControl(label, start_value, min_value, max_value) 
        {
            this->loop_track = target;
        };
        LooperTransposeControl(const char* label, MIDITrack *target) : LooperTransposeControl(label, target, 0, -127, 127) {};
        
        virtual int get_current_value() override {
            return loop_track->transpose_amount;
        }

        virtual void set_current_value(int value) override { 
            loop_track->set_transpose(value);
        }
};


// this is the 'parent' widget, it sets up and draws the LooperRecStatus and LooperDisplay widgets since they don't need to be interacted with
class LooperStatus : public SlotController {
    int ui_selected_loop_number = 0;

    LooperRecStatus *lrs = nullptr; //LooperRecStatus();
    #ifdef ENABLE_LOOPER_PIANOROLL
        LooperDisplay *lds = nullptr;
    #endif
    HarmonyStatus *lhs = nullptr;
    public: 
        LooperStatus(const char *label, MIDITrack *loop_track) : SlotController(label) {
            this->lrs = new LooperRecStatus("Looper status", loop_track);
            #ifdef ENABLE_LOOPER_PIANOROLL
            if (loop_track->bitmap_enabled) {
                this->lds = new LooperDisplay("Piano roll", loop_track);
                this->lds->show_header = false;
            }
            #endif
            this->lhs = new HarmonyStatus("Last / current note", &loop_track->last_note, &loop_track->current_note);
            this->show_header = false;
        }

        virtual void on_add() override {
            this->lrs->set_tft(this->tft);
            #ifdef ENABLE_LOOPER_PIANOROLL
                this->lds->set_tft(this->tft);
            #endif
            this->lhs->set_tft(this->tft);
        };

        virtual void update_ticks(unsigned long ticks) override {
            if (this->lrs!=nullptr) this->lrs->update_ticks(ticks);
            #ifdef ENABLE_LOOPER_PIANOROLL
                if (this->lds!=nullptr) this->lds->update_ticks(ticks);
            #endif
            if (this->lhs!=nullptr) this->lhs->update_ticks(ticks);
        }

        virtual int get_max_slots() override {
            return NUM_LOOP_SLOTS_PER_PROJECT;
        };
        virtual int get_loaded_slot() override {
            return project->loaded_loop_number;
        };
        virtual int get_selected_slot() override {
            return project->selected_loop_number;
        };
        virtual bool is_slot_empty(int i) override {
            return project->is_selected_loop_number_empty(i);
        };
        virtual bool move_to_slot_number(int i) override {
            return project->select_loop_number(i);
        };
        virtual bool load_slot_number(int i) override {
            return project->load_loop(i);
        };
        virtual bool save_to_slot_number(int i) override {
            return project->save_loop(i);
        };

        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = lrs->display(pos, selected, opened);                // draw the loop record status widget first (with header)
            pos.y = lhs->display(pos, false, false);                    // draw the last/current note display (with header, unhighlighted)
            pos.y = SlotController::display(pos, selected, opened);     // draw the loop selection widget (without header)
            #ifdef ENABLE_LOOPER_PIANOROLL
                pos.y = lds->display(pos, selected, opened);    // if its enabled, draw the pianoroll (without header)
            #endif
            
            return pos.y;
        }
};
#endif
