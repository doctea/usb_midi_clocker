#ifndef MIDI_MAPPER__INCLUDED
#define MIDI_MAPPER__INCLUDED

#include "Config.h"
#include "midi_outs.h"
#include "midi_out_wrapper.h"

#include "behaviour_bamble.h"

/*#ifdef ENABLE_CRAFTSYNTH
    #define NUM_AVAILABLE_OUTPUTS 9
#else
    #define NUM_AVAILABLE_OUTPUTS 8
#endif
extern MIDIOutputWrapper available_outputs[NUM_AVAILABLE_OUTPUTS]; */

MIDIOutputWrapper *find_wrapper_for_name(char *to_find);
int find_wrapper_index_for_label(char *to_find);
void setup_midi_output_wrapper_manager();
void set_target_wrapper_for_names(String source_label, String target_label);

class MIDIOutputWrapperManager {
    public:

    static MIDIOutputWrapperManager* getInstance();

    LinkedList<MIDIOutputWrapper *> available_outputs;

    void add(MIDIOutputWrapper *to_add) {
        available_outputs.add(to_add);
    }
    MIDIOutputWrapper* find(int index) {
        return available_outputs.get(index);
    }
    MIDIOutputWrapper* find(char *name) {
        for (int i = 0 ; i < available_outputs.size() ; i++) {
            if (strcmp(name, available_outputs.get(i)->label))
                return available_outputs.get(i);
        }
        return nullptr;
    }
    int find_index(char *label_to_find) {
        for (int i = 0 ; i < available_outputs.size() ; i++) {
            if (0==strcmp(label_to_find, available_outputs.get(i)->label)) {
                Serial.printf("find_wrapper_index_for_label() found '%s' in '%s' at index %i!\n", label_to_find, available_outputs.get(i)->label, i); Serial.flush();
                return i;
            }
        }
        //while (1)
            Serial.printf("find_wrapper_index_for_label('%s') didn't find anything!\n", label_to_find);
        return -1;
    }
    char *get_label_for_index(int index) {
        return this->available_outputs.get(index)->label;
    }

    int get_num_available() {
        return this->available_outputs.size();
    }

    private:
        static MIDIOutputWrapperManager* inst_;
        MIDIOutputWrapperManager() {
            setup_midi_output_wrapper_manager();
        }
        MIDIOutputWrapperManager(const MIDIOutputWrapperManager&);
        MIDIOutputWrapperManager& operator=(const MIDIOutputWrapperManager&);
};

extern MIDIOutputWrapperManager *midi_output_wrapper_manager;

#endif