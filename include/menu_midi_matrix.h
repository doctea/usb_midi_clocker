#ifndef MENU_MIDI_MATRIX__INCLUDED
#define MENU_MIDI_MATRIX__INCLUDED

#include "Config.h"
#include "midi_outs.h"
#include "midi_out_wrapper.h"
#include "midi_mapper_matrix_manager.h"

#include "menuitems.h"

class MidiMatrixSelectorControl : public SelectorControl {
    //void (*setter_func)(MIDIOutputWrapper *midi_output);
    //MIDIOutputWrapper *initial_selected_output_wrapper = nullptr;

    public:
    int actual_value_index;
    int selected_source_index = -1;
    int selected_target_index = -1;

    MidiMatrixSelectorControl(const char *label) : SelectorControl(label, 0) {};

    /*virtual void configure (MIDIOutputWrapper *initial_selected_output_wrapper, void (*setter_func)(MIDIOutputWrapper*)) {
        this->initial_selected_output_wrapper = initial_selected_output_wrapper;
        this->setter_func = setter_func;
        Serial.printf("configured %s @ ", initial_selected_output_wrapper->label);
        Serial.printf("%u and %u\n", this->initial_selected_output_wrapper, this->setter_func);
    }*/

    /*virtual void on_add() {
        actual_value_index = -1;
        Serial.println("MidiOutputSelectorControl#on_add()");
        if (initial_selected_output_wrapper==nullptr) {
            Serial.printf("No initial output wrapper passed to %s\n", this->label);
            this->selected_value_index = this->actual_value_index = -1;
            return;
        }
        Serial.printf("MidiOutputSelectorControl@ %p...\n", initial_selected_output_wrapper);
        Serial.printf("MidiOutputSelectorControl looking for '%s' at %u...\n", initial_selected_output_wrapper->label, *initial_selected_output_wrapper);

        this->actual_value_index = this->selected_value_index = midi_output_wrapper_manager->find_index(initial_selected_output_wrapper->label);
    }*/

    virtual const char* get_label_for_index(int index) {
        if (selected_source_index==-1) {    // select from the sources
            return midi_matrix_manager->sources[index].handle;
        } else {    // select from targets
            return midi_matrix_manager->targets[index].handle;
        }
        //Serial.printf("MidiOutputSelectorControl->get_label_for_index(%i)..\n", index); Serial.flush();
        //Serial.printf("got label %s..\n", available_outputs[index].label); Serial.flush();
        //return available_outputs[index].label;
        //return midi_output_wrapper_manager->get_label_for_index(index);
    }

    virtual void setter (int new_value) override {
        Serial.printf("MidiMatrixSelectorControl changing from %i to %i\n", this->actual_value_index, new_value); Serial.flush();
        if (selected_source_index==-1) { // select source
            selected_source_index = new_value;
        } else {
            // target selected for this source
            midi_matrix_manager->toggle_source_target(selected_source_index, new_value);
        }
        actual_value_index = new_value;
        selected_value_index = actual_value_index;
        /*if (this->setter_func!=nullptr) {
            Serial.printf("setting new output to number %i\n", new_value); Serial.flush();
            //this->setter_func(&available_outputs[new_value]);
            this->setter_func(midi_output_wrapper_manager->find(new_value));
        }*/
    }
    virtual int getter () override {
        //if (selected_source_index==-1) // select source
        return selected_value_index;
    }

    virtual int get_num_available() {
        if (selected_source_index==-1) 
            return midi_matrix_manager->sources_count;
        else
            return midi_matrix_manager->targets_count;
    }

    // classic fixed display version
    virtual int display(Coord pos, bool selected, bool opened) override {
        //Serial.println("MidiOutputSelectorControl display()!");

        pos.y = header(label, pos, selected, opened);
        tft->setTextSize(2);

        //num_values = NUM_AVAILABLE_OUTPUTS;
        num_values = this->get_num_available();

        tft->setTextSize(1);

        if (!opened) {
            // not selected, so just show the current values
            //colours(opened && selected_value_index==i, col, BLACK);
            /*char *label = (char*)get_label_for_index(this->actual_value_index);
            if (label==nullptr || this->actual_value_index == -1) {
                tft->setTextColor(RED, BLACK);
                label = (char*)"[invalid]";
            } else {
                tft->setTextColor(YELLOW, BLACK);
            }
            tft->printf((char*)"%s", (char*)label);
            tft->println((char*)"");*/
            tft->setTextColor(C_WHITE, BLACK);
            tft->printf("not opened");
        } else {
            int current_value = actual_value_index; //this->getter();

            if (selected_source_index==-1) { // show list of sources
                for (int i = 0 ; i < midi_matrix_manager->sources_count ; i++) {
                    bool is_current_value_selected = i==current_value;
                    int col = is_current_value_selected ? GREEN : C_WHITE;
                    colours(opened && selected_value_index==i, col, BLACK);
                    tft->printf((char*)"%s\n", (char*)get_label_for_index(i));
                }
            } else {        // show list of targets
                for (int i = 0 ; i < midi_matrix_manager->targets_count ; i++) {
                    bool is_current_value_selected = i==current_value;
                    bool is_current_value_connected = midi_matrix_manager->source_to_targets[selected_source_index][i];
                    int col = (is_current_value_connected && is_current_value_selected) ? PURPLE :
                              is_current_value_selected ? YELLOW : 
                              is_current_value_connected ? GREEN :
                              C_WHITE;
                    colours(opened && selected_value_index==i, col, BLACK);
                    tft->printf((char*)"%s\n", (char*)get_label_for_index(i));
                }
            }

            /*for (int i = 0 ; i < num_values ; i++) {
                bool is_current_value_selected = i==current_value;
                int col = is_current_value_selected ? GREEN : C_WHITE;
                colours(opened && selected_value_index==i, col, BLACK);
                tft->printf((char*)"%s\n", (char*)get_label_for_index(i));
                //tft->setTextColor(BLACK,BLACK);
            }*/
            if (tft->getCursorX()>0) // if we haven't wrapped onto next line then do it manually
                tft->println((char*)"");
        }
        return tft->getCursorY();
    }

    virtual bool button_select() override {
        //Serial.printf("button_select with selected_value_index %i\n", selected_value_index);
        //Serial.printf("that is available_values[%i] of %i\n", selected_value_index, available_values[selected_value_index]);
        this->setter(selected_value_index);

        char msg[255];
        //Serial.printf("about to build msg string...\n");
        sprintf(msg, "Selected %s to %s (%i)", label, get_label_for_index(selected_value_index), selected_value_index);
        //Serial.printf("about to set_last_message!");
        msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);
        return false;
    }

    virtual bool button_back() override {
        if (selected_source_index >= 0) {
            Serial.println("Backing out from selecting target");
            menu_set_last_message("Back out", GREEN);
            selected_source_index = -1;
            return true;    // don't exit to top menu
        }
        return false;       // exit to top menu
    }

};

#endif
