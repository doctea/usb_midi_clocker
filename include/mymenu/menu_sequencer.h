#include "Config.h"

#ifdef ENABLE_SEQUENCER

#include "mymenu/menu_slotcontroller.h"

#include "sequencer.h"
#include "storage.h"
#include "project.h"
extern Menu *menu;
//extern Project project;
class SequencerStatus : public SlotController {
    //int ui_selected_sequence_number = 0;
    public: 
        SequencerStatus(const char *label) : SlotController(label) {};

        virtual int get_max_slots() override {
            return NUM_PATTERN_SLOTS_PER_PROJECT;
        };
        virtual int get_loaded_slot() override {
            return project->loaded_pattern_number;
        };
        virtual int get_selected_slot() override {
            return project->selected_pattern_number;
        };
        virtual bool is_slot_empty(int i) override {
            return project->is_selected_pattern_number_empty(i);
        };
        virtual bool move_to_slot_number(int i) override {
            return project->select_pattern_number(i);
        };
        virtual bool load_slot_number(int i) override {
            return project->load_pattern(i);
        };
        virtual bool save_to_slot_number(int i) override {
            return project->save_pattern(i);
        };
};

#endif