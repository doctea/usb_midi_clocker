#include "midi_mapper.h"

// NOTE: you gotta be careful with the names of these things!
// if there is a used MIDIOutputWrapper somewhere that doesnt have a label that matches these strings EXACTLY, the midi_mapper menu item will crash and apparently 
// never tell us about it in console!!
MIDIOutputWrapper available_outputs[NUM_AVAILABLE_OUTPUTS] = {
    MIDIOutputWrapper((char*)"S1 : Bitbox : ch 1",  midi_out_serial[0], 1),
    MIDIOutputWrapper((char*)"S1 : Bitbox : ch 2",  midi_out_serial[0], 2),
    MIDIOutputWrapper((char*)"S1 : Bitbox : ch 3",  midi_out_serial[0], 3),
    MIDIOutputWrapper((char*)"S2 : unused : ch 1",  midi_out_serial[1], 1),
    MIDIOutputWrapper((char*)"S3 : Neutron : ch 4", midi_out_serial[2], 4),
    MIDIOutputWrapper((char*)"S4 : Disting : ch 1", midi_out_serial[3], 1),
    //MIDIOutputWrapper("Serial 4 [unused ch 1]", midi_out_serial[3], 1),
    //MIDIOutputWrapper("Serial 5 [unused ch 1]", midi_out_serial[4], 1),
    //MIDIOutputWrapper("Serial 6 [unused ch 1]", midi_out_serial[5], 1),
    MIDIOutputWrapper((char*)"USB : Bamble : ch 1", &midi_bamble, 1),
    MIDIOutputWrapper((char*)"USB : Bamble : ch 2", &midi_bamble, 2),
};

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
            Serial.printf("find_wrapper_index_for_label() found '%s' in '%s' at index %i!\n", label_to_find, available_outputs[i].label, i);
            return i;
        }
    }
    while (1)
        Serial.printf("find_wrapper_index_for_label('%s') didn't find anything!\n", label_to_find);
    return -1;
}