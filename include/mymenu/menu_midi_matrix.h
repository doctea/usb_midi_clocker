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

        byte source_processed_count[midi_matrix_manager->sources_count];
        byte target_processed_count[midi_matrix_manager->targets_count];
        memset(source_processed_count, 0, midi_matrix_manager->sources_count);
        memset(target_processed_count, 0, midi_matrix_manager->targets_count);

        bool opened_on_source = opened && selected_source_index==-1;
        bool opened_on_target = opened && selected_source_index>=0;
        const source_id_t relevant_source_id = opened_on_source ? selected_value_index : selected_source_index;

        // for remembering the lowest we go on screen
        int lowest_y = pos.y;

        // render MIDI sources (left-hand column)
        for (source_id_t source_id = 0 ; source_id < midi_matrix_manager->sources_count ; source_id++) {
            const bool is_current_value_selected = source_id==current_value;
            int col = is_current_value_selected ? GREEN : C_WHITE;
            if (opened_on_target && !is_current_value_selected) 
                col = tft->halfbright_565(col);
            colours(opened && selected_source_index==-1 && selected_value_index==source_id, col, BLACK);
            tft->printf("%13s", (char*)midi_matrix_manager->get_label_for_source_id(source_id));
            tft->println();
            source_position[source_id] = tft->getCursorY() - (tft->getRowHeight()/2);
        }
        lowest_y = tft->getCursorY();

        // position cursor ready to draw targets
        tft->setCursor(0, pos.y);

        // draw the selected target's currently held + last note
        if (opened_on_target) {
            tft->setCursor(0, tft->height()-30);
            tft->setTextColor(this->get_colour_for_target_id(selected_value_index), BLACK);
            tft->printf("  %3s:%3s", 
                (char*)get_note_name_c(midi_matrix_manager->get_target_for_id(selected_value_index)->current_note),
                (char*)get_note_name_c(midi_matrix_manager->get_target_for_id(selected_value_index)->last_note)
            );
        }

        int y = pos.y;
        // render target MIDI (right-hand column)
        for (target_id_t target_id = 0 ; target_id < midi_matrix_manager->targets_count ; target_id++) {

            const uint16_t target_colour = this->get_colour_for_target_id(target_id);
            const uint16_t half_target_colour = tft->halfbright_565(target_colour);
            const bool opened_and_current_target_is_connected = (opened_on_source || opened_on_target) && midi_matrix_manager->is_connected(relevant_source_id, target_id);

            bool opened_and_selected_target_is_current = opened_on_target && selected_value_index==target_id;

            colours(
                opened_and_selected_target_is_current,
                !opened || opened_and_current_target_is_connected ? target_colour : half_target_colour, 
                BLACK
            ); 
            
            tft->setCursor((tft->width()/2), y);
            //tft->printf((char*)"%c %1x %-23s", indicator, (int)target_id, (char*)get_label_for_index(target_id));
            tft->printf("%-19.19s", (char*)midi_matrix_manager->get_label_for_target_id(target_id));

            target_position[target_id] = (tft->getCursorY());// + (tft->getRowHeight()/2); // + tft->getRowHeight()) + (tft->getRowHeight()/2);
            
            tft->println();

            // draw the lines connecting sources+targets
            for (source_id_t source_id = 0 ; source_id < midi_matrix_manager->sources_count ; source_id++) {
                const uint16_t line_colour = !opened || (opened && relevant_source_id == source_id) ? target_colour : half_target_colour;
                if (midi_matrix_manager->is_connected(source_id, target_id)) {
                    // calculate the offset from the label positions to draw the connection line; take into account how many other connections there are (by asking midi_matrix_manager), and how many we've already drawn (by tracking in *_processed_count arrays)
                    //int pixels_per_source = constrain(source_processed_count[source_id] * (tft->getRowHeight() / midi_matrix_manager->connected_to_source_count(source_id)), 1, tft->getRowHeight());
                    //int pixels_per_target = constrain(target_processed_count[target_id] * (tft->getRowHeight() / midi_matrix_manager->connected_to_target_count(target_id)), 1, tft->getRowHeight());
                    int pixels_per_source = source_processed_count[source_id] % tft->getRowHeight();
                    int pixels_per_target = target_processed_count[target_id] % tft->getRowHeight();

                    int source_y_offset = pixels_per_source;
                    int target_y_offset = pixels_per_target;
                    tft->drawLine(
                        tft->characterWidth() * 13,
                        source_position[source_id] + source_y_offset, 
                        tft->characterWidth() * 14,
                        source_position[source_id] + source_y_offset,  
                        line_colour
                    );
                    tft->drawLine(
                        ///*3 + */(tft->width()/3), 
                        tft->characterWidth() * 14,
                        source_position[source_id] + source_y_offset, 
                        (tft->width()/2)-(tft->characterWidth()*2), 
                        target_position[target_id] + target_y_offset, 
                        line_colour
                    );
                    tft->drawLine(
                        (tft->width()/2)-(tft->characterWidth()*2), 
                        target_position[target_id] + target_y_offset,
                        (tft->width()/2)-(tft->characterWidth()*1), 
                        target_position[target_id] + target_y_offset,
                        line_colour
                    );
                    // TODO: some kinda logic to ensure that we always stay within the source_position and target_position rows
                    //source_position[i]+=2;   // move the cursor 2 line downs to provide a little bit of disambiguation
                    //target_position[target_id]+=2;
                    /*y = tft->getCursorY();
                    tft->setCursor(0, tft->height()-30);
                    tft->printf("pixels_per_source = %i\npixels_per_target = %i", pixels_per_source, pixels_per_target);
                    tft->setCursor(0, y);*/

                    source_processed_count[source_id]++;
                    target_processed_count[target_id]++;
                }
            }
            y = tft->getCursorY();
        }
        if (y > lowest_y) 
            lowest_y = y;

        if (tft->getCursorX()>0) // if we haven't wrapped onto next line then do it manually
            tft->println((char*)"");

        return lowest_y; //tft->getCursorY();
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
            selected_value_index = selected_source_index;
            selected_source_index = -1;
            return true;    // don't exit to top menu
        }
        return false;       // exit to top menu
    }

};

#endif
