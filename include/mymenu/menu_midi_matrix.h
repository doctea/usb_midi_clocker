#ifndef MENU_MIDI_MATRIX__INCLUDED
#define MENU_MIDI_MATRIX__INCLUDED

#include "Config.h"
#include "midi/midi_outs.h"
#include "midi/midi_out_wrapper.h"
#include "midi/midi_mapper_matrix_manager.h"

#include "menuitems.h"

class MidiMatrixSelectorControl : public SelectorControl<int> {
    //void (*setter_func)(MIDIOutputWrapper *midi_output);
    //MIDIOutputWrapper *initial_selected_output_wrapper = nullptr;

    const uint16_t target_colours[MAX_NUM_TARGETS] = {
        0xF800, //#define ST77XX_RED        
        0x07E0,        //#define ST77XX_GREEN      
        0x001F,        //#define ST77XX_BLUE       
        0x07FF,        //#define ST77XX_CYAN       
        0xF81F,        //#define ST77XX_MAGENTA   
        0xFFE0,//#define ST77XX_YELLOW     
        0xFC00,        //#define ST77XX_ORANGE     
        0xF710,        //#define ST77XX_PINK       250 + 250 + 
        (0xF800 + 0x001F)/2,
        (0x07E0 + 0x07FF)/2,
        (0xF81F + 0xFC00)/2,
        (0xFA1F + 0xF800)/2,
        (0x07E0 + 0x001F)/2,
        (0xF800 + 0x07FF)/2,
        (0xF81F + 0xFC00)/2,
        (0xA81F + 0xF877)/2,
        (0x07E0 + 0xFC00)/2,
        (0xF710 + 0xFC00)/2,
        (0xF710 + 0xF877)/2,
        (0xF800 + 0xFC00)/2,
        (0x07E0 + 0x001F)/2,
        (0xF81F + 0xFC00)/3,
        250 + (0x07E0 + 0x001F)/3,
        250 + (0xF800 + 0x07FF)/3,
    }; 

    uint16_t get_colour_for_target_id(target_id_t target_id) {
        return target_colours[target_id];
    }

    public:
    int actual_value_index;
    int selected_source_index = -1;
    int selected_target_index = -1;

    MidiMatrixSelectorControl(const char *label) : SelectorControl(label, 0) {};

    virtual const char* get_label_for_index(int index) {
        if (selected_source_index==-1) {    
            // select from the sources
            return midi_matrix_manager->get_label_for_source_id(index); //->sources[index].handle;
        } else {    
            // select from targets
            return midi_matrix_manager->get_label_for_target_id(index); //targets[index].handle;
        }
    }

    virtual void setter (int new_value) override {
        Serial.printf("MidiMatrixSelectorControl changing from %i to %i\n", this->actual_value_index, new_value); Serial_flush();
        if (selected_source_index==-1) { // select source
            selected_source_index = new_value;
            actual_value_index = new_value;
            selected_value_index = actual_value_index;
        } else {
            // toggle selected for this source + target combo
            char msg[MENU_MESSAGE_MAX];
            if (midi_matrix_manager->toggle_connect(selected_source_index, new_value)) {
                //Serial.printf("about to build msg string...\n");
                //snprintf(msg, MENU_MESSAGE_MAX, "Connected %s to %s (%i)", label, get_label_for_index(selected_value_index), selected_value_index);
                //snprintf(msg, MENU_MESSAGE_MAX, "Connected %10s to %10s", midi_matrix_manager->get_label_for_source_id(selected_source_index), midi_matrix_manager->get_label_for_target_id(new_value));
                snprintf(msg, MENU_MESSAGE_MAX, "%-5.18s <-> %-5.18s", midi_matrix_manager->get_label_for_source_id(selected_source_index), midi_matrix_manager->get_label_for_target_id(new_value));
            } else {
                //snprintf(msg, MENU_MESSAGE_MAX, "Disconnected %10s from %10s", midi_matrix_manager->get_label_for_source_id(selected_source_index), midi_matrix_manager->get_label_for_target_id(new_value));
                snprintf(msg, MENU_MESSAGE_MAX, "%-5.18s </> %-5.18s", midi_matrix_manager->get_label_for_source_id(selected_source_index), midi_matrix_manager->get_label_for_target_id(new_value));
            }
            menu_set_last_message(msg, GREEN);
        }
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
        //unsigned long time_micros = micros();

        pos.y = header(label, pos, selected, opened);
        //tft->setTextSize(2);

        if (selected_source_index == -1) {
            tft->setCursor(pos.x + (18*6), pos.y - 8);
            for(int i = 0 ; i < midi_matrix_manager->targets_count ; i++) {
                colours(selected, this->get_colour_for_target_id(i), BLACK);
                tft->printf("%1x", i);
            }
            tft->println();
        }

        //num_values = NUM_AVAILABLE_OUTPUTS;
        num_values = this->get_num_available();

        tft->setTextSize(1);

        int current_value = actual_value_index; //this->getter();

        if (selected_source_index==-1) { // show list of sources
            for (source_id_t source_id = 0 ; source_id < midi_matrix_manager->sources_count ; source_id++) {
                const bool is_current_value_selected = source_id==current_value;
                const int col = is_current_value_selected ? GREEN : C_WHITE;
                colours(opened && selected_value_index==source_id, col, BLACK);
                tft->printf("%15s : ", (char*)get_label_for_index(source_id));

                for (target_id_t target_id = 0 ; target_id < midi_matrix_manager->targets_count ; target_id++) {
                    colours(BLACK, this->get_colour_for_target_id(target_id));
                    if (midi_matrix_manager->is_connected(source_id, target_id)) {
                        //tft->printf((char*)"%15s, ", (char*)midi_matrix_manager->get_label_for_target_id(target_id));
                        tft->printf("%1x", target_id);
                    } else {
                        tft->printf(" ");
                    }
                }
                tft->println();
            }
        } else {        // show list of targets
            tft->setTextColor(BLACK,GREEN);
            tft->printf("%s outputs to..\n", (char*)midi_matrix_manager->get_label_for_source_id(selected_source_index));
            for (target_id_t target_id = 0 ; target_id < midi_matrix_manager->targets_count ; target_id++) {
                //bool is_current_value_selected = target_id==current_value;
                const bool is_current_value_connected = midi_matrix_manager->is_connected(selected_source_index, target_id);
                /*int col = (is_current_value_connected && is_current_value_selected) ? PURPLE :
                            is_current_value_connected ? GREEN :
                            C_WHITE;*/
                const uint16_t col = this->get_colour_for_target_id(target_id);
                colours(opened && selected_value_index==target_id, col, BLACK); //this->get_colour_for_target_id(target_id), BLACK);
                const char indicator = is_current_value_connected ? '*' : ' ';
                tft->printf((char*)"%c %1x %-23s", indicator, (int)target_id, (char*)get_label_for_index(target_id));

                tft->printf("  %3s:%3s", 
                    (char*)get_note_name_c(midi_matrix_manager->get_target_for_id(target_id)->current_note),
                    (char*)get_note_name_c(midi_matrix_manager->get_target_for_id(target_id)->last_note)
                );
                tft->println();
            }
        }

        if (tft->getCursorX()>0) // if we haven't wrapped onto next line then do it manually
            tft->println((char*)"");

        return tft->getCursorY();
    }

    virtual bool button_select() override {
        //Serial.printf("button_select with selected_value_index %i\n", selected_value_index);
        //Serial.printf("that is available_values[%i] of %i\n", selected_value_index, available_values[selected_value_index]);
        this->setter(selected_value_index);

        /*char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        snprintf(msg, MENU_MESSAGE_MAX, "Selected %s to %s (%i)", label, get_label_for_index(selected_value_index), selected_value_index);
        //Serial.printf("about to set_last_message!");
        //msg[MENU_MESSAGE_MAX-1] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg, GREEN);*/
        return go_back_on_select;
    }

    virtual bool button_back() override {
        if (selected_source_index >= 0) {
            //Serial.println("Backing out from selecting target");
            //menu_set_last_message((const char*)"Back out", GREEN);
            selected_source_index = -1;
            return true;    // don't exit to top menu
        }
        return false;       // exit to top menu
    }

};

#endif
