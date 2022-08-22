#ifndef MIDI_MAPPER_MATRIX__INCLUDED
#define MIDI_MAPPER_MATRIX__INCLUDED

#include "Config.h"
#include "midi_outs.h"
#include "midi_out_wrapper.h"

#include "midi_looper.h"

#include "LinkedList.h"

void setup_midi_mapper_matrix_manager();

//#include "behaviour_bamble.h"

#define NUM_SOURCES 16
#define NUM_TARGETS 16

#include "midi_mapper_matrix_types.h"

class MIDITrack;
class DeviceBehaviourBase;

struct target_entry {
    char handle[25];
    MIDIOutputWrapper *wrapper = nullptr;
};
struct source_entry {
    char handle[25];
};

class MIDIMatrixManager {
    public:
    static MIDIMatrixManager* getInstance();

    bool debug = false;

    // so we wanna do something like:-
    //      for each source
    //          store a list of targets it is connected to
    //      this requires a concept of a 'source', so like a 'source wrapper'.. 
    //          devicebehaviours might be a candidate for these?
    //          but there are also serial-midi sources, and sources from loopers...
    //          each source needs to be able to talk to this matrix manager, to know which targets to send notes to
    //      then we have targets (outputwrappers?)
    //          they don't need to know what their sources are?
    //      sometimes a source can also be a target, eg, looper.
    //      
    //  sources:-
    //      midi devices (keyboards and sequencers, usb and serial), with channel
    //      loopers
    //  targets:-
    //      synths (usb and midi), with channel
    //      loopers

    int sources_count = 0;

    source_entry sources[NUM_SOURCES] = {};
    source_id_t register_source(const char *handle) {
        Serial.printf("midi_mapper_matrix_manager#register_source() registering handle '%s'\n", handle);
        strcpy(sources[sources_count].handle, handle);
        return sources_count++;
    }
    source_id_t register_source(MIDITrack *loop_track, const char *handle);
    source_id_t register_source(DeviceBehaviourBase *device, const char *handle);

    source_id_t get_source_id_for_handle(const char *handle) {
        for (source_id_t i = 0; i < sources_count ; i++) {
            if (strcmp(handle, sources[i].handle)==0)
                return i;
        }
        Serial.printf("!! get_source_id_for_handle couldn't find a source for handle '%s'\n", handle);
        return -1;
    }

    bool source_to_targets[NUM_SOURCES][NUM_TARGETS] = {};

    void reset_matrix() {
        for (source_id_t source_id = 0 ; source_id < sources_count ; source_id++) {
            for (target_id_t target_id  = 0 ; target_id < targets_count ; target_id++) {
                disconnect_source_target(source_id, target_id);
            }
        }
    }

    bool is_connected(source_id_t source_id, target_id_t target_id) {
        return source_to_targets[source_id][target_id];
    }

    void toggle_source_target(source_id_t source_id, target_id_t target_id) {
        if (!is_connected(source_id, target_id))
            this->connect_source_target(source_id, target_id);
        else
            this->disconnect_source_target(source_id, target_id);
    }
    void connect_source_target(MIDITrack *track, DeviceBehaviourBase *device);
    void connect_source_target(DeviceBehaviourBase *device, const char *handle);
    void connect_source_target(source_id_t source_id, target_id_t target_id) {
        source_to_targets[source_id][target_id] = true;
    }

    void connect_source_target(const char *source_handle, const char *target_handle) {
        this->connect_source_target(
            this->get_source_id_for_handle(source_handle),
            this->get_target_id_for_handle(target_handle)
        );
    }
    void disconnect_source_target(const char *source_handle, const char *target_handle) {
        this->disconnect_source_target(
            this->get_source_id_for_handle(source_handle),
            this->get_target_id_for_handle(target_handle)
        );
    }
    void disconnect_source_target(source_id_t source_id, target_id_t target_id) {
        if (is_connected(source_id, target_id)) {
            if (targets[target_id].wrapper!=nullptr) 
                targets[target_id].wrapper->stop_all_notes();
        }
        source_to_targets[source_id][target_id] = false;
    }

    void send_note_on(source_id_t source_id, byte pitch, byte velocity, byte channel = 0) {
        if (source_id<0) {
            if (this->debug) Serial.printf("!! midi_mapper_matrix_manager#send_note_on() passed source_id of %i!\n", source_id);
            return;
        }
        if (this->debug) Serial.printf("midi_mapper_matrix_manager#send_note_on(source_id=%i, pitch=%i, velocity=%i, channel=%i)\n", source_id, pitch, velocity, channel);
        for (target_id_t target_id = 0 ; target_id < targets_count ; target_id++) {
            if (is_connected(source_id, target_id)) {
                //targets[target_id].wrapper->debug = true;
                if (this->debug) Serial.printf("\t%i: %s should send to %s\n", target_id, sources[source_id].handle, targets[target_id].handle);
                targets[target_id].wrapper->sendNoteOn(pitch, velocity, channel);
                //targets[target_id].wrapper->debug = false;
            }
        }
    }
    void send_note_off(source_id_t source_id, byte pitch, byte velocity, byte channel = 0) {
        if (source_id<0) {
            if (this->debug) Serial.printf("!! midi_mapper_matrix_manager#send_note_off() passed source_id of %i!\n", source_id);
            return;
        }
        if (this->debug) Serial.printf("midi_mapper_matrix_manager#send_note_off(source_id=%i, pitch=%i, velocity=%i, channel=%i)\n", source_id, pitch, velocity, channel);
        for (target_id_t target_id = 0 ; target_id < targets_count ; target_id++) {
            if (is_connected(source_id, target_id)) {
                targets[target_id].wrapper->debug = true;
                if (this->debug) Serial.printf("\t%i: %s should send to %s\n", target_id, sources[source_id].handle, targets[target_id].handle);
                targets[target_id].wrapper->sendNoteOff(pitch, velocity, channel);
                targets[target_id].wrapper->debug = false;
            }
        }
    }
    void send_control_change(source_id_t source_id, byte cc, byte value, byte channel = 0) {
        for (target_id_t target_id = 0 ; target_id < NUM_TARGETS ; target_id++) {
            if (is_connected(source_id, target_id)) {
                targets[target_id].wrapper->sendControlChange(cc, value, channel);
            }
        }
    }
    void stop_all_notes(source_id_t source_id) {
        for (target_id_t target_id = 0 ; target_id < NUM_TARGETS ; target_id++) {
            if (is_connected(source_id, target_id)) {
                targets[target_id].wrapper->stop_all_notes();
            }
        }
    }

    const char *get_label_for_source_id(source_id_t source_id) {
        return this->sources[source_id].handle;
    }
    const char *get_label_for_target_id(target_id_t target_id) {
        if(this->targets[target_id].wrapper!=nullptr) 
            return this->targets[target_id].wrapper->label;
        return (const char*)"[error - unknown]";
    }

    int targets_count = 0;
    target_entry targets[NUM_TARGETS] = {};

    int register_target(MIDIOutputWrapper *target) {
        return this->register_target(target, target->label);
    }
    int register_target(MIDITrack *target, const char *handle) {
        return this->register_target(new MIDIOutputWrapper(handle, target));
    }
    int register_target(MIDIOutputWrapper *target, const char *handle) {
        Serial.printf("midi_mapper_matrix_manager#register_target() registering handle '%s'\n", handle);
        strcpy(targets[targets_count].handle, handle);
        targets[targets_count].wrapper = target;
        return targets_count++;
    }

    int get_target_id_for_handle(const char *handle) {
        Serial.printf("get_target_id_for_handle(%s)\n", handle);
        for (int i = 0; i < targets_count ; i++) {
            Serial.printf("\t%i: looking for '%s', comparing '%s'..\n", i, handle, targets[i].handle);
            if (strcmp(handle, targets[i].handle)==0) {
                return i;
            }
        }
        Serial.printf("!! get_target_id_for_handle couldn't find a target for handle '%s'\n", handle);
        return -1;
    }
    MIDIOutputWrapper *get_target_for_handle(char *handle) {
        return this->targets[this->get_target_id_for_handle(handle)].wrapper;
    }

    private:
        static MIDIMatrixManager* inst_;
        MIDIMatrixManager() {
            //setup_midi_output_wrapper_manager();
        }
        MIDIMatrixManager(const MIDIMatrixManager&);
        MIDIMatrixManager& operator=(const MIDIMatrixManager&);
};

extern MIDIMatrixManager *midi_matrix_manager;

#endif