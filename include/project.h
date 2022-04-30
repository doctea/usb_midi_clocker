#ifndef PROJECT__INCLUDED
#define PROJECT__INCLUDED

#include "storage.h"

#define NUM_STATES_PER_PROJECT 8

using namespace storage;
//extern storage::savestate current_state;

class Project {
    public:
        int selected_sequence_number = 0;
        
        Project() {}

        void select_sequence_number(int sn) {
            Serial.printf("select_sequence_number %i\n", sn);
            selected_sequence_number = sn % NUM_STATES_PER_PROJECT;
        }

        bool load_state() {
            return load_state(selected_sequence_number);
        }
        bool save_state() {
            return save_state(selected_sequence_number);
        }

        bool load_state(int selected_sequence_number) {
            Serial.printf("load for selected_sequence_number %i\n", selected_sequence_number);
            return storage::load_state(selected_sequence_number, &storage::current_state);
        }

        bool save_state(int selected_sequence_number) {
            Serial.printf("save for selected_sequence_number %i\n", selected_sequence_number);
            return storage::save_state(selected_sequence_number, &storage::current_state);
        }

};


extern Project project;

#endif