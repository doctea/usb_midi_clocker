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

#include "chord_player.h"

extern MIDIMatrixManager *midi_matrix_manager;

class VirtualBehaviour_Progression : virtual public DeviceBehaviourUltimateBase {
    public:

    int8_t grid[8][8];
    int8_t degree = 0;
    //int8_t current_degree = 0;

    ChordPlayer *chord_player = new ChordPlayer(
        [=] (int8_t channel, int8_t note, int8_t velocity) -> void {
            this->sendNoteOn(note, velocity, channel);
        },
        [=] (int8_t channel, int8_t note, int8_t velocity) -> void {
            this->sendNoteOff(note, velocity, channel);
        }
    );

    VirtualBehaviour_Progression() : DeviceBehaviourUltimateBase() {

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
            [=](int8_t degree) -> void { midi_matrix_manager->set_global_chord_degree(degree); },
            [=]() -> int8_t { return midi_matrix_manager->get_global_chord_degree(); },
            false, true, true
        );
        global_quantise_bar->add(new LambdaToggleControl("Quantise",
            [=](bool v) -> void { midi_matrix_manager->set_global_quantise_on(v); },
            [=]() -> bool { return midi_matrix_manager->is_global_quantise_on(); }
        ));
        menuitems->add(global_quantise_bar);

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

        menuitems->add(bar);

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
            Serial.printf("invalid degree %i\n", degree);
            //this->chord_player->trigger_off_for_pitch_because_length(-1);
        }
    }

    virtual void setup_saveable_parameters() override {
        DeviceBehaviourUltimateBase::setup_saveable_parameters();
        //this->sequencer->setup_saveable_parameters();

        // todo: better way of 'nesting' a sequencer/child object's saveableparameters within a host object's
        /*
        for(unsigned int i = 0 ; i < sequencer->saveable_parameters->size() ; i++) {
            this->saveable_parameters->add(sequencer->saveable_parameters->get(i));
        }
        */
    }

    virtual void on_end_bar(int bar_number) override {
        //this->process_clocks(step);
        //if (is_bpm_on_bar(tick)) {
        //bar_number++;
        bar_number = BPM_CURRENT_BAR + 1;
        bar_number %= 8;
        for (int i = 8 ; i > 0 ; i--) {
            int d = 8 - i;
            if (grid[bar_number][i]>0) {
                Serial.printf(
                    "on_end_bar %2i: Progression starting bar %i: found active degree %i at grid column %i\n", 
                    BPM_CURRENT_BAR, 
                    bar_number, 
                    d,
                    bar_number
                );
                this->set_degree(d+1);
            }
        }
        //}
    }

    virtual bool apcmini_press(int inNumber, bool shifted) {
        byte row = (NUM_SEQUENCES-1) - (inNumber / APCMINI_DISPLAY_WIDTH);
        byte col = inNumber - (((8-1)-row)*APCMINI_DISPLAY_WIDTH);
        
        if (shifted)
            grid[row][col]--;
        else
            grid[row][col]++;

        if (grid[row][col]>6)
            grid[row][col] = 0;
        else if (grid[row][col]<0)
            grid[row][col] = 6;

        return true;
    }
    virtual bool apcmini_release(int inNumber, bool shifted) {
        byte row = (NUM_SEQUENCES-1) - (inNumber / APCMINI_DISPLAY_WIDTH);
        byte col = inNumber - (((8-1)-row)*APCMINI_DISPLAY_WIDTH);
        return false;
    }

};

extern VirtualBehaviour_Progression *behaviour_progression;

#endif