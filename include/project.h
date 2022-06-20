#ifndef PROJECT__INCLUDED
#define PROJECT__INCLUDED

#include "storage.h"
#include "midi_looper.h"

#define NUM_STATES_PER_PROJECT  8
#define NUM_LOOPS_PER_PROJECT   8

using namespace storage;

class Project;

class Project {
    bool sequence_slot_has_file[NUM_STATES_PER_PROJECT];
    bool loop_slot_has_file[NUM_LOOPS_PER_PROJECT];

    bool debug = false;

    void initialise_sequence_slots() {
        for (int i = 0 ; i < NUM_STATES_PER_PROJECT ; i++) {
            char filepath[255];
            sprintf(filepath, FILEPATH_SEQUENCE, this->current_project_number, i);
            sequence_slot_has_file[i] = SD.exists(filepath);
            Serial.printf("sequence_slot_has_file[i] = %i for %s\n", sequence_slot_has_file[i], filepath);
        }
    }
    void initialise_loop_slots(bool quick = true) {
        //MIDITrack temp_track = MIDITrack(&MIDIOutputWrapper(midi_out_bitbox, BITBOX_MIDI_CHANNEL));

        for (int i = 0 ; i < NUM_LOOPS_PER_PROJECT ; i++) {
            char filepath[255];
            sprintf(filepath, FILEPATH_LOOP, this->current_project_number, i);
            loop_slot_has_file[i] = SD.exists(filepath);
            if (!quick && loop_slot_has_file[i]) {        // test whether file is actually empty or not
                Serial.printf("checking if slot %i is actually empty...\n");
                mpk49_loop_track.load_state(this->current_project_number, i);
                Serial.printf("loaded ok\n");
                if (mpk49_loop_track.count_events()==0)
                    loop_slot_has_file[i] = false;
                Serial.printf("did count_events\n");
            }
            Serial.printf("loop_slot_has_file[i] = %i for %s\n", loop_slot_has_file[i], filepath);
        }
        mpk49_loop_track.clear_all();
    }
    public:
        int current_project_number = 0;

        int selected_sequence_number = 0;
        int loaded_sequence_number = -1;

        int selected_loop_number = 0;
        int loaded_loop_number = -1;
        
        Project() {
            //initialise_sequence_slots();
        }

        void setup_project() {
            setProjectNumber(this->current_project_number);

            initialise_sequence_slots();
            initialise_loop_slots(false);
        }

        void setProjectNumber(int number) {
            if (this->debug) Serial.printf("Project#setProjectNumber(%i)...\n", number);
            if (this->current_project_number!=number) {
                this->current_project_number = number;
                if (this->debug) Serial.printf("Switched to project number %i\n", this->current_project_number);
                this->initialise_loop_slots();
                this->initialise_sequence_slots();
                make_project_folders(number);
            }
        }
        int getProjectNumber() {
            return this->current_project_number;
        }

        ////////////// clocks / sequences
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
        bool save_sequence() {
            return save_sequence(selected_sequence_number);
        }

        bool load_state(int selected_sequence_number) {
            Serial.printf("load for selected_sequence_number %i\n", selected_sequence_number);
            bool result = storage::load_state(current_project_number, selected_sequence_number, &storage::current_state);
            if (result)
                loaded_sequence_number = selected_sequence_number;
            return result;
        }
        bool save_sequence(int selected_sequence_number) {
            Serial.printf("save for selected_sequence_number %i\n", selected_sequence_number);
            bool result = storage::save_sequence(current_project_number, selected_sequence_number, &storage::current_state);
            if (result) {
                sequence_slot_has_file[selected_sequence_number] = true;
                loaded_sequence_number = selected_sequence_number;
            }
            return result;
        }

        //////// loops/recordings
        void select_loop_number(int sn) {
            Serial.printf("select_loop_number %i\n", sn);
            selected_sequence_number = sn % NUM_LOOPS_PER_PROJECT;
        }

        bool is_selected_loop_number_empty(int sn) {
            return !loop_slot_has_file[sn];
        }
        void set_loop_slot_has_file(int slot, bool state = true) {
            loop_slot_has_file[slot] = state;
        }

        // load and save sequences / clock settings etc
        bool load_loop() {
            return load_loop(selected_loop_number, &mpk49_loop_track);
        }
        bool save_loop() {
            return save_loop(selected_loop_number, &mpk49_loop_track);
        }

        bool load_loop(int selected_loop_number, MIDITrack *track) {
            Serial.printf("load for selected_sequence_number %i/%i\n", current_project_number, selected_loop_number);
            //bool result = storage::load_state(selected_loop_number, &storage::current_state);
            bool result = track->load_state(current_project_number, selected_loop_number);
            if (result)
                loaded_loop_number = selected_loop_number;
            return result;
        }
        bool save_loop(int selected_loop_number, MIDITrack *track) {
            Serial.printf("save for selected_sequence_number %i/%i\n", current_project_number, selected_loop_number);
            //bool result = storage::save_sequence(selected_loop_number, &storage::current_state);
            bool result = track->save_sequence(current_project_number, selected_loop_number);
            if (result) {
                if (track->count_events()>0)
                    loop_slot_has_file[selected_loop_number] = true;
                loaded_loop_number = selected_loop_number;
            }
            return result;
        }
};

extern Project project;

#endif