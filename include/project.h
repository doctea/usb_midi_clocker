#pragma once

#include "Config.h"

#include "storage.h"
#ifdef ENABLE_LOOPER
#include "midi/midi_looper.h"
#endif

#include "midi/midi_mapper_matrix_manager.h"
#include "behaviours/behaviour_manager.h"

#include "mymenu/menu_fileviewers.h"

#define NUM_SCENE_SLOTS_PER_PROJECT   8
#define NUM_LOOP_SLOTS_PER_PROJECT      8

using namespace storage;

#ifdef ENABLE_LOOPER
    extern MIDITrack midi_loop_track;
#endif
#ifdef ENABLE_DRUM_LOOPER
    extern MIDITrack drums_loop_track;
#endif

extern volatile bool global_load_lock;

class Project : public SHDynamic<0, 8> {
    bool scene_slot_has_file[NUM_SCENE_SLOTS_PER_PROJECT];
    #ifdef ENABLE_LOOPER
    bool loop_slot_has_file[NUM_LOOP_SLOTS_PER_PROJECT];
    #endif

    bool debug = false;

    #ifdef ENABLE_LOOPER
    MIDITrack *temp_loop = new MIDITrack();
    #endif

    void initialise_scene_slots() {
        #ifdef ENABLE_SD
            Serial.println(F("initialise_scene_slots starting.."));
            for (unsigned int i = 0 ; i < NUM_SCENE_SLOTS_PER_PROJECT ; i++) {
                scene_slot_has_file[i] = SD.exists(storage::get_scene_filename(this->current_project_number, i));
                Serial_printf(F("\tscene_slot_has_file[i] = %i for %s\n"), scene_slot_has_file[i], get_scene_filename(this->current_project_number, i));
            }
            Serial_println(F("initialise_scene_slots finished"));
        #else
            for (unsigned int i = 0 ; i < NUM_SCENE_SLOTS_PER_PROJECT ; i++) {
                scene_slot_has_file[i] = false;
            }
            Serial.println("ENABLE_SD not defined, so pattern slots not initialised");
        #endif
    }
    #ifdef ENABLE_LOOPER
    void initialise_loop_slots(bool quick = true) {
        //MIDITrack temp_track = MIDITrack(&MIDIOutputWrapper(midi_out_bitbox, BITBOX_MIDI_CHANNEL));
        temp_loop->bitmap_enabled = false;
        #ifdef ENABLE_SD
        for (unsigned int i = 0 ; i < NUM_LOOP_SLOTS_PER_PROJECT ; i++) {
            char filepath[MAX_FILEPATH];
            snprintf(filepath, MAX_FILEPATH, FILEPATH_LOOP_FORMAT, this->current_project_number, i);
            loop_slot_has_file[i] = SD.exists(filepath);
            if (!quick && loop_slot_has_file[i]) {        // test whether file is actually empty or not
                Serial_printf(F("initialise_loop_slots: checking if loop slot %i is actually empty...\n"), i);
                temp_loop->load_loop(this->current_project_number, i);
                Serial_printf(F("initialise_loop_slots: loaded loop ok\n")); Serial_flush();
                if (temp_loop->count_events()==0)
                    loop_slot_has_file[i] = false;
                Serial_printf(F("initialise_loop_slots: did count_events\n"));
            }
            Serial_printf(F("initialise_loop_slots: loop_slot_has_file[i] = %i for %s\n"), loop_slot_has_file[i], filepath);
        }
        temp_loop->clear_all();
        #else
            for (unsigned int i = 0 ; i < NUM_LOOP_SLOTS_PER_PROJECT ; i++) {
                loop_slot_has_file[i] = false;
            }
            Serial.println("ENABLE_SD not defined, so loop slots not initialised");
        #endif
    }
    #endif
    public:
        ISaveableSettingHost* save_tree = nullptr;  // set by setup_saveloadlib() after construction
        int current_project_number = 0;

        int selected_scene_number = 0;
        int loaded_scene_number = -1;

        #ifdef ENABLE_LOOPER
        int selected_loop_number = 0;
        volatile int loaded_loop_number = -1;
        #endif

        bool load_matrix_mappings = true;
        bool load_clock_settings = true;
        bool load_sequencer_settings = true;
        bool load_behaviour_options = true;
        bool load_parameter_input_options = true;

        Project() {
            //initialise_scene_slots();
        }

        // ---- saveloadlib integration ----
        // Registers project-scope scalar settings so they are saved/loaded when
        // sl_save_to_linkedlist / sl_load_from_file is called with SL_SCOPE_PROJECT.
        // Called automatically by sl_setup_all() via SettingsRoot::setup_saveable_settings().
        virtual void setup_saveable_settings() override {
            this->set_path_segment("project");
            register_setting(new LSaveableSetting<int>(
                "project_id", "Project", nullptr,
                [this](int v) { /* id on load is informational; project number set by caller */ },
                [this]() -> int { return this->current_project_number; }
            ), SL_SCOPE_PROJECT, false);
            register_setting(new LSaveableSetting<bool>(
                "auto_advance_scene", "Project", &this->auto_advance_scene
            ), SL_SCOPE_PROJECT, false);
            #ifdef ENABLE_LOOPER
            register_setting(new LSaveableSetting<bool>(
                "auto_advance_looper", "Project", &this->auto_advance_looper
            ), SL_SCOPE_PROJECT, false);
            #endif
        }

        FLASHMEM void setup_project() {
            setProjectNumber(this->current_project_number);

            initialise_scene_slots();
            #ifdef ENABLE_LOOPER
            initialise_loop_slots(false);
            #endif
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

        void setLoadParameterInputOptions(bool value = true) {
            this->load_parameter_input_options = value;
        }
        bool isLoadParameterInputOptions() {
            return this->load_parameter_input_options;
        }

        void notify_behaviours_for_project_change(int8_t project_number) {
            behaviour_manager->notify_behaviours_for_project_change(project_number);
        }

        void setProjectNumber(int number) {
            if (this->debug) Serial_printf(F("Project#setProjectNumber(%i)...\n"), number);
            //if (this->current_project_number!=number) {
                this->current_project_number = number;
                if (this->debug) Serial_printf(F("Switched to project number %i\n"), this->current_project_number);
                make_project_folders(number);
                this->load_project_settings(number);
                #ifdef ENABLE_LOOPER
                this->initialise_loop_slots();
                #endif
                this->initialise_scene_slots();
                this->notify_behaviours_for_project_change(number);
            //}
        }
        int getProjectNumber() {
            return this->current_project_number;
        }

        ////////////// clocks / sequences
        bool select_scene_number(int sn) {
            Serial_printf(F("select_scene_number %i\n"), sn); Serial_flush();
            selected_scene_number = sn % NUM_SCENE_SLOTS_PER_PROJECT;
            return sn == selected_scene_number;
        }

        bool is_selected_scene_number_empty(int sn) {
            return !scene_slot_has_file[sn];
        }

        int get_selected_scene_number() {
            return this->selected_scene_number;
        }
        int get_loaded_scene_number() {
            return this->loaded_scene_number;
        }
        int get_max_scene_slots() {
            return NUM_SCENE_SLOTS_PER_PROJECT;
        }

        // load and save sequences / clock settings etc
        bool load_selected_scene() {
            return load_scene(selected_scene_number);
        }
        bool save_selected_scene() {
            return save_scene(selected_scene_number);
        }
        bool load_scene(bool debug = false) {
            return load_scene(selected_scene_number, debug);
        }
        bool save_scene() {
            return save_scene(selected_scene_number);
        }
        bool load_specific_scene(int selected_scene_number) {
            return this->load_scene(selected_scene_number);
        }

        bool load_scene(int selected_scene_number, bool debug = false) {
            if (debug) { Serial.printf(F("load for selected_scene_number %i\n"), selected_scene_number); Serial_flush(); }
            uint32_t micros_start = micros();
            bool result = storage::load_scene(current_project_number, selected_scene_number, &storage::current_state, debug);
            if (result)
                loaded_scene_number = selected_scene_number;
            uint32_t micros_end = micros();
            Serial_printf(F("storage::load_scene took %lu microseconds (result: %s).\n"), micros_end - micros_start, result ? "success" : "failure");
            Serial_flush();
            return result;
        }
        bool save_scene(int selected_scene_number, bool debug = false) {
            //this->debug = true;
            if (debug) { Serial.printf(F("save for selected_scene_number %i\n"), selected_scene_number); Serial_flush(); }
            bool result = storage::save_scene(current_project_number, selected_scene_number, &storage::current_state, debug);
            if (result) {
                scene_slot_has_file[selected_scene_number] = true;
                loaded_scene_number = selected_scene_number;
            }
            return result;
        }

        //////// loops/recordings
        #ifdef ENABLE_LOOPER
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
        #endif

        void select_next_scene() {
            selected_scene_number++;
            if (selected_scene_number>=NUM_SCENE_SLOTS_PER_PROJECT)
                selected_scene_number = 0;
        }
        void select_previous_scene() {
            selected_scene_number--;
            if (selected_scene_number < 0) 
                selected_scene_number = NUM_SCENE_SLOTS_PER_PROJECT-1;
        }
        void load_next_scene() {
            loaded_scene_number++;
            if (loaded_scene_number>=NUM_SCENE_SLOTS_PER_PROJECT)
                loaded_scene_number = 0;
            load_scene(loaded_scene_number);
        }
        void load_previous_scene() {
            loaded_scene_number--;
            if (loaded_scene_number < 0)
                loaded_scene_number = NUM_SCENE_SLOTS_PER_PROJECT-1;
            load_scene(loaded_scene_number);
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
                Serial.printf(F("load_loop(): load for selected_loop_number project %i / loop %i\n"), current_project_number, selected_loop_number);
                //bool result = storage::load_scene(selected_loop_number, &storage::current_state);
                bool result = track->load_loop(current_project_number, selected_loop_number);
                if (result)
                    loaded_loop_number = selected_loop_number;
                return result;
            }
            bool save_loop(int selected_loop_number, MIDITrack *track) {
                Serial.printf(F("save for selected_loop_number project %i / loop %i\n"), current_project_number, selected_loop_number);
                //bool result = storage::save_scene(selected_loop_number, &storage::current_state);
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
        bool auto_advance_scene = false;
        void on_phrase(int phrase) {
            int slot = phrase % NUM_SCENE_SLOTS_PER_PROJECT;
            Debug_printf(F("Project#on_phrase(%i) called (slot %i)...\n"), phrase, slot);
            if (auto_advance_scene) {
                this->selected_scene_number = slot % NUM_SCENE_SLOTS_PER_PROJECT;
                //Serial.printf("on_phrase loading sequence_number %i\n", selected_scene_number);
                this->load_scene(this->selected_scene_number);
                //Serial.println("done!");
            }
            #ifdef ENABLE_LOOPER
                if (auto_advance_looper) {
                    this->selected_loop_number = slot % NUM_LOOP_SLOTS_PER_PROJECT;
                    Serial.printf("on_phrase loading loop_number %i\n", selected_loop_number);
                    this->load_loop(this->selected_loop_number);
                    //Serial.println("done!");
                }
            #endif
            Debug_printf(F("Project#on_phrase(%i) finished (slot %i)!\n"), phrase, slot);
        }
        bool is_auto_advance_scene() {
            return this->auto_advance_scene;
        }
        void set_auto_advance_scene(bool auto_advance_scene) {
            this->auto_advance_scene = auto_advance_scene;
        }


        bool save_project_settings() {
            return this->save_project_settings(current_project_number);
        }
        bool save_project_settings(int save_to_project_number) {
            #ifdef ENABLE_SD
            
            make_project_folders(save_to_project_number);

            char filename[MAX_FILEPATH] = "";
            snprintf(filename, MAX_FILEPATH, FILEPATH_PROJECT_SETTINGS_FORMAT, save_to_project_number);
            Serial.printf(F("save_project_settings(%i) writing to `%s`\n"), save_to_project_number, filename);
            if (SD.exists(filename)) {
                Serial.printf(F("%s exists, deleting first!\n"), filename);
                SD.remove(filename);
            }

            // Save entire tree, scoped to SL_SCOPE_PROJECT | SL_SCOPE_ROUTING.
            // SL_SCOPE_ROUTING is included so midi_matrix connection lines are saved;
            // scale/project settings use SL_SCOPE_PROJECT.

            uint32_t micros_start = micros();
            if (!sl_save_to_file(save_tree, filename, (sl_scope_t)(SL_SCOPE_PROJECT | SL_SCOPE_ROUTING))) {
                Serial.printf(F("Error saving project settings to %s\n"), filename);
                return false;
            }
            Serial.printf(F("Saved project settings in %lu microseconds.\n"), micros() - micros_start);

            update_project_filename(filename);
            #endif
            return true;
        }

        bool load_project_settings(int project_number) {
            #ifdef ENABLE_SD

            midi_matrix_manager->reset_matrix();  // disconnect all sources and targets before loading new routing settings

            char filename[MAX_FILEPATH] = "";
            snprintf(filename, MAX_FILEPATH, FILEPATH_PROJECT_SETTINGS_FORMAT, project_number);
            Serial.printf(F("load_project_settings(%i) opening %s\n"), project_number, filename);

            uint32_t micros_start = micros();
            sl_load_from_file(filename, (sl_scope_t)(SL_SCOPE_PROJECT | SL_SCOPE_ROUTING));
            Serial.printf(F("Loaded project settings in %u microseconds.\n"), micros() - micros_start);

            update_project_filename(filename);
            #endif

            return true;
        }
};

extern Project *project;

// for use by the Menu
void save_project_settings();