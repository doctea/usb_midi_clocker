#include "behaviours/behaviour_base_serial.h"

const char *DeviceBehaviourSerialBase::get_indicator() {
    if (!indicator_done) {
        /*
        // alternate "I=x O=x" version, needs indicator_Text to be 10 characters instead
        String s = String(
            (has_input()? "I="+String(this->input_midi_number+1):"   ") + " " +
            (has_output()?"O="+String(this->output_midi_number+1):"   ")
        );
        strncpy(this->indicator_text, s.c_str(), 10);*/
        String s = String(
            (has_input() ? String(this->input_midi_number+1) :" ") + " " +
            (has_output()? String(this->output_midi_number+1):" ")
        );
        strncpy(this->indicator_text, s.c_str(), 5);
        indicator_done = true;
    }
    return this->indicator_text;
}