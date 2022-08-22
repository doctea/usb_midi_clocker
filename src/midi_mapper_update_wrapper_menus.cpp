#include "Config.h"

#include "mymenu.h"
#include "menuitems.h"
#include "menu_midi_mapper.h"

/*#ifdef ENABLE_BEATSTEP
    extern MidiOutputSelectorControl beatstep_output_selector;
#endif
#ifdef ENABLE_KEYSTEP
    extern MidiOutputSelectorControl keystep_output_selector;
#endif
#ifdef ENABLE_MPK49
    extern MidiOutputSelectorControl mpk49_output_selector;
#endif
#ifdef ENABLE_LESTRUM
    extern MidiOutputSelectorControl lestrum_pads_output_selector;
    extern MidiOutputSelectorControl lestrum_arp_output_selector;
#endif
#ifdef ENABLE_LOOPER
    extern MidiOutputSelectorControl looper_output_selector;
#endif
extern MidiOutputSelectorControl pc_usb_input_1_selector;
extern MidiOutputSelectorControl pc_usb_input_2_selector;*/

/*
//TODO: move this into MIDIOutputWrapperManager...
void update_wrapper_menus_for_name(String source_label, int index) {
    Serial.printf("update_wrapper_menus_for_name(%s, %i)\n", source_label.c_str(), index);
    //MIDIOutputWrapper *target = find_wrapper_for_name((char*)target_label.c_str());
    //int index = find_wrapper_index_for_label((char*)target_label.c_str());
    #ifdef ENABLE_BEATSTEP
        if (source_label.equals("beatstep_output")) {
            //beatstep_setOutputWrapper(target);
            beatstep_output_selector.actual_value_index = index;
            beatstep_output_selector.selected_value_index = index;
            return;
        } 
    #endif
    #ifdef ENABLE_KEYSTEP
        if (source_label.equals("keystep_output")) {
            //keystep_setOutputWrapper(target);
            keystep_output_selector.actual_value_index = index;
            keystep_output_selector.selected_value_index = index;
            return;
        } 
    #endif
    #ifdef ENABLE_MPK49
        if (source_label.equals("mpk49_output")) {
            //mpk49_setOutputWrapper(target);
            mpk49_output_selector.actual_value_index = index;
            mpk49_output_selector.selected_value_index = index;
            return;
        } 
    #endif
    #ifdef ENABLE_LESTRUM
        if (source_label.equals("lestrum_pads_output")) {
            //lestrum_pads_setOutputWrapper(target);
            lestrum_pads_output_selector.actual_value_index = index;
            lestrum_pads_output_selector.selected_value_index = index;
            return;
        } else if (source_label.equals("lestrum_arp_output")) {
            //lestrum_arp_setOutputWrapper(target);
            lestrum_arp_output_selector.actual_value_index = index;
            lestrum_arp_output_selector.selected_value_index = index;
            return;
        }
    #endif
    #ifdef ENABLE_LOOPER
        if (source_label.equals("mpk49_loop_track")) {
            Serial.printf("!!!! source_label matches 'mpk49_loop_track', so setting looper_output_selector index to %i\n", index);
            Serial.printf("!!!! actual_value_index is %i and selected_value_index is %i - replacing with %i\n", looper_output_selector.actual_value_index, looper_output_selector.selected_value_index, index);
            looper_output_selector.actual_value_index = index;
            looper_output_selector.selected_value_index = index;
            return;
        }
    #endif
    if (source_label.equals("pc_usb_1_output")) {
        //pc_usb_1_setOutputWrapper(target);
        pc_usb_input_1_selector.actual_value_index = index;
        pc_usb_input_1_selector.selected_value_index = index;
        return;
    } else if (source_label.equals("pc_usb_2_output")) {
        //pc_usb_2_setOutputWrapper(target);
        pc_usb_input_2_selector.actual_value_index = index;
        pc_usb_input_2_selector.selected_value_index = index;
        return;
    }

    Serial.printf("update_wrapper_menus_for_name: Couldn't match source_label '%s' with anything!\n", source_label.c_str());
}
*/