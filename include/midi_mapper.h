#ifndef MIDI_MAPPER__INCLUDED
#define MIDI_MAPPER__INCLUDED

#include "Config.h"
#include "midi_outs.h"
#include "midi_out_wrapper.h"

#ifdef ENABLE_CRAFTSYNTH
    #define NUM_AVAILABLE_OUTPUTS 9
#else
    #define NUM_AVAILABLE_OUTPUTS 8
#endif
extern MIDIOutputWrapper available_outputs[NUM_AVAILABLE_OUTPUTS]; 

MIDIOutputWrapper *find_wrapper_for_name(char *to_find);
int find_wrapper_index_for_label(char *to_find);

//void set_output_wrapper_for_names(char*source_label, char*target_label);
void set_target_wrapper_for_names(String source_label, String target_label);
/*class MIDIOutputWrapperManager {
    public:

    LinkedList<MIDIOutputWrapper *> available_outputs;

    void setup() {
        add(MIDIOutputWrapper((char*)"S1 : Bitbox : ch 1", midi_out_serial[0], 1));
        add(MIDIOutputWrapper((char*)"S1 : Bitbox : ch 2", midi_out_serial[0], 2));
        add(MIDIOutputWrapper((char*)"S1 : Bitbox : ch 3", midi_out_serial[0], 3));
        add(MIDIOutputWrapper((char*)"S2 : unused : ch 1", midi_out_serial[1], 1));
        add(MIDIOutputWrapper((char*)"S3 : Neutron : ch4 ", midi_out_serial[2], 4));
        add(MIDIOutputWrapper((char*)"S4 : Disting : ch 1", midi_out_serial[3], 1));
        //MIDIOutputWrapper("Serial 4 [unused ch1]", midi_out_serial[3], 1),
        //MIDIOutputWrapper("Serial 5 [unused ch1]", midi_out_serial[4], 1),
        //MIDIOutputWrapper("Serial 6 [unused ch1]", midi_out_serial[5], 1),
        
        add(MIDIOutputWrapper((char*)"USB : Bamble : ch1 ", &midi_bamble, 1));
        add(MIDIOutputWrapper((char*)"USB : Bamble : ch2 ", &midi_bamble, 2);)
    }

    void add(MIDIOutputWrapper *to_add) {
        available_outputs.add(to_add);
    }
    MIDIOutputWrapper* find(char *name) {
        for (int i = 0 ; i < available_outputs.size() ; i++) {
            if (strcmp(name, available_outputs.get(i)->label))
                return available_outputs.get(i);
        }
        return nullptr;
    }
}*/

#endif