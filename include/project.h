#ifndef PROJECT__INCLUDED
#define PROJECT__INCLUDED

#include "storage.h"
#include "midi/midi_looper.h"

//#include "behaviours/behaviour_subclocker.h"
/*#include "behaviours/behaviour_beatstep.h"
#include "behaviours/behaviour_keystep.h"
#include "behaviours/behaviour_mpk49.h"*/

#include "midi/midi_mapper_matrix_manager.h"
#include "behaviours/behaviour_manager.h"

//extern DeviceBehaviour_Subclocker *behaviour_subclocker;

#include "mymenu/menu_fileviewers.h"

#define NUM_SEQUENCE_SLOTS_PER_PROJECT  8
#define NUM_LOOP_SLOTS_PER_PROJECT      8

using namespace storage;

#ifdef ENABLE_LOOPER
    extern MIDITrack midi_loop_track;
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
        Serial.println(F("initialise_sequence_slots starting.."));
        for (unsigned int i = 0 ; i < NUM_SEQUENCE_SLOTS_PER_PROJECT ; i++) {
            char filepath[MAX_FILEPATH];
            snprintf(filepath, MAX_FILEPATH, FILEPATH_SEQUENCE_FORMAT, this->current_project_number, i);
            sequence_slot_has_file[i] = SD.exists(filepath);
            Serial.printf(F("\tsequence_slot_has_file[i] = %i for %s\n"), sequence_slot_has_file[i], filepath);
        }
        Serial.println(F("initialise_sequence_slots finished"));
    }
    void initialise_loop_slots(bool quick = true) {
        //MIDITrack temp_track = MIDITrack(&MIDIOutputWrapper(midi_out_bitbox, BITBOX_MIDI_CHANNEL));
        temp_loop->bitmap_enabled = false;

        for (unsigned int i = 0 ; i < NUM_LOOP_SLOTS_PER_PROJECT ; i++) {
            char filepath[MAX_FILEPATH];
            snprintf(filepath, MAX_FILEPATH, FILEPATH_LOOP_FORMAT, this->current_project_number, i);
            loop_slot_has_file[i] = SD.exists(filepath);
            if (!quick && loop_slot_has_file[i]) {        // test whether file is actually empty or not
                Serial.printf(F("initialise_loop_slots: checking if loop slot %i is actually empty...\n"), i);
                temp_loop->load_loop(this->current_project_number, i);
                Serial.printf(F("initialise_loop_slots: loaded loop ok\n")); Serial_flush();
                if (temp_loop->count_events()==0)
                    loop_slot_has_file[i] = false;
                Serial.printf(F("initialise_loop_slots: did count_events\n"));
            }
            Serial.printf(F("initialise_loop_slots: loop_slot_has_file[i] = %i for %s\n"), loop_slot_has_file[i], filepath);
        }
        temp_loop->clear_all();
    }
    public:
        int current_project_number = 0;

        int selected_sequence_number = 0;
        int loaded_sequence_number = -1;

        int selected_loop_number = 0;
        int loaded_loop_number = -1;

        bool load_matrix_mappings = true;
        bool load_clock_settings = true;
        bool load_sequencer_settings = true;
        bool load_behaviour_options = true;
        
        Project() {
            //initialise_sequence_slots();
        }

        FLASHMEM void setup_project() {
            setProjectNumber(this->current_project_number);

            initialise_sequence_slots();
            initialise_loop_slots(false);
        }

        void setLoadMatrixMappings(bool value = true) {
            this->load_matrix_mappings = value;
        }
        bool isLoadMatrixMappings() {
            return this->load_matrix_mappings;
        }

        void setLoadClockSettings(bool value = true) {
            this->load_clock_settings = value;
        }
        bool isLoadClockSettings() {
            return this->load_clock_settings;
        }
        void setLoadSequencerSettings(bool value = true) {
            this->load_sequencer_settings = value;
        }
        bool isLoadSequencerSettings() {
            return this->load_sequencer_settings;
        }

        void setLoadBehaviourOptions(bool value = true) {
            this->load_behaviour_options = value;
        }
        bool isLoadBehaviourOptions() {
            return this->load_behaviour_options;
        }

        void setProjectNumber(int number) {
            if (this->debug) Serial.printf(F("Project#setProjectNumber(%i)...\n"), number);
            if (this->current_project_number!=number) {
                this->current_project_number = number;
                if (this->debug) Serial.printf(F("Switched to project number %i\n"), this->current_project_number);
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
            Serial.printf(F("select_sequence_number %i\n"), sn);
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
        bool load_specific_sequence(int selected_sequence_number) {
            return this->load_sequence(selected_sequence_number);
        }

        bool load_sequence(int selected_sequence_number) {
            Serial.printf(F("load for selected_sequence_number %i\n"), selected_sequence_number); Serial_flush();
            bool result = storage::load_sequence(current_project_number, selected_sequence_number, &storage::current_state);
            if (result)
                loaded_sequence_number = selected_sequence_number;
            Serial.println(F("returning\n"));  Serial_flush();
            return result;
        }
        bool save_sequence(int selected_sequence_number) {
            Serial.printf(F("save for selected_sequence_number %i\n"), selected_sequence_number); Serial_flush();
            bool result = storage::save_sequence(current_project_number, selected_sequence_number, &storage::current_state);
            if (result) {
                sequence_slot_has_file[selected_sequence_number] = true;
                loaded_sequence_number = selected_sequence_number;
            }
            return result;
        }

        //////// loops/recordings
        bool select_loop_number(int sn) {
            Serial.printf(F("select_loop_number %i\n"), sn);
            selected_loop_number = sn % NUM_LOOP_SLOTS_PER_PROJECT;
            return selected_loop_number == sn;
        }

        bool is_selected_loop_number_empty(int sn) {
            return !loop_slot_has_file[sn];
        }
        void set_loop_slot_has_file(int slot, bool state = true) {
            loop_slot_has_file[slot] = state;
        }

        void select_next_sequence() {
            selected_sequence_number++;
            if (selected_sequence_number>=NUM_SEQUENCE_SLOTS_PER_PROJECT)
                selected_sequence_number = 0;
        }
        void select_previous_sequence() {
            selected_sequence_number--;
            if (selected_sequence_number < 0) 
                selected_sequence_number = NUM_SEQUENCE_SLOTS_PER_PROJECT-1;
        }
        void load_next_sequence() {
            loaded_sequence_number++;
            if (loaded_sequence_number>=NUM_SEQUENCE_SLOTS_PER_PROJECT)
                loaded_sequence_number = 0;
            load_sequence(loaded_sequence_number);
        }
        void load_previous_sequence() {
            loaded_sequence_number--;
            if (loaded_sequence_number < 0)
                loaded_sequence_number = NUM_SEQUENCE_SLOTS_PER_PROJECT-1;
            load_sequence(loaded_sequence_number);
        }

        #ifdef ENABLE_LOOPER
            // load and save sequences / clock settings etc
            bool load_loop(int selected_loop_number) {
                return load_loop(selected_loop_number, &midi_loop_track);
            }
            bool load_loop() {
                return load_loop(this->selected_loop_number, &midi_loop_track);
            }
            bool save_loop() {
                return save_loop(this->selected_loop_number, &midi_loop_track);
            }
            bool save_loop(int selected_loop_number) {
                return this->save_loop(selected_loop_number, &midi_loop_track);
            }

            bool load_specific_loop(int selected_loop_number) {
                return this->load_loop(selected_loop_number);
            }
            bool load_loop(int selected_loop_number, MIDITrack *track) {
                Serial.printf(F("load for selected_loop_number project %i / loop %i\n"), current_project_number, selected_loop_number);
                //bool result = storage::load_sequence(selected_loop_number, &storage::current_state);
                bool result = track->load_loop(current_project_number, selected_loop_number);
                if (result)
                    loaded_loop_number = selected_loop_number;
                return result;
            }
            bool save_loop(int selected_loop_number, MIDITrack *track) {
                Serial.printf(F("save for selected_loop_number project %i / loop %i\n"), current_project_number, selected_loop_number);
                //bool result = storage::save_sequence(selected_loop_number, &storage::current_state);
                bool result = track->save_loop(current_project_number, selected_loop_number);
                if (result) {
                    if (track->count_events()>0)
                        loop_slot_has_file[selected_loop_number] = true;
                    loaded_loop_number = selected_loop_number;
                }
                return result;
            }

            bool auto_advance_looper = false;
            bool is_auto_advance_looper() {
                return this->auto_advance_looper;
            }
            void set_auto_advance_looper(bool auto_advance_looper) {
                this->auto_advance_looper = auto_advance_looper;
            }
        #endif

        // callbacks so project can respond to events eg on_phrase...
        bool auto_advance_sequencer = false;
        void on_phrase(int phrase) {
            int slot = phrase % NUM_SEQUENCE_SLOTS_PER_PROJECT;
            Debug_printf(F("Project#on_phrase(%i) called (slot %i)...\n"), phrase, slot);
            if (auto_advance_sequencer) {
                this->selected_sequence_number = slot % NUM_SEQUENCE_SLOTS_PER_PROJECT;
                this->load_sequence(this->selected_sequence_number);
            }
            #ifdef ENABLE_LOOPER
                if (auto_advance_looper) {
                    this->selected_loop_number = slot % NUM_LOOP_SLOTS_PER_PROJECT;
                    this->load_loop(this->selected_loop_number);
                }
            #endif
            Debug_printf(F("Project#on_phrase(%i) finished (slot %i)!\n"), phrase, slot);
        }
        bool is_auto_advance_sequencer() {
            return this->auto_advance_sequencer;
        }
        void set_auto_advance_sequencer(bool auto_advance_sequencer) {
            this->auto_advance_sequencer = auto_advance_sequencer;
        }


        bool save_project_settings() {
            return this->save_project_settings(current_project_number);
        }
        bool save_project_settings(int save_to_project_number) {
            #ifdef ENABLE_SD
            //bool irqs_enabled = __irq_enabled();
            //__disable_irq();
            File myFile;

            // determine filename, delete if exists, and open the file up for writing
            char filename[MAX_FILEPATH] = "";
            snprintf(filename, MAX_FILEPATH, FILEPATH_PROJECT_SETTINGS_FORMAT, save_to_project_number);
            Serial.printf(F("save_project_settings(%i) writing to %s\n"), save_to_project_number, filename);
            if (SD.exists(filename)) {
                Serial.printf(F("%s exists, deleting first\n"), filename);
                SD.remove(filename);
            }
            myFile = SD.open(filename, FILE_WRITE_BEGIN | (uint8_t)O_TRUNC); //FILE_WRITE_BEGIN);
            if (!myFile) {    
                Serial.printf(F("Error: couldn't open %s for writing\n"), filename);
                //if (irqs_enabled) __enable_irq();
                return false;
            }

            // header
            myFile.println(F("; begin project"));
            myFile.printf(F("id=%i\n"), save_to_project_number);

            // subclocker settings
            //myFile.printf("subclocker_divisor=%i\n",     behaviour_subclocker->get_divisor());
            //myFile.printf("subclocker_delay_ticks=%i\n", behaviour_subclocker->get_delay_ticks());
            LinkedList<String> behaviour_lines = LinkedList<String>();
            behaviour_manager->save_project_add_lines(&behaviour_lines);
            for (unsigned int i = 0 ; i < behaviour_lines.size() ; i++) {
                myFile.println(behaviour_lines.get(i));
            }

            // midi matrix settings
            for (int source_id = 0 ; source_id < midi_matrix_manager->sources_count ; source_id++) {
                for (int target_id = 0 ; target_id < midi_matrix_manager->targets_count ; target_id++) {
                    if (midi_matrix_manager->is_connected(source_id,target_id)) {
                        myFile.printf(
                            F("midi_matrix_map=%s|%s\n"), 
                            midi_matrix_manager->sources[source_id].handle, 
                            midi_matrix_manager->targets[target_id].handle
                        );
                    }
                }
            }
            //midi_matrix_manager->save_to_file(myFile);

            myFile.println(F("; end project"));
            myFile.close();
            Serial.println(F("Finished saving."));

            update_project_filename(filename);

            //if (irqs_enabled) __enable_irq();
            #endif
            return true;
        }

        bool load_project_settings(int project_number) {
            #ifdef ENABLE_SD
            //bool irqs_enabled = __irq_enabled();
            //__disable_irq();
            File myFile;

            messages_log_add(String("Loading project ") + String(project_number));

            if (isLoadMatrixMappings()) {
                Serial.printf(F("load_project_settings(%i) resetting matrix!\n"), project_number);
                midi_matrix_manager->reset_matrix(); 
            }

            char filename[MAX_FILEPATH] = "";
            snprintf(filename, MAX_FILEPATH, FILEPATH_PROJECT_SETTINGS_FORMAT, project_number);
            Serial.printf(F("load_project_settings(%i) opening %s\n"), project_number, filename);
            myFile = SD.open(filename, FILE_READ);
            myFile.setTimeout(0);

            if (!myFile) {
                Serial.printf(F("Error: Couldn't open %s for reading!\n"), filename);
                //if (irqs_enabled) __enable_irq();
                return false;
            }

            String line;
            while (line = myFile.readStringUntil('\n')) {
                load_project_parse_line(line);
            }
            Serial.println(F("Closing file.."));
            myFile.close();
            //if (irqs_enabled) __enable_irq();
            Serial.println(F("File closed"));

            //Serial.printf("Loaded preset from [%s] [%i clocks, %i sequences of %i steps]\n", filename, clock_multiplier_index, sequence_data_index, output->size_steps);
            current_project_number = project_number;
            Serial.printf(F("Loaded project settings.\n"));

            update_project_filename(filename);
            #endif

            return true;
        }

        void load_project_parse_line(String line) {
            if (line.charAt(0)==';') 
                return;  // skip comment lines

            String key = line.substring(0, line.indexOf('='));
            String value = line.substring(line.indexOf('=')+1);
            line = line.replace('\n',"");

            if (this->isLoadMatrixMappings() && line.startsWith(F("midi_output_map="))) {
                // legacy save format, pre-matrix
                Serial.printf(F("----\nLoading midi_output_map line '%s'\n"), line.c_str());
                line = line.remove(0,String(F("midi_output_map=")).length());
                int split = line.indexOf('|');
                String source_label = line.substring(0,split);
                source_label = source_label.replace(F("_output"),F(""));  // translate pre-matrix style naming to matrix-style naming
                String target_label = line.substring(split+1,line.length());
                midi_matrix_manager->connect(source_label.c_str(), target_label.c_str());
                return;
            } else if (this->isLoadMatrixMappings() && line.startsWith(F("midi_matrix_map="))) {
                // midi matrix version
                Serial.printf(F("----\nLoading midi_matrix_map line '%s'\n"), line.c_str());
                line = line.remove(0,String(F("midi_matrix_map=")).length());
                int split = line.indexOf('|');
                String source_label = line.substring(0,split);
                String target_label = line.substring(split+1,line.length());
                midi_matrix_manager->connect(source_label.c_str(), target_label.c_str());
                return;
            } else if (this->isLoadBehaviourOptions() && behaviour_manager->load_parse_line(line)) {
                // ask behaviour_manager to process the line
                //Serial.printf(F("project read line '%s', processed by behaviour_manager\n"), line.c_str());
                Serial.printf(F("line '%s' was processed by behaviour_manager\n"), line.c_str());
                return;
            }
            messages_log_add(String("Unknown Project line '") + line + String("'"));
        }
};

extern Project *project;

// for use by the Menu
void save_project_settings();

#endif