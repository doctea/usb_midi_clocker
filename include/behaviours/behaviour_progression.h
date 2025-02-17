#pragma once

#include <Arduino.h>
#include "Config.h"

#ifdef ENABLE_PROGRESSION

#include "bpm.h"

#include "behaviours/behaviour_base.h"

#include "sequencer/Euclidian.h"
#include "outputs/output_processor.h"
#include "outputs/output.h"

#include "midi/midi_mapper_matrix_manager.h"

#include "behaviour_apcmini.h"  // so that we can use apcmini for UI

#include "mymenu/menuitems_scale.h"
#include "mymenu/menuitems_notedisplay.h"

#include "chord_player.h"

#include "behaviours/behaviour_cvoutput.h"

extern MIDIMatrixManager *midi_matrix_manager;

#define NUM_SONG_SECTIONS 4

class VirtualBehaviour_Progression : virtual public VirtualBehaviourBase {
    public:

    struct song_section_t {
        chord_identity_t grid[8];
        int repeats = 1;    // number of repeats until moving to next section
        
        virtual void add_section_add_lines(LinkedList<String> *lines) {
            lines->add(String("repeats=")+String(repeats));
            for (int i = 0 ; i < 8 ; i++) {
                lines->add(String("grid_")+String(i)+String("_degree=")+String(grid[i].degree));
                lines->add(String("grid_")+String(i)+String("_type=")+String(grid[i].type));
                lines->add(String("grid_")+String(i)+String("_inversion=")+String(grid[i].inversion));
            }
        };
        virtual bool parse_section_line(String key, String value) {
            if (key.startsWith("grid_")) {
                int8_t grid_index = key.substring(5,6).toInt();
                if (grid_index>=0 && grid_index<8) {
                    if (key.endsWith("_degree")) {
                        grid[grid_index].degree = value.toInt();
                    } else if (key.endsWith("_type")) {
                        grid[grid_index].type = (CHORD::Type)value.toInt();
                    } else if (key.endsWith("_inversion")) {
                        grid[grid_index].inversion = value.toInt();
                    }
                }
                return true;
            } else if (key.equals("repeats")) {
                repeats = value.toInt();
                return true;
            }
            return false;
        };
    };

    song_section_t song_sections[NUM_SONG_SECTIONS];

    enum MODE {
        DEGREE,
        QUALITY,
        INVERSION,
        NUM_MODES
    };

    MODE current_mode = DEGREE;
    chord_identity_t current_chord;
    int8_t current_section = 0;
    int8_t current_section_plays = 0;

    bool advance_progression = true;

    ChordPlayer *chord_player = new ChordPlayer(
        [=] (int8_t channel, int8_t note, int8_t velocity) -> void {
            this->sendNoteOn(note, velocity, channel);
        },
        [=] (int8_t channel, int8_t note, int8_t velocity) -> void {
            this->sendNoteOff(note, velocity, channel);
        }
    );

    VirtualBehaviour_Progression() : DeviceBehaviourUltimateBase() {
        //memset(grid, 0, 64);
        //this->chord_player->debug = true;
        song_sections[0].grid[0].degree = 1;
        song_sections[0].grid[1].degree = 2;
        song_sections[0].grid[2].degree = 5;
        song_sections[0].grid[3].degree = 4;
        song_sections[0].grid[4].degree = 3;
        song_sections[0].grid[5].degree = 2;
        song_sections[0].grid[6].degree = 6;
        song_sections[0].grid[7].degree = 3;
    }

    virtual const char *get_label() override {
      return (const char*)"Progression";
    }

    virtual int getType() override {
      return BehaviourType::virt;
    }

    //virtual void on_tick(uint32_t ticks) override {
    //  //this->process_clocks(ticks);
    //};

    virtual void actualSendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
        midi_matrix_manager->processNoteOn(this->source_id, note, velocity, channel);
    }
    virtual void actualSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        midi_matrix_manager->processNoteOff(this->source_id, note, velocity, channel);
    }

    virtual LinkedList<MenuItem*> *make_menu_items() override {
        LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();

        //this->sequencer->make_menu_items(menu, true);
        //this->output_processor->create_menu_items(true);

        // todo: this was cribbed from menu.cpp setup_menu_midi() -- can probably re-use the same controls here to save some memory!
        LambdaScaleMenuItemBar *global_quantise_bar = new LambdaScaleMenuItemBar(
            "Global Scale", 
            [=](SCALE scale) -> void { midi_matrix_manager->set_global_scale_type(scale); }, 
            [=]() -> SCALE { return midi_matrix_manager->get_global_scale_type(); },
            [=](int8_t scale_root) -> void { midi_matrix_manager->set_global_scale_root(scale_root); },
            [=]() -> int8_t { return midi_matrix_manager->get_global_scale_root(); },
            false, true, true
        );
        global_quantise_bar->add(new LambdaToggleControl("Quantise",
            [=](bool v) -> void { midi_matrix_manager->set_global_quantise_on(v); },
            [=]() -> bool { return midi_matrix_manager->is_global_quantise_on(); }
        ));
        menuitems->add(global_quantise_bar);

        LambdaChordSubMenuItemBar *global_chord_bar = new LambdaChordSubMenuItemBar(
            "Global Chord", 
            [=](int8_t degree) -> void { midi_matrix_manager->set_global_chord_degree(degree); },
            [=]() -> int8_t { return midi_matrix_manager->get_global_chord_degree(); },
            [=](CHORD::Type chord_type) -> void { midi_matrix_manager->set_global_chord_type(chord_type); }, 
            [=]() -> CHORD::Type { return midi_matrix_manager->get_global_chord_type(); },
            [=](int8_t inversion) -> void { midi_matrix_manager->set_global_chord_inversion(inversion); },
            [=]() -> int8_t { return midi_matrix_manager->get_global_chord_inversion(); },
            false, true, true
        );
        global_chord_bar->add(new LambdaToggleControl("Quantise",
            [=](bool v) -> void { midi_matrix_manager->set_global_quantise_chord_on(v); },
            [=]() -> bool { return midi_matrix_manager->is_global_quantise_chord_on(); }
        ));
        menuitems->add(global_chord_bar);

        //chord_player->make_menu_items(menuitems);

        SubMenuItemBar *bar = new SubMenuItemBar("Chord controller", true, true);
        
        /*menuitems->add(new LambdaScaleMenuItemBar(
            "Chord controller", 
            [=] (SCALE s) -> void { chord_player->set_scale(s); },
            [=] (void) -> SCALE { return chord_player->get_scale(); },
            [=] (int8_t) -> void { chord_player_}
        ));*/

        bar->add(new LambdaNumberControl<int8_t>(
            "Degree", 
            [=] (int8_t degree) -> void {
                this->set_degree(degree);
            },
            [=] (void) -> int8_t { return this->current_chord.degree; },
            nullptr,
            0, 
            7, 
            true, 
            true
        ));

        bar->add(new LambdaToggleControl("Advance progression", 
            [=] (bool v) -> void { this->advance_progression = v; },
            [=] (void) -> bool { return this->advance_progression; }
        ));

        menuitems->add(bar);

        menuitems->add(new NoteDisplay("Progression notes", &this->note_tracker));
        menuitems->add(new NoteHarmonyDisplay(
            (const char*)"Progression harmony", 
            &midi_matrix_manager->global_scale_type, 
            &midi_matrix_manager->global_scale_root, 
            &this->note_tracker,
            &midi_matrix_manager->global_quantise_on
        ));

        /*
        menuitems->add(new NoteDisplay("CV Output 1 notes", &behaviour_cvoutput_1->note_tracker));
        menuitems->add(new NoteHarmonyDisplay(
            (const char*)"CV Output 1 harmony", 
            &midi_matrix_manager->global_scale_type, 
            &midi_matrix_manager->global_scale_root, 
            &behaviour_cvoutput_1->note_tracker,
            &midi_matrix_manager->global_quantise_on
        ));
        */
        
        SubMenuItemBar *section_bar = new SubMenuItemBar("Section", true, true);
        section_bar->add(new LambdaNumberControl<int8_t>(
            "Section", 
            [=] (int8_t section) -> void {
                change_section(section);
            },
            [=] () -> int8_t { return this->current_section; },
            nullptr,
            0, 
            NUM_SONG_SECTIONS-1, 
            true, 
            false
        ));
        section_bar->add(new LambdaNumberControl<int8_t>(
            "Max repeats", 
            [=] (int8_t repeats) -> void {
                this->song_sections[current_section].repeats = repeats;
            },
            [=] () -> int8_t { return this->song_sections[current_section].repeats; },
            nullptr,
            0,
            8,
            true,
            true
        ));
        section_bar->add(new NumberControl<int8_t>("Plays", &this->current_section_plays, 0, 8, true, true));
        menuitems->add(section_bar);

        SubMenuItemBar *save_bar = new SubMenuItemBar("Section", false, true);
        save_bar->add(new LambdaActionConfirmItem(
            "Load", 
            [=] () -> void {
                this->load_section(-1, current_section);
            }
        ));
        save_bar->add(new LambdaActionConfirmItem(
            "Save", 
            [=] () -> void {
                this->save_section(-1, current_section);
            }
        ));
        menuitems->add(save_bar);

        return menuitems;
    }

    // degrees should be 1-7; 0 implies no chord, -1 implies 'use global'?
    void set_degree(int8_t degree) {
        this->current_chord.degree = degree;
        if (degree>=-1 && degree<PITCHES_PER_SCALE+1) {
            midi_matrix_manager->set_global_chord_degree(degree);
        } else {
            this->chord_player->stop_chord();
            //Serial_printf("invalid degree %i\n", degree);
            //this->chord_player->trigger_off_for_pitch_because_length(-1);
        }
    }

    virtual void setup_saveable_parameters() override {
        DeviceBehaviourUltimateBase::setup_saveable_parameters();

        this->saveable_parameters->add(new LSaveableParameter<bool>(
            "Advance progression",
            "Progression", 
            &advance_progression
        ));

        //this->sequencer->setup_saveable_parameters();

        // todo: better way of 'nesting' a sequencer/child object's saveableparameters within a host object's
        /*
        for(unsigned int i = 0 ; i < sequencer->saveable_parameters->size() ; i++) {
            this->saveable_parameters->add(sequencer->saveable_parameters->get(i));
        }
        */
    }

    // untested, but this should fire the tick before a beat happens; use this to change chords outside of a bar change..
    /*virtual void on_end_beat(int beat_number) override {
        beat_number = BPM_CURRENT_BEAT + 1;
        beat_number %= 4;
        int bar_number = BPM_CURRENT_BAR;
        if (beat_number==BEATS_PER_BAR/2) {
            int found_bars = 0;
            for (int i = 8 ; i > 0 ; i--) {
                int d = 8 - i;
                if (grid[bar_number][i]>0) {
                    Serial.printf(
                        "on_end_beat %2i: Progression starting beat %i: found active degree %i at grid column %i\n", 
                        BPM_CURRENT_BEAT, 
                        beat_number, 
                        d,
                        beat_number
                    );
                    this->set_degree(d+1);
                }
            }
        }
        //}
    }*/

    /*int8_t get_degree_from_grid(int8_t bar_number) {
        //int8_t retval = -1;
        if (debug) Serial_printf("get_degree_from_grid passed bar_number=%2i\n", bar_number);
        return grid[bar_number].degree;
    }*/

    virtual int8_t get_cell_colour_for(uint8_t x, uint8_t y) {
        if (/*y==0 ||*/ x>=8 || y>=8) return 0;
        if (current_mode==MODE::DEGREE) {
            if (y==0) { // top row
                if (x==BPM_CURRENT_BAR%8) {
                    // indicate the current bar
                    return APCMINI_YELLOW_BLINK;
                } else {
                    return APCMINI_OFF;
                }
            }
            return song_sections[current_section].grid[x].degree == (8-y);
        } else if (current_mode==MODE::QUALITY) {
            return song_sections[current_section].grid[x].type == (7-y);
        } else if (current_mode==MODE::INVERSION) {
            return song_sections[current_section].grid[x].inversion == (7-y);
        }
        return 0;
    }

    void dump_grid() {
        if (!Serial) return;

        Serial_printf("dump_grid:");
        //for (int y = 0 ; y < 8 ; y++) {
            //Serial.printf("Grid row %i: [ ", y);
            for (int x = 0 ; x < 8 ; x++) {
                //Serial.printf("%i ", grid[x].chord_degree);
                //Serial.printf("%i ", get_cell_colour_for(x, y));
                Serial_printf("{ %i %i=%5s %i }, ", song_sections[current_section].grid[x].degree, song_sections[current_section].grid[x].type, chords[song_sections[current_section].grid[x].type].label, song_sections[current_section].grid[x].inversion);
            }
            Serial_println("]");
        //}
        Serial_println("------");
    }

    void set_current_chord(chord_identity_t chord, bool requantise_immediately = true) {
        this->current_chord = chord;
        if (chord.degree>0)
            midi_matrix_manager->set_global_chord(current_chord, requantise_immediately);
        else
            this->chord_player->stop_chord();
    }

    virtual void on_end_phrase(uint32_t phrase_number) override {
        if (debug) Serial_printf("on_end_phrase %2i\n", phrase_number);
        //if (debug) dump_grid();
        // todo: this should only move the section on every 2 phrases, not every phrase
        current_section_plays++;
        if (current_section_plays >= song_sections[current_section].repeats) {
            Serial.printf("reached %i of %i plays; changing section from %i to %i\n", current_section_plays, song_sections[current_section].repeats, current_section, current_section+1);
            current_section_plays = 0;
            // todo: crashes?!
            change_section(current_section+1);

            this->chord_player->stop_chord();
            // trying to start chords here seems to cause intermittent crashes when changing sections..?
            //if (this->current_chord.is_valid_chord())
            //    this->chord_player->play_chord(song_sections[current_section].grid[BPM_CURRENT_BAR]);
        }
    }

    virtual void on_end_bar(int bar_number) override {
        this->chord_player->stop_chord();
    }

    virtual void on_bar(int bar_number) override {
        if (advance_progression) {
            bar_number = BPM_CURRENT_BAR;
            bar_number %= 8;
            //bar_number %= BARS_PER_PHRASE;// * 2;

            if (debug) Serial_printf("=======\non_end_bar %2i (going into bar number %i)\n", BPM_CURRENT_BAR % 8, bar_number);
            if (debug) dump_grid();

            this->set_current_chord(song_sections[current_section].grid[bar_number]);

            // send the chord for the current degree
            if (this->current_chord.is_valid_chord())
                this->chord_player->play_chord(this->current_chord);

            /*int8_t degree = this->get_degree_from_grid(bar_number);
            if (degree>0) {
                if (debug) Serial_printf("on_end_bar %2i (going into %i): got degree %i\n", BPM_CURRENT_BAR % 8, bar_number, degree);
                //this->set_degree(degree);
                this->set_current_chord(grid[bar_number]);
            } else {
                //this->set_degree(-1);
                this->set_current_chord(grid[bar_number]);
                if (debug) Serial_printf("on_end_bar %2i (going into %i): no degree found\n", BPM_CURRENT_BAR % 8, bar_number);
            }*/
            if (debug) Serial_printf("=======\n");
        }
    }

    virtual bool apcmini_press(int inNumber, bool shifted) {
        if (inNumber>=0 && inNumber < NUM_SEQUENCES * APCMINI_DISPLAY_WIDTH) {
            //byte row = (NUM_SEQUENCES-1) - (inNumber / APCMINI_DISPLAY_WIDTH);
            byte row = inNumber / APCMINI_DISPLAY_WIDTH;
            byte col = inNumber - (row*APCMINI_DISPLAY_WIDTH);

            Serial_printf("apcmini_press(%i, %i) => row=%i, col=%i\n", inNumber, shifted, row, col);
            Serial.flush();

            if (current_mode==MODE::DEGREE) {
                int new_degree = row + 1;
                if (new_degree>0 && new_degree<=PITCHES_PER_SCALE) {
                    song_sections[current_section].grid[col].degree = new_degree;
                    return true;
                }
            } else if (current_mode==MODE::QUALITY) {
                int new_quality = row;
                if (new_quality>=0 && new_quality<CHORD::NONE) {
                    song_sections[current_section].grid[col].type = (CHORD::Type)new_quality;
                    return true;
                }
            } else if (current_mode==MODE::INVERSION) {
                int new_inversion = row;
                if (new_inversion>=0 && new_inversion<=MAX_INVERSIONS) {
                    song_sections[current_section].grid[col].inversion = new_inversion;
                    return true;
                }
            }
        } else if (inNumber>=APCMINI_BUTTON_CLIP_STOP && inNumber < APCMINI_BUTTON_CLIP_STOP + VirtualBehaviour_Progression::MODE::NUM_MODES) {
            Serial_printf("apcmini_press(%i, %i) => mode=%i\n", inNumber, shifted, inNumber - APCMINI_BUTTON_CLIP_STOP);
            Serial.flush();
            this->current_mode = (VirtualBehaviour_Progression::MODE)(inNumber - APCMINI_BUTTON_CLIP_STOP);
            return true;
        }

        return false;
    }
    virtual bool apcmini_release(int inNumber, bool shifted) {
        //byte row = (NUM_SEQUENCES-1) - (inNumber / APCMINI_DISPLAY_WIDTH);
        //byte col = inNumber - (((8-1)-row)*APCMINI_DISPLAY_WIDTH);
        return false;
    }

    virtual void requantise_all_notes() override {
        bool initial_global_quantise_on = midi_matrix_manager->global_quantise_on;
        bool initial_global_quantise_chord_on = midi_matrix_manager->global_quantise_chord_on;   
        midi_matrix_manager->global_quantise_on = false;
        midi_matrix_manager->global_quantise_chord_on = false;
        this->chord_player->stop_chord();
        midi_matrix_manager->global_quantise_on = initial_global_quantise_on;
        midi_matrix_manager->global_quantise_chord_on = initial_global_quantise_chord_on;

        this->chord_player->play_chord(this->current_chord);
    }

    virtual void change_section(int section_number) {
        Serial.printf("change_section(%i)\n", section_number); Serial.flush();
        if (section_number==NUM_SONG_SECTIONS) section_number = 0;

        if (section_number>=0 && section_number<NUM_SONG_SECTIONS) {
            if (current_section!=section_number) {
                this->current_section_plays = 0;
            }

            current_section = section_number;
            Serial.printf("changing to section %i\n", current_section);

            Serial.printf("stopping chord, etc\n"); Serial.flush();
            /*this->chord_player->stop_chord();
            if (this->current_chord.is_valid_chord())
                this->chord_player->play_chord(song_sections[current_section].grid[BPM_CURRENT_BAR]);*/
            Serial.printf("done\n"); Serial.flush();
        }
    }

    virtual bool save_section(int section_number = -1, int project_number = -1) {
        if (section_number<0) section_number = current_section;
        if (project_number<0) project_number = project->current_project_number;

        LinkedList<String> section_lines = LinkedList<String>();
        section_lines.add(String("current_section=")+String(section_number));
        if (section_number>=0 && section_number<NUM_SONG_SECTIONS) {
            song_sections[section_number].add_section_add_lines(&section_lines);
        }
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            File myFile;

            make_project_folders(project_number);

            char filename[MAX_FILEPATH] = "";
            snprintf(filename, MAX_FILEPATH, FILEPATH_SECTION_FORMAT, project_number, section_number);
            if (debug) Serial.printf(F("save_pattern(%i, %i) writing to %s\n"), project_number, section_number, filename);
            if (SD.exists(filename)) {
              //Serial.printf(F("%s exists, deleting first\n"), filename); Serial.flush();
              SD.remove(filename);
              //Serial.println("deleted"); Serial.flush();
            }

            myFile = SD.open(filename, FILE_WRITE_BEGIN | (uint8_t)O_TRUNC);
            if (!myFile) {    
              if (debug) Serial.printf(F("Error: couldn't open %s for writing\n"), filename);
              //if (irqs_enabled) __enable_irq();
              return false;
            }
            if (debug) Serial.println("Starting data write.."); Serial_flush();

            myFile.println(F("; begin section"));
            for (uint_fast16_t i = 0 ; i < section_lines.size() ; i++) {
                myFile.println(section_lines.get(i));
            }
            myFile.println(F("; end section"));
            myFile.close();

            messages_log_add(String("Saved to project : section ") + String(project_number) + " : " + String(section_number));
        }
        return true;
    }

    virtual bool load_section(int section_number = -1, int project_number = -1) {
        if (section_number<0) section_number = current_section;
        if (project_number<0) project_number = project->current_project_number;

        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            File myFile;

            char filename[MAX_FILEPATH] = "";
            snprintf(filename, MAX_FILEPATH, FILEPATH_SECTION_FORMAT, project_number, section_number);
            if (debug) Serial.printf(F("load_section(%i, %i) opening %s\n"), project_number, section_number, filename);
            myFile = SD.open(filename, FILE_READ);
            if (!myFile) {
                if (debug) Serial.printf(F("Error: Couldn't open %s for reading!\n"), filename);
                return false;
            }
            myFile.setTimeout(0);

            String line;
            while (line = myFile.readStringUntil('\n')) {
                if (line.startsWith(";")) continue;
                if (debug) Serial.printf("load_section: parsing line %s\n", line.c_str());
                song_sections[section_number].parse_section_line(line.substring(0, line.indexOf('=')), line.substring(line.indexOf('=')+1));
            }
            myFile.close();
        }

        return true;
    }

};

extern VirtualBehaviour_Progression *behaviour_progression;

#endif