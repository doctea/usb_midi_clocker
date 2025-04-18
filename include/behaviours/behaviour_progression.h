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

extern MIDIMatrixManager *midi_matrix_manager;

#define NUM_SONG_SECTIONS 4
#define NUM_PLAYLIST_SLOTS 8

class VirtualBehaviour_Progression : virtual public VirtualBehaviourBase {
    public:
    source_id_t source_id_chord_octave = -1;
    source_id_t source_id_bass = -1;
    source_id_t source_id_topline = -1;

    uint8_t BASS_CHANNEL = 2,   TOPLINE_CHANNEL = 3;
    uint8_t bass_octave = 2,    topline_octave = 3, chord_octave = 5;

    virtual bool transmits_midi_notes() { return true; }

    struct song_section_t {
        chord_identity_t grid[8];
        
        virtual void add_section_add_lines(LinkedList<String> *lines) {
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
            } 
            return false;
        };
    };

    song_section_t song_sections[NUM_SONG_SECTIONS];

    struct playlist_entry_t {
        int8_t section;
        int8_t repeats;
    };
    struct playlist_t {
        playlist_entry_t entries[NUM_PLAYLIST_SLOTS] = { 
            { 0, 2 },
            { 1, 2 },
            { 2, 2 },
            { 3, 2 },
            { 0, 2 },
            { 1, 2 },
            { 2, 2 },
            { 3, 2 },
        };
        virtual void save_project_add_lines(LinkedList<String> *lines) {
            for (int i = 0 ; i < NUM_PLAYLIST_SLOTS ; i++) {
                lines->add(String("section_")+String(i)+String("=")+String(entries[i].section));
                lines->add(String("repeats_")+String(i)+String("=")+String(entries[i].repeats));
            }
        };
        virtual bool parse_key_value(String key, String value) {
            if (key.startsWith("section_")) {
                int8_t slot = key.substring(8,9).toInt();
                if (slot>=0 && slot<NUM_PLAYLIST_SLOTS) {
                    entries[slot].section = value.toInt();
                    return true;
                }
            } else if (key.startsWith("repeats_")) {
                int8_t slot = key.substring(8,9).toInt();
                if (slot>=0 && slot<NUM_PLAYLIST_SLOTS) {
                    entries[slot].repeats = value.toInt();
                    return true;
                }
            }
            return false;
        };
    };
    playlist_t playlist;
    int8_t playlist_position = 0;

    enum MODE {
        DEGREE,
        QUALITY,
        INVERSION,
        PLAYLIST,
        NUM_MODES
    };

    MODE current_mode = DEGREE;
    chord_identity_t current_chord;
    int8_t current_section = 0;
    int8_t current_section_plays = 0;

    bool advance_progression_playlist = true;
    bool advance_progression_bar = true;

    ChordPlayer *chord_player = new ChordPlayer(
        [=] (int8_t channel, int8_t note, int8_t velocity) -> void { this->sendNoteOn (note, velocity, channel); },
        [=] (int8_t channel, int8_t note, int8_t velocity) -> void { this->sendNoteOff(note, velocity, channel); },
        [=] (int8_t channel, int8_t note, int8_t velocity) -> void { this->sendNoteOn (note, velocity, BASS_CHANNEL); },
        [=] (int8_t channel, int8_t note, int8_t velocity) -> void { this->sendNoteOff(note, velocity, BASS_CHANNEL); },
        [=] (int8_t channel, int8_t note, int8_t velocity) -> void { this->sendNoteOn (note, velocity, TOPLINE_CHANNEL); },
        [=] (int8_t channel, int8_t note, int8_t velocity) -> void { this->sendNoteOff(note, velocity, TOPLINE_CHANNEL); }
    );

    VirtualBehaviour_Progression() : DeviceBehaviourUltimateBase() {
        //memset(grid, 0, 64);
        //this->chord_player->debug = true;
        /*song_sections[0].grid[0].degree = 1;
        song_sections[0].grid[1].degree = 2;
        song_sections[0].grid[2].degree = 5;
        song_sections[0].grid[3].degree = 4;
        song_sections[0].grid[4].degree = 3;
        song_sections[0].grid[5].degree = 2;
        song_sections[0].grid[6].degree = 6;
        song_sections[0].grid[7].degree = 3;*/

        //this->debug = this->chord_player->debug = true;
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
            if (debug) Serial_printf("actualSendNoteOn to bass: channel %i, note %i, velocity %i\n", channel, note, velocity);
            note += 12 * bass_octave;
            if(is_valid_note(note)) {
                midi_matrix_manager->processNoteOn(this->source_id_bass, note, velocity, 1);
            }
        } else if (channel==TOPLINE_CHANNEL) {
            if (debug) Serial_printf("actualSendNoteOn to topline: channel %i, note %i, velocity %i\n", channel, note, velocity);
            note += 12 * topline_octave;
            if(is_valid_note(note)) {
                midi_matrix_manager->processNoteOn(this->source_id_topline, note, velocity, 1);
            }
        } else {
            midi_matrix_manager->processNoteOn(this->source_id, note, velocity, channel);
            // send the same chord sequence to the second source_id 5 octaves up
            note += 12 * chord_octave;
            if (is_valid_note(note)) {
                midi_matrix_manager->processNoteOn(this->source_id_chord_octave, note, velocity, channel);
            }
        }
    }
    virtual void actualSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        if (channel==BASS_CHANNEL) {
            note += 12 * bass_octave;
            if (debug) Serial_printf("actualSendNoteOff to bass: channel %i, note %i, velocity %i\n", channel, note, velocity);
            if(is_valid_note(note)) {
                midi_matrix_manager->processNoteOff(this->source_id_bass, note, velocity, 1);
            }
        } else if (channel==TOPLINE_CHANNEL) {
            if (debug) Serial_printf("actualSendNoteOff to topline: channel %i, note %i, velocity %i\n", channel, note, velocity);
            note += 12 * topline_octave;
            if(is_valid_note(note)) {
                midi_matrix_manager->processNoteOff(this->source_id_topline, note, velocity, 1);
            }
        } else {
            midi_matrix_manager->processNoteOff(this->source_id, note, velocity, channel);
            // send the same chord sequence to the second source_id 5 octaves up
            note += 12 * chord_octave;
            if (is_valid_note(note)) {
                midi_matrix_manager->processNoteOff(this->source_id_chord_octave, note, velocity, channel);
            }
        }
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
            &advance_progression_bar
        ));
        this->saveable_parameters->add(new LSaveableParameter<bool>(
            "Advance playlist",
            "Progression", 
            &advance_progression_playlist
        ));

        this->saveable_parameters->add(new LSaveableParameter<uint8_t>(
            "Chord octave",
            "Progression", 
            &chord_octave
        ));
        this->saveable_parameters->add(new LSaveableParameter<uint8_t>(
            "Bass octave",
            "Progression", 
            &bass_octave
        ));
        this->saveable_parameters->add(new LSaveableParameter<uint8_t>(
            "Topline octave",
            "Progression", 
            &topline_octave
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

    void move_next_playlist() {
        move_playlist(playlist_position+1);
    }
    void move_playlist(int8_t pos) {
        playlist_position = pos;
        if (playlist_position>=NUM_PLAYLIST_SLOTS) playlist_position = 0;
        if (playlist_position<0) playlist_position = NUM_PLAYLIST_SLOTS-1;
        change_section(playlist.entries[playlist_position].section);
    }

    virtual void on_end_phrase(uint32_t phrase_number) override {
        if (debug) Serial_printf("on_end_phrase %2i\n", phrase_number);
        //if (debug) dump_grid();
        // todo: this should only move the section on every 2 phrases, not every phrase
        if (advance_progression_bar && advance_progression_playlist) {
            current_section_plays++;

            if (current_section_plays >= playlist.entries[playlist_position].repeats) {
                if (this->debug) Serial_printf("Reached %i of %i plays; changing section from %i to %i\n", current_section_plays, playlist.entries[playlist_position].repeats, current_section, current_section+1);
                current_section_plays = 0;
                // todo: crashes?!
                //change_section(current_section+1);
                move_next_playlist();

                this->chord_player->stop_chord();
                // trying to start chords here seems to cause intermittent crashes when changing sections..?
                //if (this->current_chord.is_valid_chord())
                //    this->chord_player->play_chord(song_sections[current_section].grid[BPM_CURRENT_BAR]);
            }
        }
    }

    virtual void on_end_bar(int bar_number) override {
        this->chord_player->stop_chord();
    }

    virtual void on_restart() override {
        playlist_position = 0;
        current_bar = -1;
    }

    void move_bar(int new_bar) {
        if (new_bar>=8) {
            new_bar = 0;
        } else if (new_bar<0) {
            new_bar = 7;
        }
        current_bar = new_bar;
        if (debug) Serial_printf("move_bar(%i)\n", new_bar);
        this->set_current_chord(song_sections[current_section].grid[current_bar]);

        // send the chord for the current degree
        if (this->current_chord.is_valid_chord())
            this->chord_player->play_chord(this->current_chord);
    }

    int8_t current_bar = -1; // initialise to -1 so that the first bar is 0
    virtual void on_bar(int bar_number) override {
        if (debug) Serial.printf("on_bar %2i with current_bar %2i\n", bar_number, current_bar);
        //if (advance_progression_bar) {
            //bar_number = BPM_CURRENT_BAR;
            //bar_number %= 8;
            //bar_number %= BARS_PER_PHRASE;// * 2;
            if (advance_progression_bar) {
                move_bar(current_bar+1);
            } else {
                move_bar(current_bar);
            }

            if (debug) Serial_printf("=======\non_end_bar %2i (going into bar number %i)\n", BPM_CURRENT_BAR % 8, current_bar);
            if (debug) dump_grid();

            if (debug) Serial_printf("=======\n");
        //}
    }



    virtual int8_t get_cell_colour_for(uint8_t x, uint8_t y) {
        if (/*y==0 ||*/ x>=8 || y>=8) return 0;
        if (current_mode==MODE::DEGREE) {
            if (y==0) { // top row
                if (x==current_bar) {
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
        } else if (current_mode==MODE::PLAYLIST) {
            if (playlist.entries[x].section == (7-y)) {
                if (playlist.entries[x].repeats == 2) {
                    return APCMINI_GREEN  + (playlist_position==x ? 1 : 0);
                } else if (playlist.entries[x].repeats == 4) {
                    return APCMINI_YELLOW + (playlist_position==x ? 1 : 0);
                } else if (playlist.entries[x].repeats == 8) {
                    return APCMINI_RED    + (playlist_position==x ? 1 : 0);
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
            } else if (current_mode==MODE::PLAYLIST) {
                if (row>=NUM_SONG_SECTIONS) {
                    //playlist.entries[col].section = row;
                    return false;
                }
                if (playlist.entries[col].section==row) {
                    playlist.entries[col].repeats*=2;
                    if (playlist.entries[col].repeats>16) playlist.entries[col].repeats = 2;
                } else {
                    playlist.entries[col] = { row, playlist.entries[col].repeats };
                }
                return true;
            }
        } else if (inNumber>=APCMINI_BUTTON_CLIP_STOP && inNumber < APCMINI_BUTTON_CLIP_STOP + VirtualBehaviour_Progression::MODE::NUM_MODES) {
            Serial_printf("apcmini_press(%i, %i) => mode=%i\n", inNumber, shifted, inNumber - APCMINI_BUTTON_CLIP_STOP);
            Serial.flush();
            this->current_mode = (VirtualBehaviour_Progression::MODE)(inNumber - APCMINI_BUTTON_CLIP_STOP);
            return true;
        } else if (inNumber==APCMINI_BUTTON_UNLABELED_1) {
            this->advance_progression_bar = !this->advance_progression_bar;
            return true;
        } else if (inNumber==APCMINI_BUTTON_UNLABELED_2) {
            this->advance_progression_playlist = !this->advance_progression_playlist;
            return true;
        } else if (inNumber==APCMINI_BUTTON_LEFT || inNumber==APCMINI_BUTTON_RIGHT) {
            if (current_mode==MODE::DEGREE || current_mode==MODE::QUALITY || current_mode==MODE::INVERSION) {
                if (inNumber==APCMINI_BUTTON_LEFT) {
                    move_bar(current_bar-1);
                } else {
                    move_bar(current_bar+1);
                }
                return true;
            } else if (current_mode==MODE::PLAYLIST) {
                if (inNumber==APCMINI_BUTTON_LEFT) {
                    move_playlist(playlist_position-1);
                    //move_bar(0);
                } else {
                    move_playlist(playlist_position+1);
                    //move_bar(0);
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
        bool was_playing = this->chord_player->is_playing;
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
        bool was_playing = this->chord_player->is_playing;
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
        bool was_playing = this->chord_player->is_playing;
        chord_identity_t chord;// = this->chord_player->current_chord_data.chord;
        memcpy(&chord, &this->current_chord, sizeof(chord_identity_t));
        //int8_t note = this->chord_player->current_chord_data.chord_root;
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
        bool already_playing = this->chord_player->is_playing;
        if (debug) Serial_println("behaviour_progression#requantise_all_notes()...");

        if (already_playing) {
            if (debug) Serial_printf("behaviour_progression#requantise_all_notes: already playing chord %i -- gonna stop!\n", this->current_chord.degree);
            if (debug) Serial_println("behaviour_progression#requantise_all_notes() is playing...");
            bool initial_global_quantise_on = midi_matrix_manager->global_quantise_on;
            bool initial_global_quantise_chord_on = midi_matrix_manager->global_quantise_chord_on;   
            midi_matrix_manager->global_quantise_on = false;
            midi_matrix_manager->global_quantise_chord_on = false;
            this->chord_player->stop_chord();
            midi_matrix_manager->global_quantise_on = initial_global_quantise_on;
            midi_matrix_manager->global_quantise_chord_on = initial_global_quantise_chord_on;
            if (debug) Serial_printf("behaviour_progression#requantise_all_notes: now gonna play chord %i!\n",this->current_chord.degree);
            this->chord_player->play_chord(this->current_chord);
            if (debug) Serial_printf("behaviour_progression#requantise_all_notes: played chord %i!\n", this->current_chord.degree);
            if (debug) Serial_println("behaviour_progression#requantise_all_notes() done.");

            return 4;   // assume there were 4 notes to requantise in this chord
        }

        if (debug) Serial_println("behaviour_progression#requantise_all_notes() isn't playing...");
        return 0;   
    }

    // changes section, but leaves reseting the bar to the caller to deal with (or not)
    virtual void change_section(int section_number) {
        //Serial.printf("change_section(%i)\n", section_number); Serial.flush();
        if (section_number==NUM_SONG_SECTIONS) 
            section_number = 0;
        else if (section_number<0) 
            section_number = NUM_SONG_SECTIONS-1;

        this->current_section_plays = 0;
        current_section = section_number;
    }

    virtual bool save_playlist(int project_number = -1) {
        #ifdef ENABLE_SD
        if (project_number<0) project_number = project->current_project_number;

        LinkedList<String> section_lines = LinkedList<String>();
        playlist.save_project_add_lines(&section_lines);

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
                playlist.parse_key_value(line.substring(0, line.indexOf('=')), line.substring(line.indexOf('=')+1));
            }
            myFile.close();
        }

        return true;
        #else
        return false;
        #endif
    }

    virtual bool save_section(int section_number = -1, int project_number = -1) {
        #ifdef ENABLE_SD
        if (section_number<0) section_number = current_section;
        if (project_number<0) project_number = project->getProjectNumber();

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
            if (debug) Serial_printf(F("save_pattern(%i, %i) writing to %s\n"), project_number, section_number, filename);
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
        return true;
        #else
        return false;
        #endif
    }


    virtual bool load_section(int section_number = -1, int project_number = -1) {
        #ifdef ENABLE_SD
        if (section_number<0) section_number = current_section;
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
                song_sections[section_number].parse_section_line(line.substring(0, line.indexOf('=')), line.substring(line.indexOf('=')+1));
            }
            myFile.close();
        }
        //debug = false;

        return true;
        #else
        return false;
        #endif
    }

    virtual void notify_project_changed(int project_number) override {
        // todo: load the playlist and section data for the new project
        load_playlist(project_number);
        for (int i = 0 ; i < NUM_SONG_SECTIONS ; i++) {
            load_section(i, project_number);
        }
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
                [=](scale_index_t scale) -> void { midi_matrix_manager->set_global_scale_type(scale); }, 
                [=]() -> scale_index_t { return midi_matrix_manager->get_global_scale_type(); },
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
                [=] (scale_index_t s) -> void { chord_player->set_scale(s); },
                [=] (void) -> scale_index_t { return chord_player->get_scale(); },
                [=] (int8_t) -> void { chord_player_}
            ));*/

            bar->add(new LambdaToggleControl("Advance bar", 
                [=] (bool v) -> void { this->advance_progression_bar = v; },
                [=] (void) -> bool { return this->advance_progression_bar; }
            ));
            bar->add(new LambdaToggleControl("Advance playlist", 
                [=] (bool v) -> void { this->advance_progression_playlist = v; },
                [=] (void) -> bool { return this->advance_progression_playlist; }
            ));
            menuitems->add(bar);

            menuitems->add(new NoteDisplay("Progression notes", &this->note_tracker));
            menuitems->add(new NoteHarmonyDisplay(
                (const char*)"Progression harmony", 
                &midi_matrix_manager->global_scale_identity.scale_number, 
                &midi_matrix_manager->global_scale_identity.root_note, 
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
                "PlaylistPos", 
                [=] (int8_t pos) -> void { move_playlist(pos); },
                [=] () -> int8_t { return this->playlist_position; },
                nullptr
            ));
            section_bar->add(new LambdaNumberControl<int8_t>(
                "Section", 
                [=] (int8_t section) -> void { change_section(section); },
                [=] () -> int8_t { return this->current_section; },
                nullptr,
                0, 
                NUM_SONG_SECTIONS-1, 
                true, 
                false
            ));
            /*section_bar->add(new LambdaNumberControl<int8_t>(
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
            ));*/
            section_bar->add(new NumberControl<int8_t>("Plays", &this->current_section_plays, 0, 8, true, true));
            menuitems->add(section_bar);

            SubMenuItemBar *save_section_bar = new SubMenuItemBar("Section", false, true);
            save_section_bar->add(new LambdaActionConfirmItem("Load", [=] () -> void { this->load_section(current_section); }));
            save_section_bar->add(new LambdaActionConfirmItem("Save", [=] () -> void { this->save_section(current_section); }));
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

            return menuitems;
        }
    #endif

};

extern VirtualBehaviour_Progression *behaviour_progression;

#endif