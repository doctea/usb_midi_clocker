#ifndef MENU_MIDI_MAPPER__INCLUDED
#define MENU_MIDI_MAPPER__INCLUDED

#include "Config.h"
#include "midi_outs.h"
#include "midi_out_wrapper.h"
#include "midi_mapper.h"

#include "menuitems.h"

class MidiOutputSelectorControl : public SelectorControl {
    void (*setter_func)(MIDIOutputWrapper *midi_output);
    MIDIOutputWrapper *initial_selected_output_wrapper;

    public:
    int actual_value_index;

    MidiOutputSelectorControl(const char *label) : SelectorControl(label, 0) {};
/*        SelectorControl(label, 0) {
        //strcpy(this->label, label);
        this->setter_func = setter_func;
        //this->initial_selected_output_wrapper = initial_selected_output_wrapper;
    }*/

    virtual void configure (MIDIOutputWrapper *initial_selected_output_wrapper, void (*setter_func)(MIDIOutputWrapper*)) {
        this->initial_selected_output_wrapper = initial_selected_output_wrapper;
        this->setter_func = setter_func;
        Serial.printf("configured %s @ ", initial_selected_output_wrapper->label);
        Serial.printf("%u and %u\n", this->initial_selected_output_wrapper, this->setter_func);
    }

    virtual void on_add() {
        actual_value_index = -1;
        Serial.println("on_add()");
        Serial.printf("MidiOutputSelectorControl@ %u...\n", initial_selected_output_wrapper);
        Serial.printf("MidiOutputSelectorControl looking for '%s' at %u...\n", initial_selected_output_wrapper->label, *initial_selected_output_wrapper);

        this->actual_value_index = find_wrapper_index_for_label(initial_selected_output_wrapper->label);
        this->selected_value_index = this->actual_value_index;

        /*for (unsigned int i = 0 ; i < NUM_AVAILABLE_OUTPUTS ; i++) {
            Serial.printf("Looping over %s at %u\n", available_outputs[i].label, &available_outputs[i]);
            //if (&(*available_outputs[i])==initial_selected_output_wrapper) {
            //if (available_outputs[i].same_as(initial_selected_output_wrapper)) {
            if (strcmp(available_outputs[i].label, initial_selected_output_wrapper->label)) {   // todo: compare by pointer instead of string?
                Serial.printf("MidiOutputSelectorControl() found output index %i matches passed wrapper\n", i);
                actual_value_index = i;
            }
        }
        if (actual_value_index==-1) {
            while(1);
                Serial.printf("Didn't find a match for %u aka '%s'\n", initial_selected_output_wrapper, initial_selected_output_wrapper->label);
        }*/
    }

    virtual const char* get_label_for_index(int index) {
        //Serial.printf("MidiOutputSelectorControl->get_label_for_index(%i)..\n", index); Serial.flush();
        //Serial.printf("got label %s..\n", available_outputs[index].label); Serial.flush();
        return available_outputs[index].label;
    }

    virtual void setter (int new_value) {
        Serial.printf("MidiOutputSelectorControl changing from %i to %i\n", this->actual_value_index, new_value);
        actual_value_index = new_value;
        selected_value_index = actual_value_index;
        if (this->setter_func!=nullptr) {
            Serial.printf("setting new output\n");
            this->setter_func(&available_outputs[new_value]);
        }
    }
    virtual int getter () {
        return selected_value_index;
    }

    // classic fixed display version
    virtual int display(Coord pos, bool selected, bool opened) override {
        //Serial.println("MidiOutputSelectorControl display()!");

        pos.y = header(label, pos, selected, opened);
        tft->setTextSize(2);

        num_values = NUM_AVAILABLE_OUTPUTS;

        tft->setTextSize(1);

        if (!opened) {
            // not selected, so just show the current value
            //colours(opened && selected_value_index==i, col, BLACK);

            tft->printf((char*)"%s", (char*)get_label_for_index(selected_value_index));
            tft->println((char*)"");
        } else {
            int current_value = actual_value_index; //this->getter();

            for (int i = 0 ; i < num_values ; i++) {
                bool is_current_value_selected = i==current_value;
                int col = is_current_value_selected ? GREEN : C_WHITE;
                colours(opened && selected_value_index==i, col, BLACK);
                tft->printf((char*)"%s\n", (char*)get_label_for_index(i));
                //tft->setTextColor(BLACK,BLACK);
            }
            if (tft->getCursorX()>0) // if we haven't wrapped onto next line then do it manually
                tft->println((char*)"");
        }
        return tft->getCursorY();
    }

    virtual bool button_select() {
        //Serial.printf("button_select with selected_value_index %i\n", selected_value_index);
        //Serial.printf("that is available_values[%i] of %i\n", selected_value_index, available_values[selected_value_index]);
        this->setter(selected_value_index);

        char msg[255];
        //Serial.printf("about to build msg string...\n");
        sprintf(msg, "Set %s to %s (%i)", label, get_label_for_index(selected_value_index), selected_value_index);
        //Serial.printf("about to set_last_message!");
        msg[20] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);
        return false;
    }

};

#endif