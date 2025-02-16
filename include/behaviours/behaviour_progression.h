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

class VirtualBehaviour_Progression : virtual public VirtualBehaviourBase {
    public:

    enum MODE {
        DEGREE,
        QUALITY,
        INVERSION
    };

    MODE current_mode = DEGREE;

    chord_identity_t grid[8];
    int8_t degree = 0;
    //int8_t current_degree = 0;

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
        this->chord_player->debug = true;
        grid[0].degree = 1;
        grid[1].degree = 2;
        grid[2].degree = 5;
        grid[3].degree = 4;
        grid[4].degree = 3;
        grid[5].degree = 2;
        grid[6].degree = 6;
        grid[7].degree = 3;
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
            [=] (void) -> int8_t { return this->degree; },
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

        return menuitems;
    }

    // degrees should be 1-7; 0 implies no chord, -1 implies 'use global'?
    void set_degree(int8_t degree) {
        this->degree = degree;
        if (degree>=-1 && degree<PITCHES_PER_SCALE+1) {
            /*// get the pitch for this degree in the current scale 
            int8_t pitch = quantise_get_root_pitch_for_degree(degree);
            pitch += (3*12);    // add octaves
            this->chord_player->trigger_on_for_pitch(pitch);
            */
            midi_matrix_manager->set_global_chord_degree(degree);
        } else {
            this->chord_player->stop_chord();
            Serial_printf("invalid degree %i\n", degree);
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

    int8_t get_degree_from_grid(int8_t bar_number) {
        //int8_t retval = -1;

        if (debug) Serial_printf("get_degree_from_grid passed bar_number=%2i\n", bar_number);
        return grid[bar_number].degree;

        /*for (int i = 8 ; i > 0 ; i--) {
            if (grid[bar_number].chord_degree>0) {
                return grid[bar_number].chord_degree;
                int d = 7 - i;
                d += 1;
                Serial.printf(
                    "get_degree_from_grid %2i: Progression starting bar_number=%i: found active degree %i at grid column %i, row %i\n", 
                    BPM_CURRENT_BAR, 
                    bar_number, 
                    d,
                    bar_number,
                    i
                );
                retval = d;
            }
        }*/
        //return retval;
    }

    virtual int8_t get_cell_colour_for(uint8_t x, uint8_t y) {
        if (y==0 || x>=8 || y>=8) return 0;
        return grid[x].degree == (8-y);
    }

    void dump_grid() {
        if (!Serial) return;

        Serial_printf("dump_grid:");
        //for (int y = 0 ; y < 8 ; y++) {
            //Serial.printf("Grid row %i: [ ", y);
            for (int x = 0 ; x < 8 ; x++) {
                //Serial.printf("%i ", grid[x].chord_degree);
                //Serial.printf("%i ", get_cell_colour_for(x, y));
                Serial_printf("{ %i %i=%5s %i }, ", grid[x].degree, grid[x].type, chords[grid[x].type].label, grid[x].inversion);
            }
            Serial_println("]");
        //}
        Serial_println("------");
    }

    virtual void on_end_bar(int bar_number) override {
        this->chord_player->stop_chord();

        if (advance_progression) {
            bar_number = BPM_CURRENT_BAR + 1;
            bar_number %= 8;
            //bar_number %= BARS_PER_PHRASE;// * 2;

            if (debug) Serial_printf("=======\non_end_bar %2i (going into bar number %i)\n", BPM_CURRENT_BAR % 8, bar_number);
            if (debug) dump_grid();

            int8_t degree = this->get_degree_from_grid(bar_number);
            if (degree>0) {
                if (debug) Serial_printf("on_end_bar %2i (going into %i): got degree %i\n", BPM_CURRENT_BAR % 8, bar_number, degree);
                this->set_degree(degree);
            } else {
                this->set_degree(-1);
                if (debug) Serial_printf("on_end_bar %2i (going into %i): no degree found\n", BPM_CURRENT_BAR % 8, bar_number);
            }
            if (debug) Serial_printf("=======\n");
        }

        this->chord_player->stop_chord();
    }

    virtual void on_bar(int bar_number) override {
        // send the chord for the current degree
        if (this->degree>0)
            this->chord_player->play_chord(
                quantise_get_root_pitch_for_degree(this->degree), 
                midi_matrix_manager->get_global_chord_type(), 
                midi_matrix_manager->get_global_chord_inversion()
            );
    }

    virtual bool apcmini_press(int inNumber, bool shifted) {
        //byte row = (NUM_SEQUENCES-1) - (inNumber / APCMINI_DISPLAY_WIDTH);
        byte row = inNumber / APCMINI_DISPLAY_WIDTH;
        byte col = inNumber - (row*APCMINI_DISPLAY_WIDTH);

        Serial_printf("apcmini_press(%i, %i) => row=%i, col=%i\n", inNumber, shifted, row, col);

        if (current_mode==MODE::DEGREE) {
            int new_degree = row + 1;
            if (new_degree>0 && new_degree<=7) {
                grid[col].degree = new_degree;
                return true;
            }
            //grid[col].chord_degree = new_degree;
            /*if (shifted)
                grid[col][row]--;
            else
                grid[col][row]++;

            if (grid[col][row]>6)
                grid[col][row] = 0;
            else if (grid[col][row]<0)
                grid[col][row] = 6;*/
        }

        return true;
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

        this->chord_player->play_chord(
            quantise_get_root_pitch_for_degree(this->degree), 
            midi_matrix_manager->get_global_chord_type(), 
            midi_matrix_manager->get_global_chord_inversion()
        );
    }

};

extern VirtualBehaviour_Progression *behaviour_progression;

#endif