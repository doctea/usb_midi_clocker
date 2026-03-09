#ifndef MENU_MIDI_MATRIX__INCLUDED
#define MENU_MIDI_MATRIX__INCLUDED

#include "Config.h"
#include "midi/midi_outs.h"
#include "midi/midi_out_wrapper.h"
#include "midi/midi_mapper_matrix_manager.h"

#include "menuitems.h"

class MidiMatrixSelectorControl : /*virtual*/ public SelectorControl<int> {
    //void (*setter_func)(MIDIOutputWrapper *midi_output);
    //MIDIOutputWrapper *initial_selected_output_wrapper = nullptr;

    // https://ankiewicz.com/color/hex/00cccc <- colour palette picker
    const uint16_t target_colours[MAX_NUM_TARGETS] = {
        0xF800,        //#define ST77XX_RED        
        0xDCDD,        //#define ST77XX_GREEN      
        0x0679,        //#define ST77XX_BLUE       
        0x07FF,        //#define ST77XX_CYAN       
        0xF81F,        //#define ST77XX_MAGENTA   
        0xFFE0,        //#define ST77XX_YELLOW     
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
        0xCE60,     // yellowy
        0xCF1D,     // bluey
        0xA578,     // grey-blue
        0x9E66,     // yellow-green
        0xA670,     // nicey green
        0x8351,     // purpley
    }; 

    uint16_t get_colour_for_target_id(target_id_t target_id) {
        return target_colours[target_id % (sizeof(target_colours)/sizeof(target_colours[0]))];
    }

    public:
    int actual_value_index;
    int selected_source_index = -1;

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
        Serial_printf("MidiMatrixSelectorControl changing from %i to %i\n", this->actual_value_index, new_value); Serial_flush();
        if (selected_source_index==-1) { // select source
            selected_source_index = new_value;
            actual_value_index = new_value;
            selected_value_index = actual_value_index;
            if (selected_value_index>=midi_matrix_manager->targets_count) {
                selected_value_index = midi_matrix_manager->targets_count-1;
            }
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

        int available_rows = (tft->height() - pos.y) / tft->getRowHeight();

        // draw the selected target's currently held + last note
        if (opened_on_target) {
            //tft->setCursor(0, tft->height()-30);

            // NOTE: we crashed here during initial testing, but can't seem to reproduce it since
            tft->setTextColor(this->get_colour_for_target_id(selected_value_index), BLACK);
            tft->printf("Current: %3s   Last: %3s\n", 
                (char*)get_note_name_c(midi_matrix_manager->get_target_for_id(selected_value_index)->current_note, midi_matrix_manager->get_target_for_id(selected_value_index)->default_channel),
                (char*)get_note_name_c(midi_matrix_manager->get_target_for_id(selected_value_index)->last_note, midi_matrix_manager->get_target_for_id(selected_value_index)->default_channel)
            );
            pos.y = tft->getCursorY();
        }


        // for remembering the lowest we go on screen
        int lowest_y = pos.y;

        uint16_t halfbright_white = tft->halfbright_565(C_WHITE);
        // render MIDI sources (left-hand column)
        //Serial.printf("starting sources loop render at scroll_offset %i because selected_value_index is %i and rows_available is %i\n", scroll_offset, selected_value_index, rows_available);
        for (source_id_t source_id = 0 ; source_id < midi_matrix_manager->sources_count ; source_id++) {
            source_id_t source_id_actual = (source_id + relevant_source_id + (available_rows*2)) % midi_matrix_manager->sources_count;
            //source_id_t source_id_actual = source_id;
            const bool is_current_value_selected = source_id_actual==current_value;

            // in 'select target' mode, the currently selected source is highlighted in green
            int col = !opened || opened_on_source ? C_WHITE : halfbright_white;
            if (opened_on_target && is_current_value_selected)
                // in 'select target' mode, the currently selected source is highlighted in green
                col = GREEN;
            else if (opened_on_target && midi_matrix_manager->is_connected(source_id_actual, selected_value_index))
                // in 'select target' mode, sources that are connected to the currently highlighted target are highlighted, even if they're not the currently selected source
                col = this->get_colour_for_target_id(selected_value_index);     // C_WHITE

            colours(
                (opened && selected_source_index==-1 && selected_value_index==source_id_actual) ||
                (opened_on_target && is_current_value_selected), 
                col, 
                BLACK
            );
            tft->printf("%13s", (char*)midi_matrix_manager->get_label_for_source_id(source_id_actual));
            tft->println();
            source_position[source_id_actual] = tft->getCursorY() - (tft->getRowHeight()/2);
        }
        lowest_y = tft->getCursorY();

        // position cursor ready to draw targets
        tft->setCursor(0, pos.y);

        target_id_t selected_target_index = selected_source_index>=0 ? selected_value_index : 0; // only show selected target index if we're in 'select target' mode

        int y = pos.y;
        // render target MIDI (right-hand column)
        for (target_id_t target_id = 0 ; target_id < midi_matrix_manager->targets_count ; target_id++) {
            target_id_t target_id_actual = (target_id + selected_target_index + (available_rows*2)) % midi_matrix_manager->targets_count;

            const uint16_t target_colour = this->get_colour_for_target_id(target_id_actual);
            const uint16_t half_target_colour = tft->halfbright_565(target_colour);
            const bool opened_and_current_target_is_connected = (opened_on_source || opened_on_target) && midi_matrix_manager->is_connected(relevant_source_id, target_id_actual);

            bool opened_and_selected_target_is_current = opened_on_target && selected_value_index==target_id_actual;

            colours(
                opened_and_selected_target_is_current,
                !opened || opened_and_current_target_is_connected || opened_and_selected_target_is_current? target_colour : half_target_colour, 
                BLACK
            ); 
            
            tft->setCursor((tft->width()/2), y);
            //tft->printf((char*)"%c %1x %-23s", indicator, (int)target_id, (char*)get_label_for_index(target_id));
            tft->printf("%-19.19s", (char*)midi_matrix_manager->get_label_for_target_id(target_id_actual));

            target_position[target_id_actual] = (tft->getCursorY());// + (tft->getRowHeight()/2); // + tft->getRowHeight()) + (tft->getRowHeight()/2);
            
            tft->println();
            //Serial.printf("target_id %i at y %i is '%s' with target_colour=%02x\n", target_id_actual, target_position[target_id_actual], (char*)midi_matrix_manager->get_label_for_target_id(target_id_actual), target_colour);

            // draw the lines connecting sources+targets
            for (source_id_t source_id = 0 ; source_id < midi_matrix_manager->sources_count ; source_id++) {
                const source_id_t source_id_actual = (source_id + relevant_source_id + (available_rows*2)) % midi_matrix_manager->sources_count;

                // Determine line colour using explicit, named conditions for clarity
                const bool connection_exists = midi_matrix_manager->is_connected(source_id_actual, target_id_actual);
                const bool is_opened_target_and_relevant = opened_on_target && relevant_source_id == source_id_actual;

                uint16_t line_colour;
                if (is_opened_target_and_relevant && connection_exists) {
                    // When viewing a target and this source is the relevant one and connected, highlight in GREEN
                    line_colour = GREEN;
                } else {
                    // Otherwise decide between full target colour or half-bright depending on open/selection state
                    const bool use_full_target_colour = !opened || (
                        (opened_on_source && relevant_source_id == source_id_actual) ||
                        (opened_on_target && selected_value_index == target_id_actual)
                    );
                    line_colour = use_full_target_colour ? target_colour : half_target_colour;
                }

                if (midi_matrix_manager->is_connected(source_id_actual, target_id_actual)) {
                    // calculate the offset from the label positions to draw the connection line; take into account how many other connections there are (by asking midi_matrix_manager), and how many we've already drawn (by tracking in *_processed_count arrays)
                    //int pixels_per_source = constrain(source_processed_count[source_id] * (tft->getRowHeight() / midi_matrix_manager->connected_to_source_count(source_id)), 1, tft->getRowHeight());
                    //int pixels_per_target = constrain(target_processed_count[target_id] * (tft->getRowHeight() / midi_matrix_manager->connected_to_target_count(target_id)), 1, tft->getRowHeight());
                    int pixels_per_source = source_processed_count[source_id_actual] % tft->getRowHeight();
                    int pixels_per_target = target_processed_count[target_id_actual] % tft->getRowHeight();

                    int source_y_offset = pixels_per_source + (tft->getRowHeight()/4);
                    int target_y_offset = pixels_per_target + (tft->getRowHeight()/4);
                    tft->drawLine(
                        tft->characterWidth() * 13,
                        source_position[source_id_actual] + source_y_offset, 
                        tft->characterWidth() * 14,
                        source_position[source_id_actual] + source_y_offset,  
                        line_colour
                    );
                    tft->drawLine(
                        ///*3 + */(tft->width()/3), 
                        tft->characterWidth() * 14,
                        source_position[source_id_actual] + source_y_offset, 
                        (tft->width()/2)-(tft->characterWidth()*2), 
                        target_position[target_id_actual] + target_y_offset, 
                        line_colour
                    );
                    tft->drawLine(
                        (tft->width()/2)-(tft->characterWidth()*2), 
                        target_position[target_id_actual] + target_y_offset,
                        (tft->width()/2)-(tft->characterWidth()*1), 
                        target_position[target_id_actual] + target_y_offset,
                        line_colour
                    );
                    // TODO: some kinda logic to ensure that we always stay within the source_position and target_position rows
                    //source_position[i]+=2;   // move the cursor 2 line downs to provide a little bit of disambiguation
                    //target_position[target_id]+=2;
                    /*y = tft->getCursorY();
                    tft->setCursor(0, tft->height()-30);
                    tft->printf("pixels_per_source = %i\npixels_per_target = %i", pixels_per_source, pixels_per_target);
                    tft->setCursor(0, y);*/

                    source_processed_count[source_id_actual]++;
                    target_processed_count[target_id_actual]++;
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
