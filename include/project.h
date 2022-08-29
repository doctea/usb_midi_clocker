#ifndef PROJECT__INCLUDED
#define PROJECT__INCLUDED

#include "storage.h"
#include "midi_looper.h"

#include "behaviour_subclocker.h"
/*#include "behaviour_beatstep.h"
#include "behaviour_keystep.h"
#include "behaviour_mpk49.h"*/

#include "midi_mapper_matrix_manager.h"

//extern DeviceBehaviour_Subclocker *behaviour_subclocker;

#define NUM_SEQUENCE_SLOTS_PER_PROJECT  8
#define NUM_LOOP_SLOTS_PER_PROJECT      8

using namespace storage;

#ifdef ENABLE_LOOPER
    extern MIDITrack mpk49_loop_track;
#endif
#ifdef ENABLE_DRUM_LOOPER
    extern MIDITrack drums_loop_track;
#endif

class Project {
    bool sequence_slot_has_file[NUM_SEQUENCE_SLOTS_PER_PROJECT];
    bool loop_slot_has_file[NUM_LOOP_SLOTS_PER_PROJECT];

    bool debug = false;

    MIDITrack *temp_loop = new MIDITrack();

    void initialise_sequence_slots() {
        for (int i = 0 ; i < NUM_SEQUENCE_SLOTS_PER_PROJECT ; i++) {
            char filepath[255];
            sprintf(filepath, FILEPATH_SEQUENCE_FORMAT, this->current_project_number, i);
            sequence_slot_has_file[i] = SD.exists(filepath);
            Serial.printf("sequence_slot_has_file[i] = %i for %s\n", sequence_slot_has_file[i], filepath);
        }
    }
    void initialise_loop_slots(bool quick = true) {
        //MIDITrack temp_track = MIDITrack(&MIDIOutputWrapper(midi_out_bitbox, BITBOX_MIDI_CHANNEL));

        for (int i = 0 ; i < NUM_LOOP_SLOTS_PER_PROJECT ; i++) {
            char filepath[255];
            sprintf(filepath, FILEPATH_LOOP_FORMAT, this->current_project_number, i);
            loop_slot_has_file[i] = SD.exists(filepath);
            if (!quick && loop_slot_has_file[i]) {        // test whether file is actually empty or not
                Serial.printf("checking if slot %i is actually empty...\n");
                temp_loop->load_loop(this->current_project_number, i);
                Serial.printf("loaded ok\n");
                if (temp_loop->count_events()==0)
                    loop_slot_has_file[i] = false;
                Serial.printf("did count_events\n");
            }
            Serial.printf("loop_slot_has_file[i] = %i for %s\n", loop_slot_has_file[i], filepath);
        }
        temp_loop->clear_all();
    }
    public:
        int current_project_number = 0;

        int selected_sequence_number = 0;
        int loaded_sequence_number = -1;

        int selected_loop_number = 0;
        int loaded_loop_number = -1;

        bool hold_clock_settings = false;
        bool hold_sequencer_settings = false;
        
        Project() {
            //initialise_sequence_slots();
        }

        void setup_project() {
            setProjectNumber(this->current_project_number);

            initialise_sequence_slots();
            initialise_loop_slots(false);
        }

        bool load_matrix_mappings = true;
        void setLoadMatrixMappings(bool value = true) {
            this->load_matrix_mappings = value;
        }
        bool isLoadMatrixMappings() {
            return this->load_matrix_mappings;
        }

        void setHoldClockSettings(bool value = true) {
            this->hold_clock_settings = value;
        }
        bool isHoldClockSettings() {
            return this->hold_clock_settings;
        }
        void setHoldSequencerSettings(bool value = true) {
            this->hold_sequencer_settings = value;
        }
        bool isHoldSequencerSettings() {
            return this->hold_sequencer_settings;
        }

        void setProjectNumber(int number) {
            if (this->debug) Serial.printf("Project#setProjectNumber(%i)...\n", number);
            if (this->current_project_number!=number) {
                this->current_project_number = number;
                if (this->debug) Serial.printf("Switched to project number %i\n", this->current_project_number);
                make_project_folders(number);
                this->load_project_settings(number);
                this->initialise_loop_slots();
                this->initialise_sequence_slots();
            }
        }
        int getProjectNumber() {
            return this->current_project_number;
        }

        ////////////// clocks / sequences
        bool select_sequence_number(int sn) {
            Serial.printf("select_sequence_number %i\n", sn);
            selected_sequence_number = sn % NUM_SEQUENCE_SLOTS_PER_PROJECT;
            return sn == selected_sequence_number;
        }

        bool is_selected_sequence_number_empty(int sn) {
            return !sequence_slot_has_file[sn];
        }

        int get_selected_sequence_number() {
            return this->selected_sequence_number;
        }
        int get_loaded_sequence_number() {
            return this->loaded_sequence_number;
        }
        int get_max_sequence_slots() {
            return NUM_SEQUENCE_SLOTS_PER_PROJECT;
        }

        // load and save sequences / clock settings etc
        bool load_selected_sequence() {
            return load_sequence(selected_sequence_number);
        }
        bool save_selected_sequence() {
            return save_sequence(selected_sequence_number);
        }
        bool load_sequence() {
            return load_sequence(selected_sequence_number);
        }
        bool save_sequence() {
            return save_sequence(selected_sequence_number);
        }

        bool load_sequence(int selected_sequence_number) {
            Serial.printf("load for selected_sequence_number %i\n", selected_sequence_number);
            bool result = storage::load_sequence(current_project_number, selected_sequence_number, &storage::current_state);
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
        bool select_loop_number(int sn) {
            Serial.printf("select_loop_number %i\n", sn);
            selected_loop_number = sn % NUM_LOOP_SLOTS_PER_PROJECT;
            return selected_loop_number == sn;
        }

        bool is_selected_loop_number_empty(int sn) {
            return !loop_slot_has_file[sn];
        }
        void set_loop_slot_has_file(int slot, bool state = true) {
            loop_slot_has_file[slot] = state;
        }

        // load and save sequences / clock settings etc
        bool load_loop(int selected_loop_number) {
            return load_loop(selected_loop_number, &mpk49_loop_track);
        }
        bool load_loop() {
            return load_loop(this->selected_loop_number, &mpk49_loop_track);
        }
        bool save_loop() {
            return save_loop(this->selected_loop_number, &mpk49_loop_track);
        }
        bool save_loop(int selected_loop_number) {
            return this->save_loop(selected_loop_number, &mpk49_loop_track);
        }

        bool load_loop(int selected_loop_number, MIDITrack *track) {
            Serial.printf("load for selected_sequence_number %i/%i\n", current_project_number, selected_loop_number);
            //bool result = storage::load_sequence(selected_loop_number, &storage::current_state);
            bool result = track->load_loop(current_project_number, selected_loop_number);
            if (result)
                loaded_loop_number = selected_loop_number;
            return result;
        }
        bool save_loop(int selected_loop_number, MIDITrack *track) {
            Serial.printf("save for selected_sequence_number %i/%i\n", current_project_number, selected_loop_number);
            //bool result = storage::save_sequence(selected_loop_number, &storage::current_state);
            bool result = track->save_loop(current_project_number, selected_loop_number);
            if (result) {
                if (track->count_events()>0)
                    loop_slot_has_file[selected_loop_number] = true;
                loaded_loop_number = selected_loop_number;
            }
            return result;
        }

        // callbacks so project can respond to events eg on_phrase...
        bool auto_advance_sequencer = false;
        void on_phrase(int phrase) {
            phrase = phrase % NUM_SEQUENCE_SLOTS_PER_PROJECT;
            if (auto_advance_sequencer) {
                this->selected_sequence_number = phrase % NUM_SEQUENCE_SLOTS_PER_PROJECT;
                this->load_sequence(this->selected_sequence_number);
            }
            if (auto_advance_looper) {
                this->selected_loop_number = phrase % NUM_LOOP_SLOTS_PER_PROJECT;
                this->load_loop(this->selected_loop_number);
            }
        }
        bool is_auto_advance_sequencer() {
            return this->auto_advance_sequencer;
        }
        void set_auto_advance_sequencer(bool auto_advance_sequencer) {
            this->auto_advance_sequencer = auto_advance_sequencer;
        }

        bool auto_advance_looper = false;
        bool is_auto_advance_looper() {
            return this->auto_advance_looper;
        }
        void set_auto_advance_looper(bool auto_advance_looper) {
            this->auto_advance_looper = auto_advance_looper;
        }

        bool save_project_settings() {
            return this->save_project_settings(current_project_number);
        }
        bool save_project_settings(int save_to_project_number) {
            File myFile;

            // determine filename, delete if exists, and open the file up for writing
            char filename[255] = "";
            sprintf(filename, FILEPATH_PROJECT_SETTINGS_FORMAT, save_to_project_number);
            Serial.printf("save_sequence(%i) writing to %s\n", save_to_project_number, filename);
            if (SD.exists(filename)) {
                Serial.printf("%s exists, deleting first\n", filename);
                SD.remove(filename);
            }
            myFile = SD.open(filename, FILE_WRITE_BEGIN | (uint8_t)O_TRUNC); //FILE_WRITE_BEGIN);
            if (!myFile) {    
                Serial.printf("Error: couldn't open %s for writing\n", filename);
                return false;
            }

            // header
            myFile.println("; begin project");
            myFile.printf("id=%i\n", save_to_project_number);

            // subclocker settings
            myFile.printf("subclocker_divisor=%i\n",     behaviour_subclocker->get_divisor());
            myFile.printf("subclocker_delay_ticks=%i\n", behaviour_subclocker->get_delay_ticks());

            // midi matrix settings
            for (int source_id = 0 ; source_id < midi_matrix_manager->sources_count ; source_id++) {
                for (int target_id = 0 ; target_id < midi_matrix_manager->targets_count ; target_id++) {
                    if (midi_matrix_manager->is_connected(source_id,target_id)) {
                        myFile.printf(
                            "midi_matrix_map=%s|%s\n", 
                            midi_matrix_manager->sources[source_id].handle, 
                            midi_matrix_manager->targets[target_id].handle
                        );
                    }
                }
            }
            //midi_matrix_manager->save_to_file(myFile);

            myFile.println("; end project");
            myFile.close();
            Serial.println("Finished saving.");
            return true;
        }

        bool load_project_settings(int project_number) {
            File myFile;

            if (isLoadMatrixMappings()) {
                Serial.printf("load_project_settings(%i) resetting matrix!\n");
                midi_matrix_manager->reset_matrix(); 
            }

            char filename[255] = "";
            sprintf(filename, FILEPATH_PROJECT_SETTINGS_FORMAT, project_number);
            Serial.printf("load_project_settings(%i) opening %s\n", project_number, filename);
            myFile = SD.open(filename, FILE_READ);
            myFile.setTimeout(0);

            if (!myFile) {
                Serial.printf("Error: Couldn't open %s for reading!\n", filename);
                return false;
            }

            String line;
            while (line = myFile.readStringUntil('\n')) {
                load_project_parse_line(line);
            }
            Serial.println("Closing file..");
            myFile.close();
            Serial.println("File closed");

            //Serial.printf("Loaded preset from [%s] [%i clocks, %i sequences of %i steps]\n", filename, clock_multiplier_index, sequence_data_index, output->size_steps);
            current_project_number = project_number;
            Serial.printf("Loaded project settings.\n");
            return true;
        }

        void load_project_parse_line(String line) {
            if (line.charAt(0)==';') {
                return;  // skip comment lines
            } else if (line.startsWith("subclocker_divisor=")) {
                behaviour_subclocker->set_divisor((int) line.remove(0,String("subclocker_divisor=").length()).toInt());
            } else if (line.startsWith("subclocker_delay_ticks=")) {
                behaviour_subclocker->set_delay_ticks((int) line.remove(0,String("subclocker_delay_ticks=").length()).toInt());
            } else if (this->isLoadMatrixMappings() && line.startsWith("midi_output_map=")) {
                // legacy save format, pre-matrix
                Serial.printf("----\nLoading midi_output_map line '%s'\n", line.c_str());
                line = line.remove(0,String("midi_output_map=").length());
                int split = line.indexOf('|');
                String source_label = line.substring(0,split);
                source_label = source_label.replace("_output","");  // translate pre-matrix style naming to matrix-style naming
                String target_label = line.substring(split+1,line.length());
                midi_matrix_manager->connect(source_label.c_str(), target_label.c_str());
            } else if (this->isLoadMatrixMappings() && line.startsWith("midi_matrix_map=")) {
                // midi matrix version
                Serial.printf("----\nLoading midi_matrix_map line '%s'\n", line.c_str());
                line = line.remove(0,String("midi_matrix_map=").length());
                int split = line.indexOf('|');
                String source_label = line.substring(0,split);
                String target_label = line.substring(split+1,line.length());
                midi_matrix_manager->connect(source_label.c_str(), target_label.c_str());
            }
        }
};

extern Project project;

// for use by the Menu
void save_project_settings();

#endif