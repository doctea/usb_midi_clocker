#include "behaviours/behaviour_base_serial.h"

const char *DeviceBehaviourSerialBase::get_indicator() {
    if (!indicator_done) {
        /*
        // alternate "I=x O=x" version, needs indicator_Text to be 10 characters instead
        String s = String(
            (receives_midi_notes()? "I="+String(this->input_midi_number+1):"   ") + " " +
            (transmits_midi_notes()?"O="+String(this->output_midi_number+1):"   ")
        );
        strncpy(this->indicator_text, s.c_str(), 10);*/
        String s = String(
            (receives_midi_notes() ? String(this->input_midi_number+1) :" ") + " " +
            (transmits_midi_notes()? String(this->output_midi_number+1):" ")
        );
        strncpy(this->indicator_text, s.c_str(), 5);
        indicator_done = true;
    }
    return this->indicator_text;
}

// add menuitems specific to the underlying device type (eg serial, usbserial, usbmidi, virtual)
//FLASHMEM
LinkedList<MenuItem*> *DeviceBehaviourSerialBase::make_menu_items_device() {
    String midi_info = "[MIDI DIN device]";
    if (this->transmits_midi_notes() || this->receives_midi_notes()) {
        midi_info = (receives_midi_notes() ? "MIDI in: "    + String(this->input_midi_number+1)    + " "    : "") + 
                    (transmits_midi_notes()? "MIDI out: "   + String(this->output_midi_number+1)            : " ");
    }
    this->menuitems->add(new FixedSizeMenuItem(midi_info.c_str(), 0));
    
    return this->menuitems;
}         
