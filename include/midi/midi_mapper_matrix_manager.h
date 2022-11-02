#ifndef MIDI_MAPPER_MATRIX__INCLUDED
#define MIDI_MAPPER_MATRIX__INCLUDED

#include "Config.h"
#include "midi/midi_outs.h"
#include "midi/midi_out_wrapper.h"
#include "midi/midi_helpers.h"
#include "midi/midi_looper.h"

#include "LinkedList.h"

void setup_midi_mapper_matrix_manager();

//#include "behaviours/behaviour_bamble.h"

#define MAX_NUM_SOURCES 24
#define MAX_NUM_TARGETS 24
#define NUM_REGISTERED_TARGETS targets_count
#define NUM_REGISTERED_SOURCES sources_count

#define LANGST_HANDEL_ROUT 20   // longest possible name of a target handle / get roo to do it <3

#include "midi/midi_mapper_matrix_types.h"

class MIDITrack;
class DeviceBehaviourUltimateBase;

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


    /* stuff for handling sources of midi data */
    struct source_entry {
        char handle[LANGST_HANDEL_ROUT];    // 25 * 24 = 600 bytes
    };
    int sources_count = 0;

    //DMAMEM source_entry sources[MAX_NUM_SOURCES];
    source_entry *sources = nullptr;

    // assign a source_id for the given name
    FLASHMEM source_id_t register_source(const char *handle) {
        Serial.printf(F("midi_mapper_matrix_manager#register_source() registering handle '%s'\n"), handle);
        strcpy(sources[NUM_REGISTERED_SOURCES].handle, handle);
        return NUM_REGISTERED_SOURCES++;
    }
    // assign a source_id for the midi track
    FLASHMEM source_id_t register_source(MIDITrack *loop_track, const char *handle);
    // assign a source_id for the device
    FLASHMEM source_id_t register_source(DeviceBehaviourUltimateBase *device, const char *handle);

    // get id of source for string
    FLASHMEM source_id_t get_source_id_for_handle(const char *handle) {
        for (source_id_t i = 0; i < NUM_REGISTERED_SOURCES ; i++) {
            if (strcmp(handle, sources[i].handle)==0)
                return i;
        }
        Serial.printf(F("!! get_source_id_for_handle couldn't find a source for handle '%s'\n"), handle);
        return -1;
    }

    bool source_to_targets[MAX_NUM_SOURCES][MAX_NUM_TARGETS] = {};  // 24*24 = 576 bytes

    // reset all connections (eg loading preset)
    void reset_matrix() {
        for (source_id_t source_id = 0 ; source_id < NUM_REGISTERED_SOURCES ; source_id++) {
            for (target_id_t target_id  = 0 ; target_id < NUM_REGISTERED_TARGETS ; target_id++) {
                disconnect(source_id, target_id);
            }
        }
    }
    // is this source id connected to target id
    bool is_connected(source_id_t source_id, target_id_t target_id) {
        return source_to_targets[source_id][target_id];
    }

    void toggle_connect(source_id_t source_id, target_id_t target_id) {
        if (is_connected(source_id, target_id))
            this->disconnect(source_id, target_id);
        else
            this->connect(source_id, target_id);
    }
    
    // connect source to target)
    void connect(MIDITrack *source_track, DeviceBehaviourUltimateBase *target_behaviour);
    void connect(DeviceBehaviourUltimateBase *source_behaviour, const char *target_handle);
    void connect(const char *source_handle, const char *target_handle) {
        this->connect(
            this->get_source_id_for_handle(source_handle),
            this->get_target_id_for_handle(target_handle)
        );
    }
    void connect(source_id_t source_id, target_id_t target_id) {
        source_to_targets[source_id][target_id] = true;
    }

    void disconnect(const char *source_handle, const char *target_handle) {
        this->disconnect(
            this->get_source_id_for_handle(source_handle),
            this->get_target_id_for_handle(target_handle)
        );
    }
    void disconnect(source_id_t source_id, target_id_t target_id) {
        if (is_connected(source_id, target_id)) {
            if (targets[target_id].wrapper!=nullptr) 
                targets[target_id].wrapper->stop_all_notes();
        }
        source_to_targets[source_id][target_id] = false;
    }

    ///// handle incoming or generated events (from a midi device, looper, etc) and route to connected outputs
    void processNoteOn(source_id_t source_id, byte pitch, byte velocity, byte channel = 0) {
        if (!is_valid_note(pitch)) return;
        if (source_id<0) {
            if (this->debug) Serial.printf(F("!! midi_mapper_matrix_manager#processNoteOn() passed source_id of %i!\n"), source_id);
            return;
        }
        if (this->debug) Serial.printf(F("midi_mapper_matrix_manager#processNoteOn(source_id=%i, pitch=%i, velocity=%i, channel=%i)\n"), source_id, pitch, velocity, channel);
        for (target_id_t target_id = 0 ; target_id < NUM_REGISTERED_TARGETS ; target_id++) {
            if (is_connected(source_id, target_id)) {
                //targets[target_id].wrapper->debug = true;
                if (this->debug) Serial.printf("\t%i: %s should send to %s\n", target_id, sources[source_id].handle, targets[target_id].handle);
                targets[target_id].wrapper->sendNoteOn(pitch, velocity, channel);
                //targets[target_id].wrapper->debug = false;
            }
        }
    }
    void processNoteOff(source_id_t source_id, byte pitch, byte velocity, byte channel = 0) {
        if (!is_valid_note(pitch)) return;
        if (source_id<0) {
            if (this->debug) Serial.printf(F("!! midi_mapper_matrix_manager#processNoteOff() passed source_id of %i!\n"), source_id);
            return;
        }
        if (this->debug) Serial.printf(F("midi_mapper_matrix_manager#processNoteOff(source_id=%i, pitch=%i, velocity=%i, channel=%i)\n"), source_id, pitch, velocity, channel);
        for (target_id_t target_id = 0 ; target_id < NUM_REGISTERED_TARGETS ; target_id++) {
            if (is_connected(source_id, target_id)) {
                //targets[target_id].wrapper->debug = true;
                if (this->debug) Serial.printf(F("\t%i: %s should send to %s\n"), target_id, sources[source_id].handle, targets[target_id].handle);
                targets[target_id].wrapper->sendNoteOff(pitch, velocity, channel);
                //targets[target_id].wrapper->debug = false;
            }
        }
    }
    void processControlChange(source_id_t source_id, byte cc, byte value, byte channel = 0) {
        for (target_id_t target_id = 0 ; target_id < NUM_REGISTERED_TARGETS ; target_id++) {
            if (is_connected(source_id, target_id)) {
                /*Serial.printf("midi_matrix_manager#processControlChange(%i, %i, %i, %i): %i is connected to %i!\n",
                    source_id, cc, value, channel, source_id, target_id
                ); Serial.flush();
                if (targets[target_id].wrapper==nullptr) {
                    Serial.printf("target_id %i has a nullptr wrapper!\n", target_id); Serial.flush();
                }*/
                targets[target_id].wrapper->sendControlChange(cc, value, channel);
                //Serial.println("successfully sent!"); Serial.flush();
            }
        }
    }
    void processPitchBend(source_id_t source_id, int bend, byte channel = 0) {
        for (target_id_t target_id = 0 ; target_id < NUM_REGISTERED_TARGETS ; target_id++) {
            if (is_connected(source_id, target_id)) {
                /*Serial.printf("midi_matrix_manager#processControlChange(%i, %i, %i, %i): %i is connected to %i!\n",
                    source_id, cc, value, channel, source_id, target_id
                ); Serial.flush();
                if (targets[target_id].wrapper==nullptr) {
                    Serial.printf("target_id %i has a nullptr wrapper!\n", target_id); Serial.flush();
                }*/
                targets[target_id].wrapper->sendPitchBend(bend, channel);
                //Serial.println("successfully sent!"); Serial.flush();
            }
        }
    }
    void stop_all_notes_for_source(source_id_t source_id, bool force = false) {
        if (source_id==-1) return;
        for (target_id_t target_id = 0 ; target_id < NUM_REGISTERED_TARGETS ; target_id++) {
            if (is_connected(source_id, target_id)) {
                this->stop_all_notes_for_target(target_id, force);
            }
        }
    }
    void stop_all_notes_for_target(target_id_t target_id, bool force = false) {
        if (target_id >= NUM_REGISTERED_TARGETS) return;
        Serial.printf("stop_all_notes on target_id=%i wrapper %s\n", target_id, targets[target_id].wrapper->label); Serial.flush();
        targets[target_id].wrapper->stop_all_notes(force);
    }

    void stop_all_notes(bool force = false) {
        for (target_id_t target_id = 0 ; target_id < NUM_REGISTERED_TARGETS ; target_id++) {
            Serial.printf("stop_all_notes on target_id=%i..\n", target_id); Serial.flush();
            this->stop_all_notes_for_target(target_id, force);
        }
    }
    void stop_all_notes_force() {
        this->stop_all_notes(true);
    }

    const char *get_label_for_source_id(source_id_t source_id) {
        return this->sources[source_id].handle;
    }
    const char *get_label_for_target_id(target_id_t target_id) {
        if(this->targets[target_id].wrapper!=nullptr) 
            return this->targets[target_id].wrapper->label;
        return (const char*)F("[error - unknown]");
    }

    byte getDefaultChannelForTargetId(target_id_t target_id) {
        if (target_id>=0 && target_id < NUM_REGISTERED_TARGETS)
            return this->targets[target_id].wrapper->default_channel;
        return 0;
    }

    //// stuff for handling targets of midi data
    struct target_entry {
        char handle[25];
        MIDIOutputWrapper *wrapper = nullptr;
    };

    int targets_count = 0;
    target_entry targets[MAX_NUM_TARGETS] = {};

    /*target_id_t register_target(DeviceBehaviourUltimateBase *target_behaviour, const char *handle) {
        target_id_t t = this->register_target(make_midioutputwrapper(handle, target_behaviour));
        target_behaviour->target_id = t;
        return t;
    }*/
    FLASHMEM target_id_t register_target(MIDIOutputWrapper *target) {
        return this->register_target(target, target->label);
    }
    FLASHMEM target_id_t register_target(MIDITrack *target, const char *handle) {
        return this->register_target(make_midioutputwrapper(handle, target));
    }
    FLASHMEM target_id_t register_target(MIDIOutputWrapper *target, const char *handle) {
        strcpy(targets[NUM_REGISTERED_TARGETS].handle, handle);
        targets[NUM_REGISTERED_TARGETS].wrapper = target;
        Serial.printf(F("midi_mapper_matrix_manager#register_target() registering handle '%s' as target_id %i\n"), handle, NUM_REGISTERED_TARGETS);
        if (target==nullptr) {
            Serial.printf(F("WARNING: register_target for handle %s (target_id %i) passed a null target wrapper!!!\n"), target, NUM_REGISTERED_TARGETS);
        }
        return NUM_REGISTERED_TARGETS++;
    }
    /*target_id_t register_target(DeviceBehaviourUltimateBase *target, const char *handle) {
        Serial.printf("midi_mapper_matrix_manager#register_target(DeviceBehaviour) registering handle '%s'\n", handle);
        MIDIOutputWrapper_Behaviour wrapper = make_midioutputwrapper(handle, target, )
        target->target_id = targets_count;
    }*/

    target_id_t get_target_id_for_handle(const char *handle) {
        //Serial.printf(F("get_target_id_for_handle(%s)\n"), handle);
        for (int i = 0; i < NUM_REGISTERED_TARGETS ; i++) {
            //Serial.printf(F("\t%i: looking for '%s', comparing '%s'..\n"), i, handle, targets[i].handle);
            if (strcmp(handle, targets[i].handle)==0) {
                return i;
            }
        }
        Serial.printf(F("!! get_target_id_for_handle couldn't find a target for handle '%s'\n"), handle);
        return -1;
    }
    MIDIOutputWrapper *get_target_for_handle(char *handle) {
        //return this->targets[this->get_target_id_for_handle(handle)].wrapper;
        return this->get_target_for_id(this->get_target_id_for_handle(handle));
    }
    MIDIOutputWrapper *get_target_for_id(target_id_t target_id) {
        if (target_id>=0 && targets[target_id].wrapper!=nullptr)
            return targets[target_id].wrapper;
        return nullptr;
    }

    private:
        // stuff for making singleton
        static MIDIMatrixManager* inst_;
        MIDIMatrixManager() {
            //setup_midi_output_wrapper_manager();
            //memset(&sources, 0, MAX_NUM_SOURCES*sizeof(source_entry));
            sources = (source_entry*)malloc(sizeof(source_entry) * MAX_NUM_SOURCES);
            memset(sources, 0, sizeof(source_entry) * MAX_NUM_SOURCES);
        }
        MIDIMatrixManager(const MIDIMatrixManager&);
        MIDIMatrixManager& operator=(const MIDIMatrixManager&);
};

extern MIDIMatrixManager *midi_matrix_manager;

#endif