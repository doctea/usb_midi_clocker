#include "Config.h"
#include "midi_out_wrapper.h"
#include "menuitems.h"

MIDIOutputWrapper available_outputs[6] = {
    MIDIOutputWrapper("S1 [bitbox ch3]",  midi_out_serial[0], 3),
    MIDIOutputWrapper("S2 [unused ch1]",  midi_out_serial[1], 1),
    MIDIOutputWrapper("S3 [Neutron ch4]", midi_out_serial[2], 4),
    MIDIOutputWrapper("S4 [Disting ch1]", midi_out_serial[3], 1),
    /*MIDIOutputWrapper("Serial 4 [unused ch1]", midi_out_serial[3], 1),
    MIDIOutputWrapper("Serial 5 [unused ch1]", midi_out_serial[4], 1),
    MIDIOutputWrapper("Serial 6 [unused ch1]", midi_out_serial[5], 1),*/
    MIDIOutputWrapper("USB Bamble [ch1]", midi_bamble, 1),
    MIDIOutputWrapper("USB Bamble [ch2]", midi_bamble, 2),
};

void mpk49_loop_track_setOutputWrapper(MIDIOutputWrapper *wrapper) {
    mpk49_loop_track.setOutputWrapper(wrapper);
}

class MidiOutputSelectorControl : public SelectorControl {
    int actual_value_index;
    void (*setter_func)(MIDIOutputWrapper *midi_output);

    public:

    MidiOutputSelectorControl(const char *label, MIDIOutputWrapper *initial_selected_output_wrapper, void (*setter_func)(MIDIOutputWrapper*)) :
        SelectorControl(label, 0) {
        strcpy(this->label, label);
        this->setter_func = setter_func;
        actual_value_index = -1;
        Serial.printf("MidiOutputSelectorControl looking for %s...\n", initial_selected_output_wrapper->label);
        for (int i = 0 ; i < sizeof(available_outputs) ; i++) {
            //Serial.printf("Looping over %s at %u\n", available_outputs[i].label, available_outputs[i]);
            if (&available_outputs[i]==initial_selected_output_wrapper) {
                //Serial.printf("MidiOutputSelectorControl() found output index %i matches passed wrapper\n", i);
                actual_value_index = i;
            }
        }
        if (actual_value_index==-1) {
            //Serial.printf("Didn't find a match for %u aka '%s'\n", initial_selected_output_wrapper, initial_selected_output_wrapper->label);
            while(1);
        }
    }

    virtual const char* get_label_for_index(int index) {
        Serial.printf("MidiOutputSelectorControl->get_label_for_index(%i)..\n", index); Serial.flush();
        Serial.printf("got label %s..\n", available_outputs[index].label); Serial.flush();
        return available_outputs[index].label;
    }

    virtual void setter (int new_value) {
        Serial.printf("MidiOutputSelectorControl changing from %i to %i", this->actual_value_index, new_value);
        if (this->setter_func!=nullptr)
            this->setter_func(&available_outputs[new_value]);
    }
    virtual int getter () {
        return selected_value_index;
    }

    // classic fixed display version
    virtual int display(Coord pos, bool selected, bool opened) override {
        Serial.println("MidiOutputSelectorControl display()!");

        pos.y = header(label, pos, selected, opened);
        tft->setTextSize(2);

        num_values = sizeof(available_outputs);

        if (!selected) {
            // not selected, so just show the current value
            //colours(opened && selected_value_index==i, col, BLACK);

            tft->printf("%s", (char*)get_label_for_index(selected_value_index));
            tft->println("");
        } else {
            int current_value = this->getter();

            for (int i = 0 ; i < num_values ; i++) {
                bool is_current_value_selected = i==current_value;
                int col = is_current_value_selected ? GREEN : C_WHITE;
                colours(opened && selected_value_index==i, col, BLACK);
                tft->printf("%s\n", (char*)get_label_for_index(i));
                tft->setTextColor(BLACK,BLACK);
            }
            if (tft->getCursorX()>0) // if we haven't wrapped onto next line then do it manually
                tft->println("");
            return tft->getCursorY();
        }
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