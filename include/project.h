#ifndef PROJECT__INCLUDED
#define PROJECT__INCLUDED

#include "storage.h"
#include "midi_looper.h"

#include "behaviour_subclocker.h"
#include "behaviour_beatstep.h"
#include "behaviour_keystep.h"
#include "behaviour_mpk49.h"

#include "midi_mapper_manager.h"

#include "midi_lestrum.h"
#include "midi_pc_usb.h"
//extern DeviceBehaviour_Subclocker *behaviour_subclocker;

#define NUM_SEQUENCE_SLOTS_PER_PROJECT  8
#define NUM_LOOP_SLOTS_PER_PROJECT      8

using namespace storage;

//class Project;

void set_target_wrapper_for_names(String source_label, String target_label);

class Project {
    bool sequence_slot_has_file[NUM_SEQUENCE_SLOTS_PER_PROJECT];
    bool loop_slot_has_file[NUM_LOOP_SLOTS_PER_PROJECT];

    bool debug = false;

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
                mpk49_loop_track.load_loop(this->current_project_number, i);
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
        void select_sequence_number(int sn) {
            Serial.printf("select_sequence_number %i\n", sn);
            selected_sequence_number = sn % NUM_SEQUENCE_SLOTS_PER_PROJECT;
        }

        bool is_selected_sequence_number_empty(int sn) {
            return !sequence_slot_has_file[sn];
        }

        // load and save sequences / clock settings etc
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
        void select_loop_number(int sn) {
            Serial.printf("select_loop_number %i\n", sn);
            selected_sequence_number = sn % NUM_LOOP_SLOTS_PER_PROJECT;
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
        bool auto_advance = false;
        void on_phrase(int phrase) {
            phrase = phrase % NUM_SEQUENCE_SLOTS_PER_PROJECT;
            if (auto_advance) {
                this->selected_sequence_number = phrase;
                this->load_sequence(this->selected_sequence_number);
            }
            /*if (auto_advance) {
                int tested = 0;
                do {
                    this->selected_sequence_number++;
                    if (this->selected_sequence_number>NUM_SEQUENCE_SLOTS_PER_PROJECT) this->selected_sequence_number = 0;
                    tested++;
                } while(tested<NUM_SEQUENCE_SLOTS_PER_PROJECT && this->is_selected_sequence_number_empty(this->selected_sequence_number));
                this->load_sequence(this->selected_sequence_number);
            }*/
        }
        bool is_auto_advance() {
            return this->auto_advance;
        }
        void set_auto_advance(bool auto_advance) {
            this->auto_advance = auto_advance;
        }

        bool save_project_settings() {
            return this->save_project_settings(current_project_number);
        }
        bool save_project_settings(int save_to_project_number) {
            File myFile;

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
            myFile.println("; begin project");
            myFile.printf("id=%i\n", save_to_project_number);
            /*myFile.printf("size_clocks=%i\n",     input->size_clocks);
            myFile.printf("size_sequences=%i\n",  input->size_sequences);
            myFile.printf("size_steps=%i\n",      input->size_steps);
            for (int i = 0 ; i < input->size_clocks ; i++) {
                myFile.printf("clock_multiplier=%i\n", input->clock_multiplier[i]);
            }
            for (int i = 0 ; i < input->size_clocks ; i++) {
                myFile.printf("clock_delay=%i\n", input->clock_delay[i]);
            }
            for (int i = 0 ; i < input->size_sequences ; i++) {
                myFile.printf("sequence_data=");
                for (int x = 0 ; x < input->size_steps ; x++) {
                    myFile.printf("%1x", input->sequence_data[i][x]);
                }
                myFile.println("");
            }*/
            //behaviour_subclocker->get_divisor();

            // subclocker settings
            myFile.printf("subclocker_divisor=%i\n",     behaviour_subclocker->get_divisor());
            myFile.printf("subclocker_delay_ticks=%i\n", behaviour_subclocker->get_delay_ticks());

            // midi output mappings
            #ifdef ENABLE_BEATSTEP
                if(beatstep_output!=nullptr)        myFile.printf("midi_output_map=beatstep_output|%s\n",       beatstep_output->label);
            #endif
            #ifdef ENABLE_KEYSTEP
                if(keystep_output!=nullptr)         myFile.printf("midi_output_map=keystep_output|%s\n",        keystep_output->label);
            #endif
            #ifdef ENABLE_MPK49
                if(mpk49_output!=nullptr)           myFile.printf("midi_output_map=mpk49_output|%s\n",          mpk49_output->label);
            #endif
            #ifdef ENABLE_LESTRUM
                if(lestrum_pads_output!=nullptr)    myFile.printf("midi_output_map=lestrum_pads_output|%s\n",   lestrum_pads_output->label);
                if(lestrum_arp_output!=nullptr)     myFile.printf("midi_output_map=lestrum_arp_output|%s\n",    lestrum_arp_output->label);
            #endif
            if(pc_usb_1_output!=nullptr)        myFile.printf("midi_output_map=pc_usb_1_output|%s\n",       pc_usb_1_output->label);
            if(pc_usb_2_output!=nullptr)        myFile.printf("midi_output_map=pc_usb_2_output|%s\n",       pc_usb_2_output->label);

            myFile.println("; end project");
            myFile.close();
            Serial.println("Finished saving.");
            return true;
        }

        bool load_project_settings(int project_number) {
            File myFile;

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
            } else if (line.startsWith("midi_output_map=")) {
                line = line.remove(0,String("midi_output_map=").length());
                int split = line.indexOf('|');
                String source_label = line.substring(0,split);
                String target_label = line.substring(split+1,line.length());
                //MIDIOutputWrapper *target = find_wrapper_for_name((char*)target_label.c_str());
                set_target_wrapper_for_names(source_label, target_label);
            }
        }
};

extern Project project;

// for use by the Menu
void save_project_settings();

#endif