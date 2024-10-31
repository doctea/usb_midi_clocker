#include "Config.h"

#ifdef ENABLE_USB

#include "behaviours/behaviour_base_usb.h"

#ifdef ENABLE_SCREEN
    #include "menuitems_lambda.h"

    FLASHMEM
    LinkedList<MenuItem*> *DeviceBehaviourUSBBase::make_menu_items_device() {
        /*String midi_info = "[MIDI DIN device]";
        if (this->transmits_midi_notes() || this->receives_midi_notes()) {
            midi_info = (receives_midi_notes() ? "MIDI in: "    + String(this->input_midi_number+1)    + " "    : "") + 
                        (transmits_midi_notes()? "MIDI out: "   + String(this->output_midi_number+1)            : " ");
        }
        this->menuitems->add(new FixedSizeMenuItem(midi_info.c_str(), 0));*/
        this->menuitems->add(new CallbackMenuItem("Connection info", 
            [=]() -> const char* {
                static const char *connected_string = "USB: Connected";
                static const char *disconnected_string = "USB: Disconnected";
                //char connection_info[MENU_C_MAX];
                //snprintf(connection_info, MENU_C_MAX, "USB: %s", this->is_connected() ? "Connected" : "Disconnected");
                //return connection_info;
                return this->is_connected() ? connected_string : disconnected_string;
            }
        ));
        
        return this->menuitems;
    }    
#endif

#endif