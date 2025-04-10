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

#include "Drums.h"

#include "LinkedList.h"

void setup_midi_mapper_matrix_manager();

void behaviour_manager_kill_all_current_notes();

#ifdef ENABLE_SCALES
    void behaviour_manager_requantise_all_notes(bool force = false);
#endif

//#include "behaviours/behaviour_bamble.h"

#define MAX_NUM_SOURCES 50
#define MAX_NUM_TARGETS 50
#define NUM_REGISTERED_TARGETS targets_count
#define NUM_REGISTERED_SOURCES sources_count

#define LANGST_HANDEL_ROUT 25   // longest possible name of a target handle / get roo to do it <3

#include "midi/midi_mapper_matrix_types.h"

#include "scales.h"

class MIDITrack;
class DeviceBehaviourUltimateBase;

class MIDIMatrixManager {
    public:
    static MIDIMatrixManager* getInstance();

    bool debug = false;

    #ifdef ENABLE_SCALES
        bool    global_quantise_on = false, global_quantise_chord_on = false;
        scale_identity_t global_scale_identity = {SCALE_MAJOR, SCALE_ROOT_C};
        chord_identity_t global_chord_identity = {CHORD::TRIAD, -1, 0};
    #endif
    /*int8_t  global_chord_degree = -1;
    CHORD::Type global_chord_type = CHORD::TRIAD;
    int8_t global_chord_inversion = 0;*/

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

    bool connect(source_id_t source_id, target_id_t target_id) {
        if (source_id<0 || target_id<0 || source_id >= NUM_REGISTERED_SOURCES || target_id >= NUM_REGISTERED_TARGETS)
            return false;
        if (!is_allowed(source_id, target_id))
            return false;
        if (!source_to_targets[source_id][target_id]) {
            // increment count if not already connected
            sources[source_id].connection_count++;
            targets[target_id].connection_count++;
        }
        source_to_targets[source_id][target_id] = true;
        return true;
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

    // process quantisation
    /*int8_t quantise_pitch(int8_t pitch) {
        return ::quantise_pitch_to_scale(pitch);
    }
    int8_t quantise_chord(int8_t pitch, int8_t distance_threshold) {
        return ::quantise_pitch_to_chord(pitch, distance_threshold);
    }*/
    #ifdef ENABLE_SCALES
        PROGMEM
        int8_t do_quant(int8_t pitch, int8_t channel) {
            if (channel!=GM_CHANNEL_DRUMS) {
                if (this->global_quantise_on)       pitch = ::quantise_pitch_to_scale(pitch);
                if (this->global_quantise_chord_on) pitch = ::quantise_pitch_to_chord(pitch, 3);
            }
            return pitch;
        }
    #endif

    ///// handle incoming or generated events (from a midi device, looper, etc) and route to connected outputs
    void processNoteOn(source_id_t source_id, int8_t pitch, uint8_t velocity, uint8_t channel = 0) {
        if (this->debug) Serial_printf(F("midi_mapper_matrix_manager#processNoteOn(source_id=%i (%s),\tpitch=%i (%s),\tvelocity=%i,\tchannel=%i)\n"), source_id, this->sources[source_id].handle, pitch, get_note_name_c(pitch), velocity, channel);

        if (source_id<0) {
            if (this->debug) Serial_printf(F("!! midi_mapper_matrix_manager#processNoteOn() passed source_id of %i!\n"), source_id);
            return;
        }
        if (!is_valid_note(pitch)) { 
            if (this->debug) Serial_printf("!! midi_mapper_matrix_manager#processNoteOff() passed invalid pitch %s (%i) - ignoring\n", get_note_name_c(pitch), pitch);
            return;
        }

        /*if (debug) Serial_printf(F("!! midi_mapper_matrix_manager#processNoteOn(source_id=%i,\tpitch=%i (%s),\tvelocity=%i,\tchannel=%i) "), source_id, pitch, get_note_name_c(pitch), velocity, channel);
        pitch = do_quant(pitch, channel); 
        if (debug) Serial_printf(F("=> quantised to %i (%s)\n"), pitch, get_note_name_c(pitch));
        if (!is_valid_note(pitch)) return;*/

        for (target_id_t target_id = 0 ; target_id < NUM_REGISTERED_TARGETS ; target_id++) {
            if (is_connected(source_id, target_id)) {
                //targets[target_id].wrapper->debug = true;
                if (this->debug) Serial_printf(F("\tsource %i aka %s\tshould send ON  to\t%i aka %s\t(source_id=%i)\n"), source_id, sources[source_id].handle, target_id, targets[target_id].handle);
                targets[target_id].wrapper->sendNoteOn(pitch, velocity, channel);
                //targets[target_id].wrapper->debug = false;
            }
        }
    }
    void processNoteOff(source_id_t source_id, int8_t pitch, uint8_t velocity, uint8_t channel = 0) {
        if (this->debug) Serial_printf(F("midi_mapper_matrix_manager#processNoteOff(source_id=%i,\tpitch=%i,\tvelocity=%i,\tchannel=%i)\n"), source_id, pitch, velocity, channel);

        if (source_id<0) {
            if (this->debug) Serial_printf(F("\t!! midi_mapper_matrix_manager#processNoteOff() passed source_id of %i!\n"), source_id);
            return;
        }
        if (!is_valid_note(pitch)) {
            if (this->debug) Serial_printf("midi_mapper_matrix_manager#processNoteOff() passed invalid pitch %s (%i) - ignoring\n", get_note_name_c(pitch), pitch);
            return;
        }

        /*if (debug) Serial_printf(F("midi_mapper_matrix_manager#processNoteOff(source_id=%i,\tpitch=%i (%s),\tvelocity=%i,\tchannel=%i) "), source_id, pitch, get_note_name_c(pitch), velocity, channel);
        pitch = do_quant(pitch, channel);
        if (debug) Serial_printf(F("=> quantised to %i (%s)\n"), pitch, get_note_name_c(pitch));*/

        if (!is_valid_note(pitch)) {
            if (this->debug) Serial_printf("midi_mapper_matrix_manager#processNoteOff() requantised pitch to %s (%i) - so is now invalid - so ignoring!\n", get_note_name_c(pitch), pitch);
            return;
        }

        for (target_id_t target_id = 0 ; target_id < NUM_REGISTERED_TARGETS ; target_id++) {
            if (is_connected(source_id, target_id)) {
                //targets[target_id].wrapper->debug = true;
                if (this->debug) Serial_printf(F("\t%s\tshould send to\t%s\t(target_id=%i)\n"), sources[source_id].handle, targets[target_id].handle, target_id);
                if (this->debug) Serial_printf(F("\tsource %i aka %s\tshould send OFF to\t%i aka %s\t(source_id=%i)\n"), source_id, sources[source_id].handle, target_id, targets[target_id].handle);
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

    #ifdef ENABLE_SCALES
        /////// scale and quantisation functions
        // getters & setters for quantisation on/offs
        void set_global_quantise_on(bool v, bool requantise_immediately = true) {
            bool changed = v != this->global_quantise_on;
            this->global_quantise_on = v;
            if (changed && requantise_immediately) {
                //Serial_println(F("set_global_quantise_on() about to call behaviour_manager_requantise_all_notes()"));
                behaviour_manager_requantise_all_notes();
            }
        }
        bool is_global_quantise_on() {
            return this->global_quantise_on;
        }

        void set_global_quantise_chord_on(bool v, bool requantise_immediately = true) {
            bool changed = v != this->global_quantise_chord_on;
            this->global_quantise_chord_on = v;
            if (changed && requantise_immediately) {
                //Serial_println(F("set_global_quantise_chord_on() about to call behaviour_manager_requantise_all_notes()"));
                behaviour_manager_requantise_all_notes();
            }
        }
        bool is_global_quantise_chord_on() {
            return this->global_quantise_chord_on;
        }

        // getters & setters for global scale + global chord settings
        chord_identity_t get_global_chord() {
            return chord_identity_t {
                this->global_chord_identity.type,
                this->global_chord_identity.degree,
                this->global_chord_identity.inversion
            };
        }
        void set_global_chord(chord_identity_t chord, bool requantise_immediately = true) {
            bool changed = chord.diff(this->global_chord_identity);
            this->global_chord_identity = chord;
            if (changed && requantise_immediately) {
                //Serial_println(F("set_global_chord() about to call behaviour_manager_requantise_all_notes()"));
                behaviour_manager_requantise_all_notes();
            }
        }

        int8_t get_global_scale_root() {
            return this->global_scale_identity.root_note;
        }
        void set_global_scale_root(int8_t scale_root, bool requantise_immediately = true) {
            if (debug) Serial_printf("midi_mapper_matrix_manager#set_global_scale_root(%i) currently has global_scale_identity.root_note=%i\n", scale_root, this->global_scale_identity.root_note);
            bool changed = scale_root != global_scale_identity.root_note;
            if (changed && debug) {
                Serial_printf("======== midi_mapper_matrix_manager#set_global_scale_root() changing %s (%i) to %s (%i)\n", get_note_name_c(global_scale_identity.root_note), global_scale_identity.root_note, get_note_name_c(scale_root), scale_root);
                Serial_printf("Current scale:\t");
                print_scale(global_scale_identity.root_note, global_scale_identity.scale_number);
                Serial_printf("New scale:\t");
                print_scale(global_scale_identity.root_note, global_scale_identity.scale_number);
            }
            this->global_scale_identity.root_note = scale_root;
            if (changed && requantise_immediately) {
                if (debug) Serial_println(F("set_global_scale_root() about to call behaviour_manager_requantise_all_notes()"));
                behaviour_manager_requantise_all_notes();
                if (debug) Serial_printf("======= midi_mapper_matrix_manager#set_global_scale_root() done requantising all notes\n");
            }
        }

        scale_index_t get_global_scale_type() {
            return this->global_scale_identity.scale_number;
        }
        void set_global_scale_type(scale_index_t scale_type, bool requantise_immediately = true) {
            bool changed = scale_type!=global_scale_identity.scale_number;
            this->global_scale_identity.scale_number = scale_type;
            if (changed && requantise_immediately) {
                //Serial_println(F("set_global_scale_type() about to call behaviour_manager_requantise_all_notes()"));
                behaviour_manager_requantise_all_notes();
            }
        }

        int8_t get_global_chord_degree() {
            return this->global_chord_identity.degree;
        }
        void set_global_chord_degree(int8_t degree, bool requantise_immediately = true) {
            bool changed = degree != get_global_chord_degree();
            this->global_chord_identity.degree = degree;
            if (changed && requantise_immediately) {
                Serial_println(F("set_global_chord_degree() about to call behaviour_manager_requantise_all_notes()"));
                behaviour_manager_requantise_all_notes();
            }
        }

        CHORD::Type get_global_chord_type() {
            return this->global_chord_identity.type;
        }
        void set_global_chord_type(CHORD::Type chord_type, bool requantise_immediately = true) {
            bool changed = chord_type != get_global_chord_type();
            this->global_chord_identity.type = chord_type;
            if (changed && requantise_immediately) {
                Serial_println(F("set_global_chord_type() about to call behaviour_manager_requantise_all_notes()"));
                behaviour_manager_requantise_all_notes();
            }
        }

        int8_t get_global_chord_inversion() {
            return this->global_chord_identity.inversion;
        }
        void set_global_chord_inversion(int8_t inversion, bool requantise_immediately = true) {
            bool changed = inversion != get_global_chord_inversion();
            this->global_chord_identity.inversion = inversion;
            if (changed && requantise_immediately) {
                Serial_println(F("set_global_chord_inversion() about to call behaviour_manager_requantise_all_notes()"));
                behaviour_manager_requantise_all_notes();
            }
        }
    #endif

    ////////// save and load 
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
        #ifdef ENABLE_SCALES
            lines->add(String("global_scale_type=")+String(get_global_scale_type()));
            lines->add(String("global_scale_root=")+String(get_global_scale_root()));
            lines->add(String("global_quantise_on=")+String(is_global_quantise_on()?"true":"false"));
            lines->add(String("global_quantise_chord_on=")+String(is_global_quantise_chord_on()?"true":"false"));
        #endif
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
        }
        #ifdef ENABLE_SCALES
            else if (key.equals("global_scale_type")) {
                this->set_global_scale_type((scale_index_t)value.toInt());
                return true;
            } else if (key.equals("global_scale_root")) {
                this->set_global_scale_root(value.toInt());
                return true;
            } else if (key.equals("global_quantise_on")) {
                this->set_global_quantise_on(value.equals("true") || value.equals("on") || value.equals("1"));
                return true;
            } else if (key.equals("global_quantise_chord_on")) {
                this->set_global_quantise_chord_on(value.equals("true") || value.equals("on") || value.equals("1"));
                return true;
            }
        #endif
        return false;
    }

    private:
        // stuff for making singleton
        static MIDIMatrixManager* inst_;
        MIDIMatrixManager() {
            //setup_midi_output_wrapper_manager();
            //memset(&sources, 0, MAX_NUM_SOURCES*sizeof(source_entry));
            sources = (source_entry*)CALLOC_FUNC(MAX_NUM_SOURCES, sizeof(source_entry));
            //memset(sources, 0, sizeof(source_entry) * MAX_NUM_SOURCES);
            memset(disallow_map, 0, sizeof(bool)*MAX_NUM_SOURCES*MAX_NUM_TARGETS);

            #ifdef ENABLE_SCALES
                set_global_scale_identity_target(&this->global_scale_identity);
                set_global_chord_identity_target(&this->global_chord_identity);
            #endif
        }
        MIDIMatrixManager(const MIDIMatrixManager&);
        MIDIMatrixManager& operator=(const MIDIMatrixManager&);
};

extern MIDIMatrixManager *midi_matrix_manager;
