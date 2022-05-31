#include "midi_mapper.h"

MIDIOutputWrapper available_outputs[NUM_AVAILABLE_OUTPUTS] = {
    MIDIOutputWrapper((char*)"S1 : Bitbox : ch 1", midi_out_serial[0], 1),
    MIDIOutputWrapper((char*)"S1 : Bitbox : ch 2", midi_out_serial[0], 2),
    MIDIOutputWrapper((char*)"S1 : Bitbox : ch 3", midi_out_serial[0], 3),
    MIDIOutputWrapper((char*)"S2 : unused : ch 1", midi_out_serial[1], 1),
    MIDIOutputWrapper((char*)"S3 : Neutron : ch4 ", midi_out_serial[2], 4),
    MIDIOutputWrapper((char*)"S4 : Disting : ch 1", midi_out_serial[3], 1),
    //MIDIOutputWrapper("Serial 4 [unused ch1]", midi_out_serial[3], 1),
    //MIDIOutputWrapper("Serial 5 [unused ch1]", midi_out_serial[4], 1),
    //MIDIOutputWrapper("Serial 6 [unused ch1]", midi_out_serial[5], 1),
    
    MIDIOutputWrapper((char*)"USB : Bamble : ch1 ", &midi_bamble, 1),
    MIDIOutputWrapper((char*)"USB : Bamble : ch2 ", &midi_bamble, 2),
};

MIDIOutputWrapper *find_wrapper_for_name(char *to_find) {
    for (int i = 0 ; i < NUM_AVAILABLE_OUTPUTS ; i++) {
        if (strcmp(to_find, available_outputs[i].label))
            return &available_outputs[i];
    }
    return nullptr;
}

int find_wrapper_index_for_name(char *to_find) {
    for (int i = 0 ; i < NUM_AVAILABLE_OUTPUTS ; i++) {
        if (strcmp(to_find, available_outputs[i].label))
            return i;
    }
    return -1;
}