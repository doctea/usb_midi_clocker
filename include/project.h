#pragma once

#include "Config.h"

#include "storage.h"
#ifdef ENABLE_LOOPER
#include "midi/midi_looper.h"
#endif

#include "midi/midi_mapper_matrix_manager.h"
#include "behaviours/behaviour_manager.h"

#include "mymenu/menu_fileviewers.h"

#define NUM_PATTERN_SLOTS_PER_PROJECT   8
#define NUM_LOOP_SLOTS_PER_PROJECT      8

using namespace storage;

#ifdef ENABLE_LOOPER
    extern MIDITrack midi_loop_track;
#endif
#ifdef ENABLE_DRUM_LOOPER
    extern MIDITrack drums_loop_track;
#endif

extern volatile bool global_load_lock;

class Project : public SHStorage<0, 8> {
    bool pattern_slot_has_file[NUM_PATTERN_SLOTS_PER_PROJECT];
    #ifdef ENABLE_LOOPER
    bool loop_slot_has_file[NUM_LOOP_SLOTS_PER_PROJECT];
    #endif

    bool debug = false;

    #ifdef ENABLE_LOOPER
    MIDITrack *temp_loop = new MIDITrack();
    #endif

    void initialise_pattern_slots() {
        #ifdef ENABLE_SD
            Serial.println(F("initialise_pattern_slots starting.."));
            for (unsigned int i = 0 ; i < NUM_PATTERN_SLOTS_PER_PROJECT ; i++) {
                pattern_slot_has_file[i] = SD.exists(storage::get_pattern_filename(this->current_project_number, i));
                Serial_printf(F("\tpattern_slot_has_file[i] = %i for %s\n"), pattern_slot_has_file[i], get_pattern_filename(this->current_project_number, i));
            }
            Serial_println(F("initialise_pattern_slots finished"));
        #else
            for (unsigned int i = 0 ; i < NUM_PATTERN_SLOTS_PER_PROJECT ; i++) {
                pattern_slot_has_file[i] = false;
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
        int current_project_number = 0;

        int selected_pattern_number = 0;
        int loaded_pattern_number = -1;

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
            //initialise_pattern_slots();
        }

        // ---- saveloadlib integration ----
        // Registers project-scope scalar settings so they are saved/loaded when
        // sl_save_to_linkedlist / sl_load_from_file is called with SL_SCOPE_PROJECT.
        // Called automatically by sl_setup_all() via SettingsRoot::setup_saveable_settings().
        virtual void setup_saveable_settings() override {
            this->set_path_segment("project");
            register_setting(new LSaveableSetting<int>(
                "id", "Project", nullptr,
                [this](int v) { /* id on load is informational; project number set by caller */ },
                [this]() -> int { return this->current_project_number; }
            ), false, SL_SCOPE_PROJECT);
            register_setting(new LSaveableSetting<bool>(
                "auto_advance_pattern", "Project", &this->auto_advance_pattern
            ), false, SL_SCOPE_PROJECT);
            #ifdef ENABLE_LOOPER
            register_setting(new LSaveableSetting<bool>(
                "auto_advance_looper", "Project", &this->auto_advance_looper
            ), false, SL_SCOPE_PROJECT);
            #endif
        }

        FLASHMEM void setup_project() {
            setProjectNumber(this->current_project_number);

            initialise_pattern_slots();
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
                this->initialise_pattern_slots();
                this->notify_behaviours_for_project_change(number);
            //}
        }
        int getProjectNumber() {
            return this->current_project_number;
        }

        ////////////// clocks / sequences
        bool select_pattern_number(int sn) {
            Serial_printf(F("select_pattern_number %i\n"), sn); Serial_flush();
            selected_pattern_number = sn % NUM_PATTERN_SLOTS_PER_PROJECT;
            return sn == selected_pattern_number;
        }

        bool is_selected_pattern_number_empty(int sn) {
            return !pattern_slot_has_file[sn];
        }

        int get_selected_pattern_number() {
            return this->selected_pattern_number;
        }
        int get_loaded_pattern_number() {
            return this->loaded_pattern_number;
        }
        int get_max_pattern_slots() {
            return NUM_PATTERN_SLOTS_PER_PROJECT;
        }

        // load and save sequences / clock settings etc
        bool load_selected_pattern() {
            return load_pattern(selected_pattern_number);
        }
        bool save_selected_pattern() {
            return save_pattern(selected_pattern_number);
        }
        bool load_pattern(bool debug = false) {
            return load_pattern(selected_pattern_number, debug);
        }
        bool save_pattern() {
            return save_pattern(selected_pattern_number);
        }
        bool load_specific_pattern(int selected_pattern_number) {
            return this->load_pattern(selected_pattern_number);
        }

        bool load_pattern(int selected_pattern_number, bool debug = false) {
            if (debug) { Serial.printf(F("load for selected_pattern_number %i\n"), selected_pattern_number); Serial_flush(); }
            uint32_t micros_start = micros();
            bool result = storage::load_pattern(current_project_number, selected_pattern_number, &storage::current_state, debug);
            if (result)
                loaded_pattern_number = selected_pattern_number;
            uint32_t micros_end = micros();
            Serial_printf(F("load_pattern took %lu microseconds - returning\n"), micros_end - micros_start);
            Serial_flush();
            return result;
        }
        bool save_pattern(int selected_pattern_number, bool debug = false) {
            //this->debug = true;
            if (debug) { Serial.printf(F("save for selected_pattern_number %i\n"), selected_pattern_number); Serial_flush(); }
            bool result = storage::save_pattern(current_project_number, selected_pattern_number, &storage::current_state, debug);
            if (result) {
                pattern_slot_has_file[selected_pattern_number] = true;
                loaded_pattern_number = selected_pattern_number;
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

        void select_next_pattern() {
            selected_pattern_number++;
            if (selected_pattern_number>=NUM_PATTERN_SLOTS_PER_PROJECT)
                selected_pattern_number = 0;
        }
        void select_previous_pattern() {
            selected_pattern_number--;
            if (selected_pattern_number < 0) 
                selected_pattern_number = NUM_PATTERN_SLOTS_PER_PROJECT-1;
        }
        void load_next_pattern() {
            loaded_pattern_number++;
            if (loaded_pattern_number>=NUM_PATTERN_SLOTS_PER_PROJECT)
                loaded_pattern_number = 0;
            load_pattern(loaded_pattern_number);
        }
        void load_previous_pattern() {
            loaded_pattern_number--;
            if (loaded_pattern_number < 0)
                loaded_pattern_number = NUM_PATTERN_SLOTS_PER_PROJECT-1;
            load_pattern(loaded_pattern_number);
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
                //bool result = storage::load_pattern(selected_loop_number, &storage::current_state);
                bool result = track->load_loop(current_project_number, selected_loop_number);
                if (result)
                    loaded_loop_number = selected_loop_number;
                return result;
            }
            bool save_loop(int selected_loop_number, MIDITrack *track) {
                Serial.printf(F("save for selected_loop_number project %i / loop %i\n"), current_project_number, selected_loop_number);
                //bool result = storage::save_pattern(selected_loop_number, &storage::current_state);
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
        bool auto_advance_pattern = false;
        void on_phrase(int phrase) {
            int slot = phrase % NUM_PATTERN_SLOTS_PER_PROJECT;
            Debug_printf(F("Project#on_phrase(%i) called (slot %i)...\n"), phrase, slot);
            if (auto_advance_pattern) {
                this->selected_pattern_number = slot % NUM_PATTERN_SLOTS_PER_PROJECT;
                //Serial.printf("on_phrase loading sequence_number %i\n", selected_pattern_number);
                this->load_pattern(this->selected_pattern_number);
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
        bool is_auto_advance_pattern() {
            return this->auto_advance_pattern;
        }
        void set_auto_advance_pattern(bool auto_advance_pattern) {
            this->auto_advance_pattern = auto_advance_pattern;
        }


        bool save_project_settings() {
            return this->save_project_settings(current_project_number);
        }
        bool save_project_settings(int save_to_project_number) {
            #ifdef ENABLE_SD
            File myFile;

            char filename[MAX_FILEPATH] = "";
            snprintf(filename, MAX_FILEPATH, FILEPATH_PROJECT_SETTINGS_FORMAT, save_to_project_number);
            Serial.printf(F("save_project_settings(%i) writing to %s\n"), save_to_project_number, filename);
            if (SD.exists(filename)) {
                Serial.printf(F("%s exists, deleting first\n"), filename);
                SD.remove(filename);
            }
            myFile = SD.open(filename, FILE_WRITE_BEGIN | (uint8_t)O_TRUNC);
            if (!myFile) {
                Serial.printf(F("Error: couldn't open %s for writing\n"), filename);
                return false;
            }

            myFile.println(F("; begin project"));

            // Save Project-scope scalar settings via saveloadlib ("project~key=value" lines)
            LinkedList<String> project_lines = LinkedList<String>();
            sl_save_to_linkedlist(this, project_lines, SL_SCOPE_PROJECT);
            for (unsigned int i = 0 ; i < project_lines.size() ; i++) {
                myFile.println(project_lines.get(i));
            }

            // Save behaviour project-scope settings ("BehaviourLabel~key=value" lines)
            LinkedList<String> behaviour_lines = LinkedList<String>();
            behaviour_manager->save_project_add_lines(&behaviour_lines);
            for (unsigned int i = 0 ; i < behaviour_lines.size() ; i++) {
                myFile.println(behaviour_lines.get(i));
            }

            // Save MIDI matrix mappings via saveloadlib tree
            // Connections are scoped SL_SCOPE_ROUTING; scale settings are SL_SCOPE_PROJECT
            LinkedList<String> matrix_lines = LinkedList<String>();
            sl_save_to_linkedlist(midi_matrix_manager, matrix_lines, (sl_scope_t)(SL_SCOPE_ROUTING | SL_SCOPE_PROJECT));
            for (unsigned int i = 0 ; i < matrix_lines.size() ; i++) {
                myFile.println(matrix_lines.get(i));
            }

            myFile.println(F("; end project"));
            myFile.close();
            Serial.println(F("Finished saving."));

            update_project_filename(filename);
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

        void load_project_parse_line(String line, bool debug = false) {
            if (line.charAt(0)==';')
                return;  // skip comment lines

            line.replace('\n', "");
            line.replace('\r', "");

            int eq = line.indexOf('=');
            if (eq < 0) return;
            String left  = line.substring(0, eq);
            String value = line.substring(eq + 1);

            int tilde = left.indexOf('~');
            if (tilde >= 0) {
                // Tilde-delimited format: "segment~key=value"
                String segment = left.substring(0, tilde);
                String rest    = left.substring(tilde + 1);

                if (segment.equals(F("project"))) {
                    // Route to this Project's saveloadlib settings tree
                    static char restbuf[SL_MAX_LINE];
                    static char valbuf[SL_MAX_LINE];
                    rest.toCharArray(restbuf, sizeof(restbuf));
                    value.toCharArray(valbuf, sizeof(valbuf));
                    static char* segs[16];
                    int cnt = sl_tokenise_inplace(restbuf, segs, 16);
                    if (cnt > 0) load_line(segs, cnt, valbuf, SL_SCOPE_PROJECT);
                    return;
                } else if (segment.equals(F("midi_matrix"))) {
                    // Route to midi_matrix_manager's saveloadlib tree.
                    // SL_SCOPE_ROUTING is included only when isLoadMatrixMappings() is true;
                    // scale settings (SL_SCOPE_PROJECT) are always loaded.
                    sl_scope_t routing_scope = this->isLoadMatrixMappings()
                        ? (sl_scope_t)(SL_SCOPE_PROJECT | SL_SCOPE_ROUTING)
                        : SL_SCOPE_PROJECT;
                    static char restbuf[SL_MAX_LINE];
                    static char valbuf[SL_MAX_LINE];
                    rest.toCharArray(restbuf, sizeof(restbuf));
                    value.toCharArray(valbuf, sizeof(valbuf));
                    static char* segs[16];
                    int cnt = sl_tokenise_inplace(restbuf, segs, 16);
                    if (cnt > 0) midi_matrix_manager->load_line(segs, cnt, valbuf, routing_scope);
                    return;
                } else if (this->isLoadBehaviourOptions() && behaviour_manager->load_parse_line(line)) {
                    // "BehaviourLabel~key=value" handled by behaviour_manager
                    return;
                } else if (this->isLoadMatrixMappings() && midi_matrix_manager->load_parse_line(line)) {
                    // legacy fallback for any tilde-prefixed matrix lines in old files
                    return;
                }
                if (debug) Serial.printf(F("project: unknown tilde line '%s'\n"), line.c_str());
                messages_log_add(String("Unknown Project line '") + line + String("'"));
                return;
            }

            // Legacy flat format
            if (this->isLoadMatrixMappings() && line.startsWith(F("midi_output_map="))) {
                // pre-matrix legacy format
                if (debug) Serial.printf(F("----\nLoading midi_output_map line '%s'\n"), line.c_str());
                line = line.remove(0, String(F("midi_output_map=")).length());
                int split = line.indexOf('|');
                String source_label = line.substring(0, split);
                source_label.replace(F("_output"), F(""));
                String target_label = line.substring(split+1, line.length());
                midi_matrix_manager->connect(source_label.c_str(), target_label.c_str());
                return;
            } else if (this->isLoadMatrixMappings() && midi_matrix_manager->load_parse_line(line)) {
                return;
            } else if (this->isLoadBehaviourOptions() && behaviour_manager->load_parse_line(line)) {
                return;
            }
            if (debug) Serial.printf(F("project read line '%s', but didn't know how to process it\n"), line.c_str());
            messages_log_add(String("Unknown Project line '") + line + String("'"));
        }
};

extern Project *project;

// for use by the Menu
void save_project_settings();