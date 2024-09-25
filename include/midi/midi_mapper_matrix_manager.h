#if (defined __GNUC__) && (__GNUC__ >= 5) && (__GNUC_MINOR__ >= 4) && (__GNUC_PATCHLEVEL__ > 1)
    #pragma GCC diagnostic ignored "-Wpragmas"
    #pragma GCC diagnostic ignored "-Wformat-truncation"
    #pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

#pragma once

#include "Config.h"
#include "midi/midi_outs.h"
#include "midi/midi_out_wrapper.h"
#include "midi_helpers.h"
#include "midi/midi_looper.h"

#include "LinkedList.h"

void setup_midi_mapper_matrix_manager();

void behaviour_manager_kill_all_current_notes();

//#include "behaviours/behaviour_bamble.h"

#define MAX_NUM_SOURCES 30
#define MAX_NUM_TARGETS 30
#define NUM_REGISTERED_TARGETS targets_count
#define NUM_REGISTERED_SOURCES sources_count

#define LANGST_HANDEL_ROUT 25   // longest possible name of a target handle / get roo to do it <3

#include "midi/midi_mapper_matrix_types.h"

class MIDITrack;
class DeviceBehaviourUltimateBase;

class MIDIMatrixManager {
    public:
    static MIDIMatrixManager* getInstance();

    bool debug = false;

    bool    global_quantise_on = false;
    int8_t  global_scale_root = SCALE_ROOT_C;
    SCALE   global_scale_type = SCALE::MAJOR;

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
        uint8_t connection_count = 0;
    };
    uint8_t sources_count = 0;

    //DMAMEM source_entry sources[MAX_NUM_SOURCES];
    source_entry *sources = nullptr;

    // assign a source_id for the given name
    FLASHMEM source_id_t register_source(const char *handle) {
        //Serial_printf(F("midi_mapper_matrix_manager#register_source() registering handle '%s'\n"), handle);
        strncpy(sources[NUM_REGISTERED_SOURCES].handle, handle, LANGST_HANDEL_ROUT);
        return NUM_REGISTERED_SOURCES++;
    }
    // assign a source_id for the midi track
    //FLASHMEM 
    source_id_t register_source(MIDITrack *loop_track, const char *handle);
    // assign a source_id for the device
    //FLASHMEM 
    source_id_t register_source(DeviceBehaviourUltimateBase *device, const char *handle);

    // get id of source for string
    FLASHMEM source_id_t get_source_id_for_handle(const char *handle) {
        for (source_id_t i = 0; i < NUM_REGISTERED_SOURCES ; i++) {
            if (strcmp(handle, sources[i].handle)==0)
                return i;
        }
        Serial_printf(F("!! get_source_id_for_handle couldn't find a source for handle '%s'\n"), handle);
        return -1;
    }

    bool source_to_targets[MAX_NUM_SOURCES][MAX_NUM_TARGETS] = {};  // 24*24 = 576 bytes
    bool disallow_map[MAX_NUM_SOURCES][MAX_NUM_TARGETS] = {};

    // reset all connections (eg loading preset)
    void reset_matrix() {
        // todo: could optimise by checking connected_to_source_count() and connected_to_target_count()
        for (source_id_t source_id = 0 ; source_id < NUM_REGISTERED_SOURCES ; source_id++) {
            for (target_id_t target_id  = 0 ; target_id < NUM_REGISTERED_TARGETS ; target_id++) {
                disconnect(source_id, target_id);
            }
        }
    }
    // is this source id connected to target id
    bool is_connected(source_id_t source_id, target_id_t target_id) {
        if (source_id<0 || target_id<0 || source_id>NUM_REGISTERED_SOURCES || target_id>NUM_REGISTERED_TARGETS)
            return false;
        return source_to_targets[source_id][target_id];
    }

    // toggle a connection - return false if connection is now broken, true if it is now made
    bool toggle_connect(source_id_t source_id, target_id_t target_id) {
        if (is_connected(source_id, target_id)) {
            this->disconnect(source_id, target_id);
            return false;
        } else {
            this->connect(source_id, target_id);
            return true;
        }
    }

    // don't allow source X to connect to target Y -- eg to avoid loopbacks
    void disallow(source_id_t source_id, target_id_t target_id) {
        disallow_map[source_id][target_id] = true;
    }
    bool is_allowed(source_id_t source_id, target_id_t target_id) {
        return !disallow_map[source_id][target_id];
    }
    
    // connect source to target)
    void connect(MIDITrack *source_track, DeviceBehaviourUltimateBase *target_behaviour);
    void connect(DeviceBehaviourUltimateBase *source_behaviour, const char *target_handle);
    void connect(const char *source_handle, const char *target_handle) {
        if (source_handle==nullptr || target_handle==nullptr) return;
        this->connect(
            this->get_source_id_for_handle(source_handle),
            this->get_target_id_for_handle(target_handle)
        );
    }
    void connect(source_id_t source_id, target_id_t target_id) {
        if (source_id<0 || target_id<0 || source_id >= NUM_REGISTERED_SOURCES || target_id >= NUM_REGISTERED_TARGETS)
            return;
        if (!is_allowed(source_id, target_id))
            return;
        if (!source_to_targets[source_id][target_id]) {
            // increment count if not already connected
            sources[source_id].connection_count++;
            targets[target_id].connection_count++;
        }
        source_to_targets[source_id][target_id] = true;
    }

    void disconnect(const char *source_handle, const char *target_handle) {
        if (source_handle==nullptr || target_handle==nullptr) return;
        this->disconnect(
            this->get_source_id_for_handle(source_handle),
            this->get_target_id_for_handle(target_handle)
        );
    }
    void disconnect(source_id_t source_id, target_id_t target_id) {
        if (source_id==-1 || target_id==-1 || source_id >= NUM_REGISTERED_SOURCES || target_id >= NUM_REGISTERED_TARGETS) return;
        if (is_connected(source_id, target_id)) {
            if (targets[target_id].wrapper!=nullptr) 
                targets[target_id].wrapper->stop_all_notes();
            // decrement counts, only if connected
            sources[source_id].connection_count--;
            targets[target_id].connection_count--;
        }
        source_to_targets[source_id][target_id] = false;
    }

    uint8_t connected_to_source_count(source_id_t source_id) {
        if (source_id==-1 || source_id >= NUM_REGISTERED_SOURCES) return 0;

        return sources[source_id].connection_count;
    }
    uint8_t connected_to_target_count(target_id_t target_id) {
        if (target_id==-1 || target_id >= NUM_REGISTERED_SOURCES) return 0;

        return targets[target_id].connection_count;
    }

    ///// handle incoming or generated events (from a midi device, looper, etc) and route to connected outputs
    void processNoteOn(source_id_t source_id, int8_t pitch, uint8_t velocity, uint8_t channel = 0) {
        if (!is_valid_note(pitch)) return;
        if (this->global_quantise_on) pitch = quantise_pitch(pitch);
        if (!is_valid_note(pitch)) return;

        if (source_id<0) {
            if (this->debug) Serial_printf(F("!! midi_mapper_matrix_manager#processNoteOn() passed source_id of %i!\n"), source_id);
            return;
        }
        if (this->debug) Serial_printf(F("midi_mapper_matrix_manager#processNoteOn(source_id=%i,\tpitch=%i,\tvelocity=%i,\tchannel=%i)\n"), source_id, pitch, velocity, channel);

        for (target_id_t target_id = 0 ; target_id < NUM_REGISTERED_TARGETS ; target_id++) {
            if (is_connected(source_id, target_id)) {
                //targets[target_id].wrapper->debug = true;
                if (this->debug) Serial_printf(F("\t%s\tshould send to\t%s\t(source_id=%i)\n"), sources[source_id].handle, targets[target_id].handle, target_id);
                targets[target_id].wrapper->sendNoteOn(pitch, velocity, channel);
                //targets[target_id].wrapper->debug = false;
            }
        }
    }
    void processNoteOff(source_id_t source_id, int8_t pitch, uint8_t velocity, uint8_t channel = 0) {
        if (!is_valid_note(pitch)) {
            if (this->debug) Serial_printf("midi_mapper_matrix_manager#processNoteOff() passed invalid pitch %i - ignoring\n", pitch);
            return;
        }
        if (this->global_quantise_on) pitch = quantise_pitch(pitch);
        if (!is_valid_note(pitch)) return;

        if (source_id<0) {
            if (this->debug) Serial_printf(F("!! midi_mapper_matrix_manager#processNoteOff() passed source_id of %i!\n"), source_id);
            return;
        }
        if (this->debug) Serial_printf(F("midi_mapper_matrix_manager#processNoteOff(source_id=%i,\tpitch=%i,\tvelocity=%i,\tchannel=%i)\n"), source_id, pitch, velocity, channel);

        for (target_id_t target_id = 0 ; target_id < NUM_REGISTERED_TARGETS ; target_id++) {
            if (is_connected(source_id, target_id)) {
                //targets[target_id].wrapper->debug = true;
                if (this->debug/* || targets[target_id].wrapper->debug || source_id==12*/) Serial_printf(F("\t%s\tshould send to\t%s\t(target_id=%i)\n"), sources[source_id].handle, targets[target_id].handle, target_id);
                targets[target_id].wrapper->sendNoteOff(pitch, velocity, channel);
                //targets[target_id].wrapper->debug = false;
            }
        }
    }
    void processControlChange(source_id_t source_id, int8_t cc, uint8_t value, uint8_t channel = 0) {
        if (source_id==-1) return;
        for (target_id_t target_id = 0 ; target_id < NUM_REGISTERED_TARGETS ; target_id++) {
            if (is_connected(source_id, target_id)) {
                /*Serial_printf("midi_matrix_manager#processControlChange(%i, %i, %i, %i): %i is connected to %i!\n",
                    source_id, cc, value, channel, source_id, target_id
                ); Serial_flush();
                if (targets[target_id].wrapper==nullptr) {
                    Serial_printf("target_id %i has a nullptr wrapper!\n", target_id); Serial_flush();
                }*/
                targets[target_id].wrapper->sendControlChange(cc, value, channel);
                //Serial_println("successfully sent!"); Serial_flush();
            }
        }
    }
    void processPitchBend(source_id_t source_id, int bend, uint8_t channel = 0) {
        if (source_id==-1) return;
        for (target_id_t target_id = 0 ; target_id < NUM_REGISTERED_TARGETS ; target_id++) {
            if (is_connected(source_id, target_id)) {
                /*Serial_printf("midi_matrix_manager#processControlChange(%i, %i, %i, %i): %i is connected to %i!\n",
                    source_id, cc, value, channel, source_id, target_id
                ); Serial_flush();
                if (targets[target_id].wrapper==nullptr) {
                    Serial_printf("target_id %i has a nullptr wrapper!\n", target_id); Serial_flush();
                }*/
                targets[target_id].wrapper->sendPitchBend(bend, channel);
                //Serial_println("successfully sent!"); Serial_flush();
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
        if (target_id==-1 || target_id >= NUM_REGISTERED_TARGETS) return;
        Serial_printf("stop_all_notes on target_id=%i wrapper %s\n", target_id, targets[target_id].wrapper->label); Serial_flush();
        targets[target_id].wrapper->stop_all_notes(force);
    }

    void stop_all_notes(bool force = false) {
        for (target_id_t target_id = 0 ; target_id < NUM_REGISTERED_TARGETS ; target_id++) {
            Serial_printf("stop_all_notes on target_id=%i..\n", target_id); Serial_flush();
            this->stop_all_notes_for_target(target_id, force);
        }
    }
    void stop_all_notes_force() {
        this->stop_all_notes(true);
    }

    const char *get_label_for_source_id(source_id_t source_id) {
        if (source_id==-1) return nullptr;
        return this->sources[source_id].handle;
    }
    const char *get_label_for_target_id(target_id_t target_id) {
        if (target_id==-1) return nullptr;
        if(this->targets[target_id].wrapper!=nullptr) 
            return this->targets[target_id].wrapper->label;
        return (const char*)F("[error - unknown]");
    }

    uint8_t getDefaultChannelForTargetId(target_id_t target_id) {
        if (target_id>=0 && target_id < NUM_REGISTERED_TARGETS)
            return this->targets[target_id].wrapper->default_channel;
        return 0;
    }

    //// stuff for handling targets of midi data
    struct target_entry {
        char handle[LANGST_HANDEL_ROUT];
        uint8_t connection_count = 0;
        MIDIOutputWrapper *wrapper = nullptr;
    };

    uint8_t targets_count = 0;
    target_entry targets[MAX_NUM_TARGETS] = {};

    /*target_id_t register_target(DeviceBehaviourUltimateBase *target_behaviour, const char *handle) {
        target_id_t t = this->register_target(make_midioutputwrapper(handle, target_behaviour));
        target_behaviour->target_id = t;
        return t;
    }*/
    FLASHMEM target_id_t register_target(MIDIOutputWrapper *target) {
        return this->register_target(target, target->label);
    }
    FLASHMEM target_id_t register_target(MIDITrack *target, const char *handle, uint8_t channel = 1) {
        return this->register_target(make_midioutputwrapper(handle, target, channel));
    }
    FLASHMEM target_id_t register_target(MIDIOutputWrapper *target, const char *handle) {
        // TODO: detect and warn if duplicate handle used
        strncpy(targets[NUM_REGISTERED_TARGETS].handle, handle, LANGST_HANDEL_ROUT);
        targets[NUM_REGISTERED_TARGETS].wrapper = target;
        Serial_printf(F("midi_mapper_matrix_manager#register_target() registering handle '%s' as target_id %i\n"), handle, NUM_REGISTERED_TARGETS);
        if (target==nullptr) {
            Serial_printf(F("WARNING: register_target for handle %s (target_id %i) passed a null target wrapper!!!\n"), target, NUM_REGISTERED_TARGETS);
        }
        return NUM_REGISTERED_TARGETS++;
    }
    /*target_id_t register_target(DeviceBehaviourUltimateBase *target, const char *handle) {
        Serial_printf("midi_mapper_matrix_manager#register_target(DeviceBehaviour) registering handle '%s'\n", handle);
        MIDIOutputWrapper_Behaviour wrapper = make_midioutputwrapper(handle, target, )
        target->target_id = targets_count;
    }*/

    target_id_t get_target_id_for_handle(const char *handle) {
        //Serial_printf(F("get_target_id_for_handle(%s)\n"), handle);
        for (unsigned int i = 0 ; i < NUM_REGISTERED_TARGETS ; i++) {
            //Serial_printf(F("\t%i: looking for '%s', comparing '%s'..\n"), i, handle, targets[i].handle);
            if (strcmp(handle, targets[i].handle)==0) {
                return i;
            }
        }
        Serial_printf(F("!! get_target_id_for_handle couldn't find a target for handle '%s'\n"), handle);
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

    // look up the serial midi number for a given uart device
    serial_midi_number_t get_serial_midi_number_for_device(midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *device) {
        for (int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
            if (device==midi_out_serial[i])
                return i;
        }
        return -1;
    }

    int8_t get_global_scale_root() {
        return this->global_scale_root;
    }
    void set_global_scale_root(int8_t scale_root) {
        if (scale_root!=global_scale_root) {
            // force note off for anything currently playing, in theory so that notes don't get stuck on...
            // but in fact, we may prefer to change things around so that all quantisation is done inside midi_mapper_matrix_manager, 
            // instead of in the behaviour or wrapper..?
            //this->stop_all_notes();
            behaviour_manager_kill_all_current_notes();
        }
        this->global_scale_root = scale_root;
    }
    SCALE get_global_scale_type() {
        return this->global_scale_type;
    }
    void set_global_scale_type(SCALE scale_type) {
        if (scale_type!=global_scale_type) {
            // force note off for anything currently playing, so that notes don't get stuck on
            //this->stop_all_notes();
            behaviour_manager_kill_all_current_notes();
        }
        this->global_scale_type = scale_type;
    }
    void set_global_quantise_on(bool v) {
        this->global_quantise_on = v;
    }
    bool is_global_quantise_on() {
        return this->global_quantise_on;
    }
    void save_project_add_lines(LinkedList<String> *lines) {
        for (source_id_t source_id = 0 ; source_id < sources_count ; source_id++) {
            for (target_id_t target_id = 0 ; target_id < targets_count ; target_id++) {
                if (is_connected(source_id,target_id)) {
                    lines->add(
                        String("midi_matrix_map=")+
                        String(sources[source_id].handle) +
                        String('|') +
                        String(targets[target_id].handle)
                    );
                }
            }
        }
        lines->add(String("global_scale_type=")+String(get_global_scale_type()));
        lines->add(String("global_scale_root=")+String(get_global_scale_root()));
        lines->add(String("global_quantise_on=")+String(is_global_quantise_on()?"true":"false"));
    }

    bool load_parse_line(String line) {
        line = line.replace('\n',"");
        line = line.replace('\r',"");
        //Serial_printf("\t\tbehaviour_manager#load_parse_line() passed line \"%s\"\n", line.c_str()); Serial_flush();
        int split = line.indexOf('=');
        if (split>=0) {
            String key = line.substring(0, split);
            String value = line.substring(split+1);
            return this->load_parse_key_value(key, value);
        } else {
            return this->load_parse_key_value(line, "");
        }
    }

    bool load_parse_key_value(String key, String value) {
        if (key.equals(F("midi_matrix_map"))) {
            // midi matrix version
            Serial_printf(F("----\nLoading midi_matrix_map line '%s=%s'\n"), key.c_str(), value.c_str());
            int split = value.indexOf('|');
            String source_label = value.substring(0,split);
            String target_label = value.substring(split+1,value.length());
            this->connect(source_label.c_str(), target_label.c_str());
            return true;
        } else if (key.equals("global_scale_type")) {
            this->set_global_scale_type((SCALE)value.toInt());
            return true;
        } else if (key.equals("global_scale_root")) {
            this->set_global_scale_root(value.toInt());
            return true;
        } else if (key.equals("global_quantise_on")) {
            this->set_global_quantise_on(value.equals("true") || value.equals("on") || value.equals("1"));
            return true;
        }
        return false;
    }

    private:
        // stuff for making singleton
        static MIDIMatrixManager* inst_;
        MIDIMatrixManager() {
            //setup_midi_output_wrapper_manager();
            //memset(&sources, 0, MAX_NUM_SOURCES*sizeof(source_entry));
            sources = (source_entry*)calloc(MAX_NUM_SOURCES, sizeof(source_entry));
            //memset(sources, 0, sizeof(source_entry) * MAX_NUM_SOURCES);
            memset(disallow_map, 0, sizeof(bool)*MAX_NUM_SOURCES*MAX_NUM_TARGETS);

            set_global_scale_root_target(&this->global_scale_root);
            set_global_scale_type_target(&this->global_scale_type);
        }
        MIDIMatrixManager(const MIDIMatrixManager&);
        MIDIMatrixManager& operator=(const MIDIMatrixManager&);
};

extern MIDIMatrixManager *midi_matrix_manager;
