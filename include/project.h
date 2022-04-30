#ifndef PROJECT__INCLUDED
#define PROJECT__INCLUDED

#include "storage.h"

#define NUM_STATES_PER_PROJECT 8

using namespace storage;

class Project {
    bool sequence_slot_has_file[NUM_STATES_PER_PROJECT];

    void initialise_sequence_slots() {
        for (int i = 0 ; i < NUM_STATES_PER_PROJECT ; i++) {
            char filepath[255];
            sprintf(filepath, FILEPATH_SEQUENCE, i);
            sequence_slot_has_file[i] = SD.exists(filepath);
            Serial.printf("sequence_slot_has_file[i] = %i for %s\n", sequence_slot_has_file[i], filepath);
        }
    }
    public:
        int selected_sequence_number = 0;
        int loaded_sequence_number = -1;
        
        Project() {
            //initialise_sequence_slots();
        }

        void setup_project() {
            initialise_sequence_slots();
        }

        void select_sequence_number(int sn) {
            Serial.printf("select_sequence_number %i\n", sn);
            selected_sequence_number = sn % NUM_STATES_PER_PROJECT;
        }

        bool is_selected_sequence_number_empty(int sn) {
            return !sequence_slot_has_file[sn];
        }

        // load and save sequences / clock settings etc
        bool load_state() {
            return load_state(selected_sequence_number);
        }
        bool save_state() {
            return save_state(selected_sequence_number);
        }

        bool load_state(int selected_sequence_number) {
            Serial.printf("load for selected_sequence_number %i\n", selected_sequence_number);
            bool result = storage::load_state(selected_sequence_number, &storage::current_state);
            if (result)
                loaded_sequence_number = selected_sequence_number;
            return result;
        }
        bool save_state(int selected_sequence_number) {
            Serial.printf("save for selected_sequence_number %i\n", selected_sequence_number);
            bool result = storage::save_state(selected_sequence_number, &storage::current_state);
            if (result) {
                sequence_slot_has_file[selected_sequence_number] = true;
                loaded_sequence_number = selected_sequence_number;
            }
            return result;
        }

};


extern Project project;

#endif