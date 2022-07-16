#include "midi_mapper.h"

#include "behaviour_bamble.h"
#include "behaviour_craftsynth.h"

#include "midi_mapper_update_wrapper_menus.h"

// NOTE: you gotta be careful with the names of these things!
// if there is a used MIDIOutputWrapper somewhere that doesnt have a label that matches these strings EXACTLY, the midi_mapper menu item will crash and apparently 
// never tell us about it in console!!
// update NUM_AVAILABLE_OUTPUTS in midi_mapper.h
MIDIOutputWrapper available_outputs[NUM_AVAILABLE_OUTPUTS] = {
    MIDIOutputWrapper((char*)"S1 : Bitbox : ch 1",  midi_out_serial[0], 1),
    MIDIOutputWrapper((char*)"S1 : Bitbox : ch 2",  midi_out_serial[0], 2),
    MIDIOutputWrapper((char*)"S1 : Bitbox : ch 3",  midi_out_serial[0], 3),
    MIDIOutputWrapper((char*)"S2 : unused : ch 1",  midi_out_serial[1], 1),
    MIDIOutputWrapper((char*)"S3 : Neutron : ch 4", midi_out_serial[2], 4),
    MIDIOutputWrapper((char*)"S4 : Disting : ch 1", midi_out_serial[3], 1),
    //MIDIOutputWrapper("Serial 4 [unused ch 1]", midi_out_serial[3], 1),di
    //MIDIOutputWrapper("Serial 5 [unused ch 1]", midi_out_serial[4], 1),
    //MIDIOutputWrapper("Serial 6 [unused ch 1]", midi_out_serial[5], 1),
    MIDIOutputWrapper((char*)"USB : Bamble : ch 1", &(behaviour_bamble->device), 1),
    MIDIOutputWrapper((char*)"USB : Bamble : ch 2", &(behaviour_bamble->device), 2),
    //MIDIOutputWrapper((char*)"USB : Bamble : ch 3", &(behaviour_bamble->device), 3),
    #ifdef ENABLE_CRAFTSYNTH
        MIDIOutputWrapper((char*)"USB : CraftSynth : ch 1", &(behaviour_craftsynth->device), 1),
    #endif
}; 
    
//#define NUM_AVAILABLE_OUTPUTS sizeof(available_outputs)

MIDIOutputWrapper *find_wrapper_for_name(char *label_to_find) {
    /*for (int i = 0 ; i < NUM_AVAILABLE_OUTPUTS ; i++) {
        if (strcmp(to_find, available_outputs[i].label))
            return &available_outputs[i];
    }
    return nullptr;*/
    int f = find_wrapper_index_for_label(label_to_find);
    if (f>=0) 
        return &available_outputs[f];
    return nullptr;
}

int find_wrapper_index_for_label(char *label_to_find) {
    for (int i = 0 ; i < NUM_AVAILABLE_OUTPUTS ; i++) {
        if (0==strcmp(label_to_find, available_outputs[i].label)) {
            Serial.printf("find_wrapper_index_for_label() found '%s' in '%s' at index %i!\n", label_to_find, available_outputs[i].label, i); Serial.flush();
            return i;
        }
    }
    //while (1)
        Serial.printf("find_wrapper_index_for_label('%s') didn't find anything!\n", label_to_find);
    return -1;
}

#include "behaviour_beatstep.h"
#include "behaviour_keystep.h"
#include "behaviour_mpk49.h"

/*#include "menu_midi_mapper.h"
extern MidiOutputSelectorControl beatstep_output_selector;
extern MidiOutputSelectorControl keystep_output_selector;
extern MidiOutputSelectorControl mpk49_output_selector;
extern MidiOutputSelectorControl lestrum_pads_output_selector;
extern MidiOutputSelectorControl lestrum_arp_output_selector;
extern MidiOutputSelectorControl pc_usb_1_selector;
extern MidiOutputSelectorControl pc_usb_2_selector;*/

void set_target_wrapper_for_names(String source_label, String target_label) {
    Serial.printf("set_target_wrapper_for_names(%s, %s)\n", source_label.c_str(), target_label.c_str()); Serial.flush();
    MIDIOutputWrapper *target = find_wrapper_for_name((char*)target_label.c_str());
    int index = find_wrapper_index_for_label((char*)target_label.c_str());
    if (source_label.equals("beatstep_output")) {
        beatstep_setOutputWrapper(target);
        //beatstep_output_selector.actual_value_index = index;
    } else if (source_label.equals("keystep_output")) {
        keystep_setOutputWrapper(target);
        //keystep_output_selector.actual_value_index = index;
    } else if (source_label.equals("mpk49_output")) {
        mpk49_setOutputWrapper(target);
        //mpk49_output_selector.actual_value_index = index;
    } else if (source_label.equals("lestrum_pads_output")) {
        lestrum_pads_setOutputWrapper(target);
        //lestrum_pads_output_selector.actual_value_index = index;
    } else if (source_label.equals("lestrum_arp_output")) {
        lestrum_arp_setOutputWrapper(target);
        //lestrum_arp_output_selector.actual_value_index = index;
    } else if (source_label.equals("pc_usb_1_output")) {
        pc_usb_1_setOutputWrapper(target);
        //pc_usb_1_selector.actual_value_index = index;
    } else if (source_label.equals("pc_usb_2_output")) {
        pc_usb_2_setOutputWrapper(target);
        //pc_usb_2_selector.actual_value_index = index;
    } else
        Serial.printf("set_target_wrapper_for_names: Couldn't match source_label '%s' with anything!\n", source_label.c_str()); Serial.flush();
    update_wrapper_menus_for_name(source_label, index);
}

/*char *find_wrapper_name_for_object(MIDIOutputWrapper *obj_to_find) {
    for (int i = 0 ; i < NUM_AVAILABLE_OUTPUTS ; i++) {
        if (available_outputs[i]==obj_to_find) 
            return available_outputs[i].
    }
}*/