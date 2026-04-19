#pragma once

#include <Arduino.h>
#include "Config.h"

#if defined(ENABLE_PROGRESSION) && defined(ENABLE_SCALES)

#include "bpm.h"

#include "behaviours/behaviour_base.h"

#include "sequencer/Euclidian.h"
#include "outputs/output_processor.h"
#include "outputs/output.h"

#include "midi/midi_mapper_matrix_manager.h"

#include "behaviour_apcmini.h"  // so that we can use apcmini for UI

#include "mymenu/menuitems_scale.h"
#include "mymenu/menuitems_notedisplay.h"
#include "mymenu/menuitems_progression.h"
#include "chord_player.h"

#include "behaviours/behaviour_cvoutput.h"

#include "arranger.h"

extern MIDIMatrixManager *midi_matrix_manager;

class VirtualBehaviour_Progression : virtual public VirtualBehaviourBase {
    public:
    source_id_t source_id_chord_octave = -1;
    source_id_t source_id_bass = -1;
    source_id_t source_id_topline = -1;

    uint8_t BASS_CHANNEL = 2,   TOPLINE_CHANNEL = 3;
    uint8_t bass_octave = 2,    topline_octave = 3, chord_octave = 5;

    // Proxy dirty-flag methods to arranger
    virtual bool has_song_changes_to_save() {
        return arranger->has_changes_to_save();
    }
    virtual void mark_song_save_done() {
        arranger->mark_save_done();
    }
    virtual void mark_song_as_modified() {
        arranger->mark_as_modified();
    }

    virtual bool transmits_midi_notes() { return true; }

    enum MODE {
        DEGREE,
        QUALITY,
        INVERSION,
        PLAYLIST,
        NUM_MODES
    };

    MODE current_mode = DEGREE;

    // Proxy advance flags to arranger for backwards compatibility
    // (menu.cpp and other code accesses these directly)
    bool& advance_progression_bar      = arranger->advance_bar;
    bool& advance_progression_playlist = arranger->advance_playlist;

    ChordPlayer *chord_player = new ChordPlayer(
        [=] (int8_t channel, int8_t note, int8_t velocity) -> void { this->sendNoteOn (note, velocity, channel); },
        [=] (int8_t channel, int8_t note, int8_t velocity) -> void { this->sendNoteOff(note, velocity, channel); },
        [=] (int8_t channel, int8_t note, int8_t velocity) -> void { this->sendNoteOn (note, velocity, BASS_CHANNEL); },
        [=] (int8_t channel, int8_t note, int8_t velocity) -> void { this->sendNoteOff(note, velocity, BASS_CHANNEL); },
        [=] (int8_t channel, int8_t note, int8_t velocity) -> void { this->sendNoteOn (note, velocity, TOPLINE_CHANNEL); },
        [=] (int8_t channel, int8_t note, int8_t velocity) -> void { this->sendNoteOff(note, velocity, TOPLINE_CHANNEL); }
    );

    VirtualBehaviour_Progression() : DeviceBehaviourUltimateBase() {
        this->set_path_segment("progression");

        // Wire up arranger callbacks
        arranger->on_chord_changed([this](const chord_identity_t& chord, bool requantise) {
            if (chord.degree > 0)
                conductor->set_chord_identity(chord, requantise);
            else
                this->chord_player->stop_chord();

            if (chord.is_valid_chord())
                this->chord_player->play_chord(chord);
        });
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
        if (channel==BASS_CHANNEL) {
            if (debug) Serial_printf("actualSendNoteOn to bass: channel %i, note %i, velocity %i (source_id_bass=%i)\n", channel, note, velocity, this->source_id_bass);
            note += 12 * bass_octave;
            if(is_valid_note(note)) {
                midi_matrix_manager->processNoteOn(this->source_id_bass, note, velocity); //, 1);
            }
        } else if (channel==TOPLINE_CHANNEL) {
            if (debug) Serial_printf("actualSendNoteOn to topline: channel %i, note %i, velocity %i (source_id_topline=%i)\n", channel, note, velocity, this->source_id_topline);
            note += 12 * topline_octave;
            if(is_valid_note(note)) {
                midi_matrix_manager->processNoteOn(this->source_id_topline, note, velocity); //, 1);
            }
        } else {
            midi_matrix_manager->processNoteOn(this->source_id, note, velocity); //, channel);
            // send the same chord sequence to the second source_id 5 octaves up
            note += 12 * chord_octave;
            if (is_valid_note(note)) {
                midi_matrix_manager->processNoteOn(this->source_id_chord_octave, note, velocity); //, channel);
            }
        }
    }
    virtual void actualSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        if (channel==BASS_CHANNEL) {
            note += 12 * bass_octave;
            if (debug) Serial_printf("actualSendNoteOff to bass: channel %i, note %i, velocity %i\n", channel, note, velocity);
            if(is_valid_note(note)) {
                midi_matrix_manager->processNoteOff(this->source_id_bass, note, velocity); //, 1);
            }
        } else if (channel==TOPLINE_CHANNEL) {
            if (debug) Serial_printf("actualSendNoteOff to topline: channel %i, note %i, velocity %i\n", channel, note, velocity);
            note += 12 * topline_octave;
            if(is_valid_note(note)) {
                midi_matrix_manager->processNoteOff(this->source_id_topline, note, velocity); //, 1);
            }
        } else {
            midi_matrix_manager->processNoteOff(this->source_id, note, velocity, channel);
            // send the same chord sequence to the second source_id 5 octaves up
            note += 12 * chord_octave;
            if (is_valid_note(note)) {
                midi_matrix_manager->processNoteOff(this->source_id_chord_octave, note, velocity); //, channel);
            }
        }
    }

    // degrees should be 1-7; 0 implies no chord, -1 implies 'use global'?
    void set_degree(int8_t degree) {
        arranger->current_chord.degree = degree;
        if (degree>=-1 && degree<PITCHES_PER_SCALE+1) {
            conductor->set_chord_degree(degree);
        } else {
            this->chord_player->stop_chord();
        }
    }

    virtual void setup_saveable_settings() override {
        DeviceBehaviourUltimateBase::setup_saveable_settings();
        register_setting(new LSaveableSetting<bool>("Advance progression",  "Progression", &arranger->advance_bar), SL_SCOPE_SCENE);
        register_setting(new LSaveableSetting<bool>("Advance playlist",      "Progression", &arranger->advance_playlist), SL_SCOPE_SCENE);
        register_setting(new LSaveableSetting<uint8_t>("Chord octave",   "Progression", &chord_octave), SL_SCOPE_SCENE);
        register_setting(new LSaveableSetting<uint8_t>("Bass octave",     "Progression", &bass_octave), SL_SCOPE_SCENE);
        register_setting(new LSaveableSetting<uint8_t>("Topline octave",  "Progression", &topline_octave), SL_SCOPE_SCENE);

        // Song structure: sections + playlist (saved per-project)
        for (int i = 0; i < NUM_SONG_SECTIONS; i++) {
            char lbl[16];
            snprintf(lbl, sizeof(lbl), "section_%i", i);
            register_setting(new SaveableSectionGridSetting(lbl, "Progression", &arranger->song_sections[i]), SL_SCOPE_PROJECT);
        }
        register_setting(new SaveablePlaylistSetting("playlist", "Progression", &arranger->playlist), SL_SCOPE_PROJECT);
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

    void dump_grid() {
        if (!Serial) return;

        Serial_printf("dump_grid:");
            for (int x = 0 ; x < CHORDS_PER_SECTION ; x++) {
                Serial_printf("{ %i %i=%5s %i }, ", arranger->song_sections[arranger->current_section].grid[x].degree, arranger->song_sections[arranger->current_section].grid[x].type, chords[arranger->song_sections[arranger->current_section].grid[x].type].label, arranger->song_sections[arranger->current_section].grid[x].inversion);
            }
            Serial_println("]");
        Serial_println("------");
    }

    void set_current_chord(chord_identity_t chord, bool requantise_immediately = true) {
        arranger->current_chord = chord;
        if (chord.degree>0)
            conductor->set_chord_identity(arranger->current_chord, requantise_immediately);
        else
            this->chord_player->stop_chord();
    }

    void move_next_playlist() {
        arranger->move_next_playlist();
    }
    void move_playlist(int8_t pos) {
        arranger->move_playlist(pos);
    }

    virtual void on_end_phrase(uint32_t phrase_number) override {
        if (debug) Serial_printf("on_end_phrase %2i\n", phrase_number);
        int8_t prev_section = arranger->current_section;
        arranger->on_end_phrase(phrase_number);
        if (arranger->current_section != prev_section) {
            this->chord_player->stop_chord();
        }
    }

    virtual void on_end_bar(int bar_number) override {
        this->chord_player->stop_chord();
    }

    virtual void on_restart() override {
        arranger->on_restart();
    }

    void move_bar(int new_bar) {
        arranger->move_bar(new_bar);
    }

    virtual void on_bar(int bar_number) override {
        if (debug) Serial.printf("on_bar %2i with current_bar %2i\n", bar_number, arranger->current_bar);
        arranger->on_bar(bar_number);

        if (debug) Serial_printf("=======\non_end_bar %2i (going into bar number %i)\n", BPM_CURRENT_BAR % 8, arranger->current_bar);
        if (debug) dump_grid();
        if (debug) Serial_printf("=======\n");
    }



    virtual int8_t get_cell_colour_for(uint8_t x, uint8_t y) {
        if (/*y==0 ||*/ x>=8 || y>=8) return 0;
        if (current_mode==MODE::DEGREE) {
            if (y==0) { // top row
                if (x==arranger->current_bar) {
                    // indicate the current bar
                    return APCMINI_YELLOW_BLINK;
                } else {
                    return APCMINI_OFF;
                }
            }
            return arranger->song_sections[arranger->current_section].grid[x].degree == (8-y);
        } else if (current_mode==MODE::QUALITY) {
            return arranger->song_sections[arranger->current_section].grid[x].type == (7-y);
        } else if (current_mode==MODE::INVERSION) {
            return arranger->song_sections[arranger->current_section].grid[x].inversion == (7-y);
        } else if (current_mode==MODE::PLAYLIST) {
            if (arranger->playlist.entries[x].section == (7-y)) {
                if (arranger->playlist.entries[x].repeats == 2) {
                    return APCMINI_GREEN  + (arranger->playlist_position==x ? 1 : 0);
                } else if (arranger->playlist.entries[x].repeats == 4) {
                    return APCMINI_YELLOW + (arranger->playlist_position==x ? 1 : 0);
                } else if (arranger->playlist.entries[x].repeats == 8) {
                    return APCMINI_RED    + (arranger->playlist_position==x ? 1 : 0);
                }
            }
            return APCMINI_OFF;
        }
        return APCMINI_OFF;
    }

    virtual bool apcmini_press(int inNumber, bool shifted) {
        if (inNumber>=0 && inNumber < NUM_SEQUENCES * APCMINI_DISPLAY_WIDTH) {
            //byte row = (NUM_SEQUENCES-1) - (inNumber / APCMINI_DISPLAY_WIDTH);
            int8_t row = inNumber / APCMINI_DISPLAY_WIDTH;
            int8_t col = inNumber - (row*APCMINI_DISPLAY_WIDTH);

            Serial_printf("apcmini_press(%i, %i) => row=%i, col=%i\n", inNumber, shifted, row, col);
            Serial_flush();

            if (current_mode==MODE::DEGREE) {
                int new_degree = row + 1;
                if (new_degree>0 && new_degree<=PITCHES_PER_SCALE) {
                    arranger->song_sections[arranger->current_section].grid[col].degree = new_degree;
                    mark_song_as_modified();
                    return true;
                }
            } else if (current_mode==MODE::QUALITY) {
                int new_quality = row;
                if (new_quality>=0 && new_quality<CHORD::NONE) {
                    arranger->song_sections[arranger->current_section].grid[col].type = (CHORD::Type)new_quality;
                    mark_song_as_modified();
                    return true;
                }
            } else if (current_mode==MODE::INVERSION) {
                int new_inversion = row;
                if (new_inversion>=0 && new_inversion<=MAX_INVERSIONS) {
                    arranger->song_sections[arranger->current_section].grid[col].inversion = new_inversion;
                    mark_song_as_modified();
                    return true;
                }
            } else if (current_mode==MODE::PLAYLIST) {
                if (row>=NUM_SONG_SECTIONS) {
                    return false;
                }
                int8_t original_repeats = arranger->playlist.entries[col].repeats;
                if (arranger->playlist.entries[col].section==row) {
                    arranger->playlist.entries[col].repeats*=2;
                    if (arranger->playlist.entries[col].repeats>16) 
                        arranger->playlist.entries[col].repeats = 2;
                } else {
                    arranger->playlist.entries[col] = { row, arranger->playlist.entries[col].repeats };
                }
                if (arranger->playlist.entries[col].repeats != original_repeats) {
                    mark_song_as_modified();
                }
                return true;
            }
        } else if (inNumber>=APCMINI_BUTTON_CLIP_STOP && inNumber < APCMINI_BUTTON_CLIP_STOP + VirtualBehaviour_Progression::MODE::NUM_MODES) {
            Serial_printf("apcmini_press(%i, %i) => mode=%i\n", inNumber, shifted, inNumber - APCMINI_BUTTON_CLIP_STOP);
            Serial_flush();
            this->current_mode = (VirtualBehaviour_Progression::MODE)(inNumber - APCMINI_BUTTON_CLIP_STOP);
            return true;
        } else if (inNumber==APCMINI_BUTTON_UNLABELED_1) {
            this->advance_progression_bar = !this->advance_progression_bar;
            mark_song_as_modified();
            return true;
        } else if (inNumber==APCMINI_BUTTON_UNLABELED_2) {
            this->advance_progression_playlist = !this->advance_progression_playlist;
            mark_song_as_modified();
            return true;
        } else if (inNumber==APCMINI_BUTTON_LEFT || inNumber==APCMINI_BUTTON_RIGHT) {
            if (current_mode==MODE::DEGREE || current_mode==MODE::QUALITY || current_mode==MODE::INVERSION) {
                if (inNumber==APCMINI_BUTTON_LEFT) {
                    move_bar(arranger->current_bar-1);
                } else {
                    move_bar(arranger->current_bar+1);
                }
                return true;
            } else if (current_mode==MODE::PLAYLIST) {
                if (inNumber==APCMINI_BUTTON_LEFT) {
                    move_playlist(arranger->playlist_position-1);
                } else {
                    move_playlist(arranger->playlist_position+1);
                }
                return true;
            }
        }

        return false;
    }
    virtual bool apcmini_release(int inNumber, bool shifted) {
        //byte row = (NUM_SEQUENCES-1) - (inNumber / APCMINI_DISPLAY_WIDTH);
        //byte col = inNumber - (((8-1)-row)*APCMINI_DISPLAY_WIDTH);
        return false;
    }


    virtual void change_bass_octave(int new_bass_octave) {
        bool was_playing = this->chord_player->is_playing_bass;
        int8_t note = this->chord_player->current_bass_note;
        if (was_playing) {
            this->chord_player->stop_bass_note();
        }
        this->bass_octave = new_bass_octave;
        if (was_playing) {
            this->chord_player->play_bass_note(note, MIDI_MAX_VELOCITY);
        }
    }

    virtual void change_topline_octave(int new_topline_octave) {
        bool was_playing = this->chord_player->is_playing_topline;
        int8_t note = this->chord_player->current_topline_note;
        if (was_playing) {
            this->chord_player->stop_topline_note();
        }
        this->topline_octave = new_topline_octave;
        if (was_playing) {
            this->chord_player->play_topline_note(note, MIDI_MAX_VELOCITY);
        }
    }

    virtual void change_chord_octave(int new_chord_octave) {
        bool was_playing = this->chord_player->is_playing_chord;
        chord_identity_t chord;
        memcpy(&chord, &arranger->current_chord, sizeof(chord_identity_t));
        if (was_playing) {
            this->chord_player->stop_chord();
        }
        this->chord_octave = new_chord_octave;
        if (was_playing) {
            if (debug) Serial_printf("change_chord_octave(%i): was playing chord %i\n", new_chord_octave, chord.degree);
            this->chord_player->play_chord(chord);
        }
    }


    virtual int requantise_all_notes() override {
        bool already_playing = this->chord_player->is_playing_chord;
        if (debug) Serial_println("behaviour_progression#requantise_all_notes()...");

        if (already_playing) {
            if (debug) Serial_printf("behaviour_progression#requantise_all_notes: already playing chord %i -- gonna stop!\n", arranger->current_chord.degree);
            if (debug) Serial_println("behaviour_progression#requantise_all_notes() is playing...");
            // TODO: this is a big hack -- need to sort this out properly
            // chord player needs to be able to stop the existing chord without applying quantisation..
            // so we temporarily disable quantisation, stop the chord, then restore the quantisation mode and re-play the chord to apply the new quantisation settings
            quantise_mode_t initial_global_quantise_mode = conductor->get_global_quantise_mode();

            conductor->set_global_quantise_mode(QUANTISE_MODE_NONE);
            this->chord_player->stop_chord();
            
            if (debug) Serial_printf("behaviour_progression#requantise_all_notes: now gonna re-play chord %i!\n", arranger->current_chord.degree);
            conductor->set_global_quantise_mode(initial_global_quantise_mode);
            this->chord_player->play_chord(arranger->current_chord);

            if (debug) Serial_printf("behaviour_progression#requantise_all_notes: played chord %i!\n", arranger->current_chord.degree);
            if (debug) Serial_println("behaviour_progression#requantise_all_notes() done.");

            return 4;   // assume there were 4 notes to requantise in this chord
        }

        if (debug) Serial_println("behaviour_progression#requantise_all_notes() isn't playing...");
        return 0;   
    }

    // changes section, but leaves reseting the bar to the caller to deal with (or not)
    virtual void change_section(int section_number) {
        arranger->change_section(section_number);
    }

    virtual bool save_playlist(int project_number = -1) {
        #ifdef ENABLE_SD
        if (project_number<0) project_number = project->current_project_number;

        LinkedList<String> section_lines = LinkedList<String>();
        arranger->playlist.save_project_add_lines(&section_lines);

        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            File myFile;

            make_project_folders(project_number);

            char filename[MAX_FILEPATH] = "";
            snprintf(filename, MAX_FILEPATH, FILEPATH_PLAYLIST_FORMAT, project_number);
            if (debug) Serial_printf(F("save_playlist(%i) writing to %s\n"), project_number, filename);
            if (SD.exists(filename)) {
              //Serial.printf(F("%s exists, deleting first\n"), filename); Serial.flush();
              SD.remove(filename);
              //Serial.println("deleted"); Serial.flush();
            }

            myFile = SD.open(filename, FILE_WRITE_BEGIN | (uint8_t)O_TRUNC);
            if (!myFile) {    
              if (debug) Serial_printf(F("Error: couldn't open %s for writing\n"), filename);
              //if (irqs_enabled) __enable_irq();
              return false;
            }
            if (debug) { Serial_println("Starting data write.."); Serial_flush(); }

            myFile.println(F("; begin playlist"));
            for (uint_fast16_t i = 0 ; i < section_lines.size() ; i++) {
                myFile.println(section_lines.get(i));
            }
            myFile.println(F("; end playlist"));
            myFile.close();

            messages_log_add(String("Saved to project : playlist ") + String(project_number));
        }

        mark_song_save_done();

        return true;
        #else
        return false;
        #endif
    }

    virtual bool load_playlist(int project_number = -1) {
        #ifdef ENABLE_SD
        if (project_number<0) project_number = project->getProjectNumber();

        Serial.printf("Progression#load_playlist(%i)\n", project_number);

        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            File myFile;

            char filename[MAX_FILEPATH] = "";
            snprintf(filename, MAX_FILEPATH, FILEPATH_PLAYLIST_FORMAT, project_number);
            if (debug) Serial_printf(F("load_playlist(%i) opening %s\n"), project_number, filename);
            myFile = SD.open(filename, FILE_READ);
            if (!myFile) {
                if (debug) Serial_printf(F("Error: Couldn't open %s for reading!\n"), filename);
                return false;
            }
            myFile.setTimeout(0);

            String line;
            while (line = myFile.readStringUntil('\n')) {
                if (line.startsWith(";")) continue;
                if (debug) Serial_printf("load_playlist: parsing line %s\n", line.c_str());
                arranger->playlist.parse_key_value(line.substring(0, line.indexOf('=')), line.substring(line.indexOf('=')+1));
            }
            myFile.close();
        }

        mark_song_save_done();

        return true;
        #else
        return false;
        #endif
    }

    virtual bool save_section(int section_number = -1, int project_number = -1) {
        #ifdef ENABLE_SD
        if (section_number<0) section_number = arranger->current_section;
        if (project_number<0) project_number = project->getProjectNumber();

        LinkedList<String> section_lines = LinkedList<String>();
        section_lines.add(String("current_section=")+String(section_number));
        if (section_number>=0 && section_number<NUM_SONG_SECTIONS) {
            arranger->song_sections[section_number].add_section_add_lines(&section_lines);
        }
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            File myFile;

            make_project_folders(project_number);

            char filename[MAX_FILEPATH] = "";
            snprintf(filename, MAX_FILEPATH, FILEPATH_SECTION_FORMAT, project_number, section_number);
            if (debug) Serial_printf(F("save_scene(%i, %i) writing to %s\n"), project_number, section_number, filename);
            if (SD.exists(filename)) {
              //Serial.printf(F("%s exists, deleting first\n"), filename); Serial.flush();
              SD.remove(filename);
              //Serial.println("deleted"); Serial.flush();
            }

            myFile = SD.open(filename, FILE_WRITE_BEGIN | (uint8_t)O_TRUNC);
            if (!myFile) {    
              if (debug) Serial_printf(F("Error: couldn't open %s for writing\n"), filename);
              //if (irqs_enabled) __enable_irq();
              return false;
            }
            if (debug) { Serial_println("Starting data write.."); Serial_flush(); }

            myFile.println(F("; begin section"));
            for (uint_fast16_t i = 0 ; i < section_lines.size() ; i++) {
                myFile.println(section_lines.get(i));
            }
            myFile.println(F("; end section"));
            myFile.close();

            messages_log_add(String("Saved to project : section ") + String(project_number) + " : " + String(section_number));
        }

        mark_song_save_done();

        return true;
        #else
        return false;
        #endif
    }


    virtual bool load_section(int section_number = -1, int project_number = -1) {
        #ifdef ENABLE_SD
        if (section_number<0) section_number = arranger->current_section;
        if (project_number<0) project_number = project->getProjectNumber();

        Serial.printf("Progression#load_section(section_number=%i, project_number=%i)\n", section_number, project_number);
        //debug = true;

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
                arranger->song_sections[section_number].parse_section_line(line.substring(0, line.indexOf('=')), line.substring(line.indexOf('=')+1));
            }
            myFile.close();
        }
        //debug = false;

        mark_song_save_done();

        return true;
        #else
        return false;
        #endif
    }

    // notify_project_changed: saveloadlib handles bulk project loading of
    // sections+playlist via SL_SCOPE_PROJECT.  Old per-file methods are kept
    // for individual section import/export.
    //
    // Migration: if old-format files exist but the new saveloadlib project
    // file has already been loaded (and presumably contained no arranger data
    // because it was saved before the migration), we detect non-empty old
    // files, load them, and re-save the project so the data is written in the
    // new format.  The old files are left in place for safety.
    virtual void notify_project_changed(int project_number) override {
        #ifdef ENABLE_SD
        // Migration must run only after saveloadlib has a valid root/tree,
        // otherwise re-save can fail and consume recovery files.
        if (SL_ROOT == nullptr || project == nullptr || project->save_tree == nullptr) {
            Serial.println("Arranger migration: save tree not ready, skipping migration for now.");
            return;
        }

        // One-time recovery path:
        // If normal legacy files are missing but '.migrated' backups exist,
        // promote backups back to legacy filenames so they can be imported.
        // After import, they are archived as '.restored' to avoid re-triggering.
        bool promoted_from_migrated = false;

        char filename[MAX_FILEPATH] = "";
        snprintf(filename, MAX_FILEPATH, FILEPATH_PLAYLIST_FORMAT, project_number);

        if (!SD.exists(filename)) {
            char migrated_playlist[MAX_FILEPATH] = "";
            snprintf(migrated_playlist, MAX_FILEPATH, FILEPATH_PLAYLIST_FORMAT ".migrated", project_number);
            char restored_playlist[MAX_FILEPATH] = "";
            snprintf(restored_playlist, MAX_FILEPATH, FILEPATH_PLAYLIST_FORMAT ".restored", project_number);
            const char* backup_playlist = nullptr;
            if (SD.exists(migrated_playlist)) backup_playlist = migrated_playlist;
            else if (SD.exists(restored_playlist)) backup_playlist = restored_playlist;

            if (backup_playlist != nullptr) {
                if (SD.rename(backup_playlist, filename)) {
                    promoted_from_migrated = true;
                    Serial.printf("Arranger restore: promoted %s -> %s\n", backup_playlist, filename);
                } else {
                    Serial.printf("Arranger restore: failed to promote %s -> %s\n", backup_playlist, filename);
                }
            }
        }

        for (int i = 0 ; i < NUM_SONG_SECTIONS ; i++) {
            char sec_filename[MAX_FILEPATH] = "";
            snprintf(sec_filename, MAX_FILEPATH, FILEPATH_SECTION_FORMAT, project_number, i);
            if (!SD.exists(sec_filename)) {
                char sec_migrated[MAX_FILEPATH] = "";
                snprintf(sec_migrated, MAX_FILEPATH, FILEPATH_SECTION_FORMAT ".migrated", project_number, i);
                char sec_restored[MAX_FILEPATH] = "";
                snprintf(sec_restored, MAX_FILEPATH, FILEPATH_SECTION_FORMAT ".restored", project_number, i);
                const char* backup_section = nullptr;
                if (SD.exists(sec_migrated)) backup_section = sec_migrated;
                else if (SD.exists(sec_restored)) backup_section = sec_restored;

                if (backup_section != nullptr) {
                    if (SD.rename(backup_section, sec_filename)) {
                        promoted_from_migrated = true;
                        Serial.printf("Arranger restore: promoted %s -> %s\n", backup_section, sec_filename);
                    } else {
                        Serial.printf("Arranger restore: failed to promote %s -> %s\n", backup_section, sec_filename);
                    }
                }
            }
        }

        // Check whether old-format playlist file exists for this project
        if (SD.exists(filename)) {
            Serial.printf("Arranger migration: found old playlist file %s, importing...\n", filename);
            load_playlist(project_number);
            for (int i = 0 ; i < NUM_SONG_SECTIONS ; i++) {
                char sec_filename[MAX_FILEPATH] = "";
                snprintf(sec_filename, MAX_FILEPATH, FILEPATH_SECTION_FORMAT, project_number, i);
                if (SD.exists(sec_filename)) {
                    Serial.printf("Arranger migration: found old section file %s, importing...\n", sec_filename);
                    load_section(i, project_number);
                }
            }
            // Re-save via saveloadlib so data is persisted in the new format
            Serial.println("Arranger migration: re-saving project in new format...");
            bool save_ok = project->save_project_settings(project_number);

            if (!save_ok) {
                Serial.println("Arranger migration: save failed; keeping legacy files for retry.");
                return;
            }

            // Rename old files so migration doesn't re-trigger
            char newname[MAX_FILEPATH] = "";
            snprintf(newname, MAX_FILEPATH, FILEPATH_PLAYLIST_FORMAT ".migrated", project_number);
            SD.rename(filename, newname);
            Serial.printf("Arranger migration: renamed %s -> %s\n", filename, newname);

            // If this migration came from promoted '.migrated' backups,
            // archive the re-created '.migrated' files as '.restored' so
            // this restore path runs only once.
            if (promoted_from_migrated) {
                char restored_name[MAX_FILEPATH] = "";
                snprintf(restored_name, MAX_FILEPATH, FILEPATH_PLAYLIST_FORMAT ".restored", project_number);
                if (SD.exists(restored_name)) SD.remove(restored_name);
                if (SD.rename(newname, restored_name)) {
                    Serial.printf("Arranger restore: archived %s -> %s\n", newname, restored_name);
                } else {
                    Serial.printf("Arranger restore: failed to archive %s -> %s\n", newname, restored_name);
                }
            }

            for (int i = 0 ; i < NUM_SONG_SECTIONS ; i++) {
                char sec_filename[MAX_FILEPATH] = "";
                snprintf(sec_filename, MAX_FILEPATH, FILEPATH_SECTION_FORMAT, project_number, i);
                if (SD.exists(sec_filename)) {
                    char sec_newname[MAX_FILEPATH] = "";
                    snprintf(sec_newname, MAX_FILEPATH, FILEPATH_SECTION_FORMAT ".migrated", project_number, i);
                    SD.rename(sec_filename, sec_newname);
                    Serial.printf("Arranger migration: renamed %s -> %s\n", sec_filename, sec_newname);

                    if (promoted_from_migrated) {
                        char sec_restored[MAX_FILEPATH] = "";
                        snprintf(sec_restored, MAX_FILEPATH, FILEPATH_SECTION_FORMAT ".restored", project_number, i);
                        if (SD.exists(sec_restored)) SD.remove(sec_restored);
                        if (SD.rename(sec_newname, sec_restored)) {
                            Serial.printf("Arranger restore: archived %s -> %s\n", sec_newname, sec_restored);
                        } else {
                            Serial.printf("Arranger restore: failed to archive %s -> %s\n", sec_newname, sec_restored);
                        }
                    }
                }
            }
            Serial.println("Arranger migration: done.");
        }
        #endif
    }


    #ifdef ENABLE_SCREEN
        virtual LinkedList<MenuItem*> *make_menu_items() override {
            LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();

            menu->add_pinned(new ProgressionPinnedMenuItem("Progression"));

            SubMenuItemBar *limit_ranges_bar = new SubMenuItemBar("Octave offsets", true, true);
            // TODO: properly stop chord player or bass/topline when changing octaves
            limit_ranges_bar->add(new LambdaNumberControl<int8_t>(
                "Chord octave", 
                [=] (int8_t octave) -> void { 
                    if(debug) Serial.printf("-----\nchanging chord_octave from %i to %i\n", chord_octave, octave);
                    this->change_chord_octave(octave);
                    if (debug) Serial.printf("done changing chord_octave, now %i\n-----\n", chord_octave);
                },
                [=] () -> int8_t { return this->chord_octave; },
                nullptr,
                0, 10, true, true
            ));
            limit_ranges_bar->add(new LambdaNumberControl<int8_t>(
                "Bass octave", 
                [=] (int8_t octave) -> void { 
                    if(debug) Serial.printf("-----\nchanging bass_octave from %i to %i\n", bass_octave, octave);
                    this->change_bass_octave(octave);
                    if (debug) Serial.printf("done changing bass_octave, now %i\n-----\n", bass_octave);
                },
                [=] () -> int8_t { return this->bass_octave; },
                nullptr,
                0, 10, true, true
            ));
            limit_ranges_bar->add(new LambdaNumberControl<int8_t>(
                "Topline octave", 
                [=] (int8_t octave) -> void { 
                    if(debug) Serial.printf("-----\nchanging topline_octave from %i to %i\n", topline_octave, octave);
                    this->change_topline_octave(octave);
                    if(debug) Serial.printf("changing topline_octave from %i to %i\n-----\n", topline_octave, octave);
                },
                [=] () -> int8_t { return this->topline_octave; },
                nullptr,
                0, 10, true, true
            ));
            menuitems->add(limit_ranges_bar);

            //this->sequencer->make_menu_items(menu, true);
            //this->output_processor->create_menu_items(true);

            // todo: this was cribbed from menu.cpp setup_menu_midi() -- can probably re-use the same controls here to save some memory!
            LambdaScaleMenuItemBar *global_quantise_bar = new LambdaScaleMenuItemBar(
                "Global Scale", 
                [=](scale_index_t scale) -> void { conductor->set_scale_type(scale); }, 
                [=]() -> scale_index_t { return conductor->get_scale_type(); },
                [=](int8_t scale_root) -> void { conductor->set_scale_root(scale_root); },
                [=]() -> int8_t { return conductor->get_scale_root(); },
                false, true, true
            );
            global_quantise_bar->add(new LambdaQuantiseModeControl(
                "Quant",
                [=](int8_t v) -> void { conductor->set_global_quantise_mode((quantise_mode_t)constrain((int)v, (int)QUANTISE_MODE_NONE, (int)QUANTISE_MODE_CHORD)); },
                [=]() -> int8_t { return (int8_t)conductor->get_global_quantise_mode(); }
            ));
            menuitems->add(global_quantise_bar);

            LambdaChordSubMenuItemBar *global_chord_bar = new LambdaChordSubMenuItemBar(
                "Global Chord", 
                [=](int8_t degree) -> void { conductor->set_chord_degree(degree); },
                [=]() -> int8_t { return conductor->get_chord_degree(); },
                [=](CHORD::Type chord_type) -> void { conductor->set_chord_type(chord_type); }, 
                [=]() -> CHORD::Type { return conductor->get_chord_type(); },
                [=](int8_t inversion) -> void { conductor->set_chord_inversion(inversion); },
                [=]() -> int8_t { return conductor->get_chord_inversion(); },
                false, true, true
            );
            // global_chord_bar->add(new LambdaToggleControl("Quantise",
            //     [=](bool v) -> void { conductor->set_global_quantise_chord_on(v); },
            //     [=]() -> bool { return conductor->is_global_quantise_chord_on(); }
            // ));
            menuitems->add(global_chord_bar);

            //chord_player->make_menu_items(menuitems);

            SubMenuItemBar *bar = new SubMenuItemBar("Chord controller", true, true);
            
            /*menuitems->add(new LambdaScaleMenuItemBar(
                "Chord controller", 
                [=] (scale_index_t s) -> void { chord_player->set_scale(s); },
                [=] (void) -> scale_index_t { return chord_player->get_scale(); },
                [=] (int8_t) -> void { chord_player_}
            ));*/

            bar->add(new LambdaToggleControl("Advance bar", 
                [=] (bool v) -> void { this->advance_progression_bar = v; mark_song_as_modified();},
                [=] (void) -> bool { return this->advance_progression_bar; }
            ));
            bar->add(new LambdaToggleControl("Advance playlist", 
                [=] (bool v) -> void { this->advance_progression_playlist = v; mark_song_as_modified();},
                [=] (void) -> bool { return this->advance_progression_playlist; }
            ));
            menuitems->add(bar);

            menuitems->add(new NoteDisplay("Progression notes", &this->note_tracker));
            menuitems->add(new NoteHarmonyDisplay(
                (const char*)"Progression harmony",
                &conductor->global_scale_identity.scale_number, 
                &conductor->global_scale_identity.root_note, 
                &this->note_tracker,
                &conductor->global_quantise_mode
            ));

            SubMenuItemBar *section_bar = new SubMenuItemBar("Section", true, true);
            section_bar->add(new LambdaNumberControl<int8_t>(
                "PlaylistPos", 
                [=] (int8_t pos) -> void { move_playlist(pos); },
                [=] () -> int8_t { return arranger->playlist_position; },
                nullptr
            ));
            section_bar->add(new LambdaNumberControl<int8_t>(
                "Section", 
                [=] (int8_t section) -> void { change_section(section); },
                [=] () -> int8_t { return arranger->current_section; },
                nullptr,
                0, 
                NUM_SONG_SECTIONS-1, 
                true, 
                false
            ));
            section_bar->add(new NumberControl<int8_t>("Plays", &arranger->current_section_plays, 0, 8, true, true));
            menuitems->add(section_bar);

            SubMenuItemBar *save_section_bar = new SubMenuItemBar("Section", false, true);
            save_section_bar->add(new LambdaActionConfirmItem("Load", [=] () -> void { this->load_section(arranger->current_section); }));
            save_section_bar->add(new LambdaActionConfirmItem("Save", [=] () -> void { this->save_section(arranger->current_section); }));
            menuitems->add(save_section_bar);

            SubMenuItemBar *save_playlist_bar = new SubMenuItemBar("Playlist", false, true);
            save_playlist_bar->add(new LambdaActionConfirmItem("Load", [=] () -> void { this->load_playlist(-1); }));
            save_playlist_bar->add(new LambdaActionConfirmItem("Save", [=] () -> void { this->save_playlist(-1); }));
            menuitems->add(save_playlist_bar);

            /*
            // test menu items to test generating new scales
            menuitems->add(new LambdaActionItem("Generate new scale", [=] () -> void {
                Serial.printf("Testing 'Major' scale 'w w h w w w h'...\n");
                const scale_pattern_t scale_pattern = *make_scale_pattern_t_from_string("w w h w w w h", "Major");
                for (int i = 0 ; i < PITCHES_PER_SCALE ; i++) {
                    Serial.printf("Creating scale with rotation %i\n", i);
                    //const scale_t scale = { "Major", scale_pattern, i};
                    const scale_t scale = *make_scale_t_from_pattern(&scale_pattern, "test", i);
                    print_scale(0, scale);    
                }

                Serial.printf("Testing 'test4' scale 'h h h h h h h'...\n");
                scale_pattern_t scale_pattern_test = *make_scale_pattern_t_from_string("h h h h h h h", "Invalid scale");
                scale_t scale = { "test4", &scale_pattern_test, 0};
                print_scale(0, scale);

                Serial.printf("Testing 'test5' scale 'Lydian.aug' rotated 2...\n");
                //const scale_pattern_t *scale_pattern_test2 = make_scale_pattern_t_from_string("w w w w w w w", "Invalid scale");
                const scale_t scale2 = *make_scale_t_from_pattern(scale_patterns[1], "??Lydian.aug", 2);
                print_scale(0, scale2);
                
            }));
            */

            // test menu item to output all scales
            menuitems->add(new LambdaActionItem("Output all scales", [=] () -> void {
                dump_all_scales();
            }));

            // test menu item to output all chords in the current scale
            menuitems->add(new LambdaActionItem("Output chords", [=] () -> void {
                dump_all_chords();
            }));

            // test menu item to output all scales and all chords
            menuitems->add(new LambdaActionItem("Output all scales and chords", [=] () -> void {
                dump_all_scales_and_chords();
            }));

            menuitems->add(new ToggleControl<bool>("Debug", &this->debug));

            this->create_menu_items_playlist();

            return menuitems;
        }

        
        //void create_menu_items_playlist();
        virtual void create_menu_items_playlist() {
            //LinkedList<MenuItem *> *menuitems = new LinkedList<MenuItem *>();

            // SubMenuItemBar *playlist_bar = new SubMenuItemBar("Playlist controls", false, true);
            // playlist_bar->add(new LambdaActionItem("Next section", [=] () -> void { move_next_playlist(); }));
            // menuitems->add(playlist_bar);

            // create only one set of enable/advance buttons, and save/load buttons, and re-use them for all of the pages here
            // add save/load buttons 
            SubMenuItemBar *save_load_bar = new SubMenuItemBar("Section controls", false, true);
            save_load_bar->add(new LambdaActionConfirmItem("Save", [=] () -> void { this->save_playlist(); }));
            save_load_bar->add(
                new CallbackMenuItem(
                    "State", 
                    [=] () -> const char* { return this->has_song_changes_to_save() ? "Unsaved" : "Saved"; }, 
                    [=] () -> uint16_t { return this->has_song_changes_to_save() ? RED : GREEN; },
                    false
                )
            );
            save_load_bar->add(new LambdaActionConfirmItem("Load", [=] () -> void { this->load_playlist(); }));

            // add controls to enable/disable advance of bars and section
            SubMenuItemBar *advance_bar = new SubMenuItemBar("Advance controls", false, true);
            advance_bar->add(new LambdaToggleControl("Advance bar", 
                [=] (bool v) -> void { this->advance_progression_bar = v; },
                [=] (void) -> bool { return this->advance_progression_bar; }
            ));
            advance_bar->add(new LambdaToggleControl("Advance playlist", 
                [=] (bool v) -> void { this->advance_progression_playlist = v; },
                [=] (void) -> bool { return this->advance_progression_playlist; }
            ));

            
            // a page showing and allowing edit of the the song structure (playlist)
            // header row
            menu->add_page("Playlist", this->colour, false);
            menu->add(new MenuItem("Section      Repeats        ", false, true));
            for (int i = 0 ; i < NUM_SONG_SECTIONS ; i++) {
                menu->add(new LambdaPlaylistSubMenuItemBarWithIndicator(
                    (String("Slot ") + String(i)).c_str(),
                    [=](int8_t section) -> void { arranger->playlist.entries[i].section = section; mark_song_as_modified(); },
                    [=]() -> int8_t { return arranger->playlist.entries[i].section; },
                    [=](int8_t repeats) -> void { arranger->playlist.entries[i].repeats = repeats; mark_song_as_modified(); },
                    [=]() -> int8_t { return arranger->playlist.entries[i].repeats; },
                    i,
                    &arranger->current_section,
                    NUM_SONG_SECTIONS, 
                    MAX_REPEATS,
                    false, false
                ));
            }

            menu->add(save_load_bar);
            menu->add(advance_bar);

            // one page per song section
            for (int i = 0 ; i < NUM_SONG_SECTIONS ; i++) {
                menu->add_page((String("Section ") + String(i)).c_str(), this->colour, false);

                // header row
                menu->add(new MenuItem("Degree      Type        Inversion", false, true));

                // then one row per bar
                for (int j = 0 ; j < CHORDS_PER_SECTION ; j++) {
                    LambdaChordSubMenuItemBarWithIndicator *section_bar = new LambdaChordSubMenuItemBarWithIndicator(
                        (String("Bar ") + String(j)).c_str(),
                        [=](int8_t degree) -> void { 
                            arranger->song_sections[i].grid[j].degree = degree; 
                            mark_song_as_modified(); 
                        },
                        [=]() -> int8_t { 
                            return arranger->song_sections[i].grid[j].degree; 
                        },
                        [=](CHORD::Type chord_type) -> void { 
                            arranger->song_sections[i].grid[j].type = chord_type; 
                            mark_song_as_modified(); },
                        [=]() -> CHORD::Type { 
                            return arranger->song_sections[i].grid[j].type; 
                        },
                        [=](int8_t inversion) -> void { 
                            arranger->song_sections[i].grid[j].inversion = inversion; 
                            mark_song_as_modified(); 
                        },
                        [=]() -> int8_t { 
                            return arranger->song_sections[i].grid[j].inversion; 
                        },
                        i, j, &arranger->current_section, &arranger->current_bar,
                        false, false, false
                    );
                    menu->add(section_bar);
                }

                menu->add(save_load_bar);
                menu->add(advance_bar);
            }
        }

    #endif
};

extern VirtualBehaviour_Progression *behaviour_progression;

#endif
