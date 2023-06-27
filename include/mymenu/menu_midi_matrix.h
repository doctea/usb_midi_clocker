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

        pos.y = header(label, pos, selected, opened);
        num_values = this->get_num_available();

        tft->setTextSize(1);

        int current_value = actual_value_index; //this->getter();

        int source_position[midi_matrix_manager->sources_count];
        int target_position[midi_matrix_manager->targets_count];

        for (source_id_t source_id = 0 ; source_id < midi_matrix_manager->sources_count ; source_id++) {
            const bool is_current_value_selected = source_id==current_value;
            const int col = is_current_value_selected ? GREEN : C_WHITE;
            colours(opened && selected_source_index==-1 && selected_value_index==source_id, col, BLACK);
            tft->printf("%13s", (char*)midi_matrix_manager->get_label_for_source_id(source_id));
            tft->println();
            source_position[source_id] = tft->getCursorY() - (tft->getRowHeight()/2);
        }

        tft->setCursor(0, pos.y);

        bool opened_source = opened && selected_source_index==-1;
        bool opened_target = opened && selected_source_index>=0;
        const source_id_t relevant_source_id = opened_source ? selected_value_index : selected_source_index;

        int y = pos.y;
        for (target_id_t target_id = 0 ; target_id < midi_matrix_manager->targets_count ; target_id++) {

            const uint16_t target_colour = this->get_colour_for_target_id(target_id);
            const uint16_t half_target_colour = tft->halfbright_565(target_colour);
            const bool opened_and_current_target_is_connected = (opened_source || opened_target) && midi_matrix_manager->is_connected(relevant_source_id, target_id);

            bool opened_and_selected_target_is_current = opened_target && selected_value_index==target_id;

            colours(
                opened_and_selected_target_is_current,
                !opened || opened_and_current_target_is_connected ? target_colour : half_target_colour, 
                BLACK
            ); 
            
            tft->setCursor((tft->width()/2), y);
            //tft->printf((char*)"%c %1x %-23s", indicator, (int)target_id, (char*)get_label_for_index(target_id));
            tft->printf("%-19.19s", (char*)midi_matrix_manager->get_label_for_target_id(target_id));

            // todo: restore the ability to show the current/last notes
            /*tft->printf("  %3s:%3s", 
                (char*)get_note_name_c(midi_matrix_manager->get_target_for_id(target_id)->current_note),
                (char*)get_note_name_c(midi_matrix_manager->get_target_for_id(target_id)->last_note)
            );*/

            target_position[target_id] = (tft->getCursorY()) + (tft->getRowHeight()/2); // + tft->getRowHeight()) + (tft->getRowHeight()/2);
            
            tft->println();
            
            for (source_id_t i = 0 ; i < midi_matrix_manager->sources_count ; i++) {
                const uint16_t line_colour = !opened || (opened && relevant_source_id == i) ? target_colour : half_target_colour;
                if (midi_matrix_manager->is_connected(i, target_id)) {
                    //TODO: use correct logic to avoid drawing fullbright colours for all sources connected to this target..
                    tft->drawLine(
                        tft->characterWidth() * 13,
                        source_position[i], 
                        tft->characterWidth() * 14,
                        source_position[i],  
                        line_colour
                    );
                    tft->drawLine(
                        ///*3 + */(tft->width()/3), 
                        tft->characterWidth() * 14,
                        source_position[i], 
                        (tft->width()/2)-(tft->characterWidth()*2), 
                        target_position[target_id], 
                        line_colour
                    );
                    tft->drawLine(
                        (tft->width()/2)-(tft->characterWidth()*2), 
                        target_position[target_id],
                        (tft->width()/2)-(tft->characterWidth()*1), 
                        target_position[target_id],
                        line_colour
                    );
                    // TODO: some kinda logic to ensure that we always stay within the source_position and target_position rows
                    source_position[i]+=2;   // move the cursor 2 line downs to provide a little bit of disambiguation
                    target_position[target_id]+=2;
                }
            }
            y = tft->getCursorY();
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
