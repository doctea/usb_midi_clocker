
#include "Config.h"

#include "project.h"

#include "midi/midi_mapper_matrix_manager.h"

#include "behaviours/behaviour_bamble.h"
#include "behaviours/behaviour_craftsynth.h"
#include "behaviours/behaviour_skulptsynth.h"
#include "behaviours/behaviour_mpk49.h"
#include "behaviours/behaviour_beatstep.h"
#include "behaviours/behaviour_keystep.h"

#include "behaviours/behaviour_bitbox.h"
#include "behaviours/behaviour_neutron.h"
#include "behaviours/behaviour_lestrum.h"
#include "behaviours/behaviour_drumkit.h"
#include "behaviours/behaviour_dptlooper.h"
#include "behaviours/behaviour_midimuso_4pv.h"
#include "behaviours/behaviour_midimuso_4mv.h"

#include "behaviours/behaviour_microlidian.h"

#include "behaviours/behaviour_cvinput.h"
#include "behaviours/behaviour_cvoutput.h"

#include "behaviours/behaviour_opentheremin.h"
#include "behaviours/behaviour_midibassproxy.h"
#include "behaviours/behaviour_arpeggiator.h"

#include "behaviours/behaviour_gate_sequencer.h"

#include "behaviours/behaviour_apcmini.h"

#include "behaviours/behaviour_euclidianrhythms.h"

#include "behaviours/behaviour_progression.h"

#include "midi/midi_mapper_update_wrapper_menus.h"

#include "midi/midi_looper.h"
#include "midi/midi_pc_usb.h"

MIDIMatrixManager *midi_matrix_manager = nullptr;
MIDIMatrixManager* MIDIMatrixManager::inst_ = nullptr;

MIDIMatrixManager* MIDIMatrixManager::getInstance() {
    if (inst_ == nullptr) {
        inst_ = new MIDIMatrixManager();
    }
    return inst_;
}

#ifdef ENABLE_LOOPER
source_id_t MIDIMatrixManager::register_source(MIDITrack *loop_track, const char *handle) {
    if (loop_track==nullptr) return -1;
    strncpy(sources[sources_count].handle, handle, LANGST_HANDEL_ROUT);
    loop_track->source_id = sources_count;
    return sources_count++;
}
#endif
source_id_t MIDIMatrixManager::register_source(DeviceBehaviourUltimateBase *device, const char *handle) {
    if (device==nullptr) return -1;
    strncpy(sources[sources_count].handle, handle, LANGST_HANDEL_ROUT);
    device->source_id = sources_count;
    return sources_count++;
}

#ifdef ENABLE_LOOPER
void MIDIMatrixManager::connect(MIDITrack *track, DeviceBehaviourUltimateBase *device) {
    if (track==nullptr || device==nullptr) return;
    this->connect(track->source_id, device->target_id);
}
#endif
void MIDIMatrixManager::connect(DeviceBehaviourUltimateBase *device, const char *handle) {
    if (device==nullptr || handle==nullptr) return;
    this->connect(device->source_id, this->get_target_id_for_handle(handle)); //this->get_target_id_for_handle->target_id);
}

// initialise the output pointers, initialise the outputs and assign them to their defaults
//FLASHMEM 
void setup_midi_mapper_matrix_manager() {
    //Serial.println(F("##### setup_midi_mapper_matrix_manager..")); Serial_flush();
    midi_matrix_manager = MIDIMatrixManager::getInstance();


    #ifdef ENABLE_PROGRESSION
        behaviour_progression->source_id            = midi_matrix_manager->register_source("Progression");
        behaviour_progression->source_id_chord_octave = midi_matrix_manager->register_source("Prog.chord");
        behaviour_progression->source_id_bass       = midi_matrix_manager->register_source("Prog.Bass");
        behaviour_progression->source_id_topline    = midi_matrix_manager->register_source("Prog.Top");
    #endif

    // for remembering which serial midi connections are mapped to defined devices
    bool midi_out_used[NUM_MIDI_OUTS] = {false, false, false, false, false, false, false, false};
    bool midi_in_used[NUM_MIDI_OUTS] = {false, false, false, false, false, false, false, false};

    for (int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
        char label[30];

        Serial.printf("Checking/setting up midi_out_serial[%i].. ", i); Serial_flush();
        //Serial.printf("midi_out_serial[%i] = %p\n", i, midi_out_serial[i]); Serial_flush();
        //Serial.printf("ENABLE_BITBOX = %p\n", &ENABLE_BITBOX); Serial_flush();

        // first check all the possible serial output devices
        #ifdef ENABLE_BITBOX
            if (&ENABLE_BITBOX==midi_out_serial[i]) {
                midi_out_used[i] = true;
                midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"S1 : Bitbox : ch 1",  behaviour_bitbox, 1));
                midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"S1 : Bitbox : ch 2",  behaviour_bitbox, 2));
                midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"S1 : Bitbox : ch 3",  behaviour_bitbox, 3));
                midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"S1 : Bitbox : ch 10", behaviour_bitbox, 10));    
            }
        #endif
        #ifdef ENABLE_NEUTRON
            if (&ENABLE_NEUTRON==midi_out_serial[i]) {
                midi_out_used[i] = true;
                behaviour_neutron->target_id = midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"S3 : Neutron : ch 1", behaviour_neutron));
            }
        #endif
        #ifdef ENABLE_MAMMB33
            if (&ENABLE_MAMMB33==midi_out_serial[i]) {
                midi_out_used[i] = true;
                midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"S2 : MAM MB33 : ch 1", &ENABLE_MAMMB33, 1)); // for MB33
            }
        #endif
        #ifdef ENABLE_DISTING
            if (&ENABLE_DISTING==midi_out_serial[i]) {
                midi_out_used[i] = true;
                midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"S4 : Disting : ch 1", ENABLE_DISTING, 1));
            }
        #endif
        #ifdef ENABLE_BEHRINGER_EDGE_SERIAL
            if (&ENABLE_BEHRINGER_EDGE_SERIAL==midi_out_serial[i]) {
                midi_out_used[i] = true;
                // clock only so don't register as a target
            }
        #endif
        #ifdef ENABLE_BEHRINGER_EDGE_DEDICATED
            if (&ENABLE_BEHRINGER_EDGE_DEDICATED==midi_out_serial[i]) {
                midi_out_used[i] = true;
                // clock only so don't register as a target
            }
        #endif
        #ifdef ENABLE_MIDIMUSO_4MV
            if (&ENABLE_MIDIMUSO_4MV==midi_out_serial[i]) {
                midi_out_used[i] = true;
                behaviour_midimuso_4mv->voice_target_id[0] = midi_matrix_manager->register_target(make_midioutputwrapper("MIDIMuso-4MV Out 1", behaviour_midimuso_4mv, 1));
                behaviour_midimuso_4mv->voice_target_id[1] = midi_matrix_manager->register_target(make_midioutputwrapper("MIDIMuso-4MV Out 2", behaviour_midimuso_4mv, 2));
                behaviour_midimuso_4mv->voice_target_id[2] = midi_matrix_manager->register_target(make_midioutputwrapper("MIDIMuso-4MV Out 3", behaviour_midimuso_4mv, 3));
                behaviour_midimuso_4mv->voice_target_id[3] = midi_matrix_manager->register_target(make_midioutputwrapper("MIDIMuso-4MV Out 4", behaviour_midimuso_4mv, 4));
                behaviour_midimuso_4mv->target_id = midi_matrix_manager->register_target(make_midioutputwrapper("MIDIMuso-4MV Auto",  behaviour_midimuso_4mv, 5));    
            }
        #endif
        #ifdef ENABLE_MIDIMUSO_4PV
            if (&ENABLE_MIDIMUSO_4PV==midi_out_serial[i]) {
                midi_out_used[i] = true;
                behaviour_midimuso_4pv->target_id = midi_matrix_manager->register_target(make_midioutputwrapper("MIDIMUSO-PV", (DeviceBehaviourUltimateBase *)behaviour_midimuso_4pv, (byte)1, (int8_t)4));
            }
        #endif
        #ifdef ENABLE_SKULPTSYNTH_SERIAL
            if (&ENABLE_SKULPTSYNTH_SERIAL==midi_out_serial[i]) {
                midi_out_used[i] = true;
                snprintf(label, 30, "S%i : SkulptSynth : ch 1", i+1);
                behaviour_skulptsynth->target_id = midi_matrix_manager->register_target(make_midioutputwrapper(label, behaviour_skulptsynth, 1));
            }
        #endif
        #ifdef ENABLE_DPT_LOOPER
            if (&ENABLE_DPT_LOOPER==midi_out_serial[i]) {
                midi_out_used[i] = true;
                behaviour_dptlooper->target_id = midi_matrix_manager->register_target(make_midioutputwrapper("DPT Looper", behaviour_dptlooper));
            }
        #endif

        // if MIDI slot not used, add the default output
        if (!midi_out_used[i]) {
            snprintf(label, 30, "S%i : MIDIOUT : ch 1", i+1);
            midi_matrix_manager->register_target(make_midioutputwrapper(label, midi_out_serial[i], 1));
        }

        // then check all the possible serial input devices
        #ifdef ENABLE_LESTRUM
            if (&ENABLE_LESTRUM==midi_out_serial[i]) {
                midi_in_used[i] = true;
                behaviour_lestrum->source_id     = midi_matrix_manager->register_source("lestrum_arp");
                behaviour_lestrum->source_id_2   = midi_matrix_manager->register_source("lestrum_pads");
            }
        #endif
        #ifdef ENABLE_DRUMKIT
            if (&ENABLE_DRUMKIT==midi_out_serial[i]) {
                midi_in_used[i] = true;
            }
        #endif

        // if MIDI slot not used, add the default input
        if (!midi_in_used[i]) {
            snprintf(label, 30, "midi_in_%i", i+1);
            midi_matrix_manager->register_source(label);
        }
    }

    behaviour_sequencer_gates->target_id = midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"Seq. Gate Drums", behaviour_sequencer_gates, 10));

    /*for (int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
        char label[30];
        if (!midi_out_used[i]) {
            snprintf(label, 30, "S%i : MIDIOUT : ch 1", i+1);
            midi_matrix_manager->register_target(make_midioutputwrapper(label, midi_out_serial[i], 1));
        }
        if (!midi_in_used[i]) {
            snprintf(label, 30, "midi_in_%i", i+1);
            midi_matrix_manager->register_source(label);
        }
    }*/

    #if defined(ENABLE_BAMBLE) && defined(ENABLE_BAMBLE_OUTPUT)
        behaviour_bamble->self_register_midi_matrix_targets(midi_matrix_manager);
    #endif

    #ifdef ENABLE_CRAFTSYNTH_USB
        behaviour_craftsynth->target_id = midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"USB : CraftSynth : ch 1", behaviour_craftsynth, 1));
    #endif
    #if defined(ENABLE_CRAFTSYNTH) && !defined(ENABLE_CRAFTSYNTH_USB)
        midi_matrix_manager->register_target(new MIDIOutputWrapper((char*)"S6 : CraftSynth : ch1", midi_out_serial[5], 1));
        midi_out_serial_clock_enabled[5] = true;
    #endif

    #ifdef ENABLE_SKULPTSYNTH_USB
        behaviour_skulptsynth->target_id = midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"USB : SkulptSynth : ch 1", behaviour_skulptsynth, 1));
    #endif

    #if defined(ENABLE_BAMBLE) && defined(ENABLE_BAMBLE_INPUT)
        behaviour_bamble->self_register_midi_matrix_sources(midi_matrix_manager);
    #endif

    #ifdef ENABLE_USB
        // add the sources - these are *from* PC *to* Teensy
        pc_usb_sources[0] = midi_matrix_manager->register_source((const char*)"pc_usb_1");
        pc_usb_sources[1] = midi_matrix_manager->register_source((const char*)"pc_usb_2");
        pc_usb_sources[2] = midi_matrix_manager->register_source((const char*)"pc_usb_3");
        pc_usb_sources[3] = midi_matrix_manager->register_source((const char*)"pc_usb_4");

        #ifdef ENABLE_BAMBLE
            // then, set the output wrapper pointers to the default wrappers (again, from PC to teensy)
            midi_matrix_manager->connect("pc_usb_1", "USB : Bamble : ch 1");
            midi_matrix_manager->connect("pc_usb_2", "USB : Bamble : ch 2");
            midi_matrix_manager->connect("pc_usb_3", "USB : Bamble : drums");
            midi_matrix_manager->connect("pc_usb_4", "USB : Bamble : ch 4");
        #endif

        // other direction -- from Teensy to PC
        midi_matrix_manager->register_target(new MIDIOutputWrapper_PC((const char*)"PC : Host : 1", 0, 1));
        midi_matrix_manager->register_target(new MIDIOutputWrapper_PC((const char*)"PC : Host : 2", 1, 1));
        midi_matrix_manager->register_target(new MIDIOutputWrapper_PC((const char*)"PC : Host : 3", 2, 1));
        midi_matrix_manager->register_target(new MIDIOutputWrapper_PC((const char*)"PC : Host : 4", 3, 1));

        #ifdef ENABLE_BEATSTEP
            behaviour_beatstep->target_id = midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"USB : Beatstep trans", behaviour_beatstep, 1));
        #endif
    #endif

    #ifdef ENABLE_MPK49
        midi_matrix_manager->register_source(behaviour_mpk49, "mpk49");
        midi_matrix_manager->connect(behaviour_mpk49,       "S1 : Bitbox : ch 3");
    #endif

    #ifdef ENABLE_BEATSTEP
        midi_matrix_manager->register_source(behaviour_beatstep,    "beatstep");
        #ifdef ENABLE_BEATSTEP_2
            midi_matrix_manager->register_source(behaviour_beatstep_2,  "beatstep#2");
        #endif
        midi_matrix_manager->connect(behaviour_beatstep,    "S3 : Neutron : ch 4");
    #endif

    #ifdef ENABLE_KEYSTEP
        midi_matrix_manager->register_source(behaviour_keystep, "keystep");
        midi_matrix_manager->connect(behaviour_keystep,     "S1 : Bitbox : ch 3");
    #endif

    // instantiate the loop tracks and point them at their default output wrappers
    #ifdef ENABLE_LOOPER
        midi_matrix_manager->register_source(&midi_loop_track, "loop_track_1");
        midi_matrix_manager->register_target(&midi_loop_track, "loop_track_1");
        midi_matrix_manager->connect("loop_track_1",            "S1 : Bitbox : ch 3");
        #ifdef ENABLE_MPK49
            midi_matrix_manager->connect(behaviour_mpk49,           "loop_track_1");
        #endif
    #endif

    #ifdef ENABLE_DRUM_LOOPER
        midi_matrix_manager->register_source(&drums_loop_track, "drumkit");
        midi_matrix_manager->register_target(&drums_loop_track, "loop_track_drums");
        midi_matrix_manager->connect("drumkit", "USB : Bamble : drums");
        midi_matrix_manager->connect("loop_track_drums", "USB : Bamble : drums");
        drums_loop_track.set_quantization_value(0);
        //drums_loop_track.debug = true;
    #endif

    #ifdef ENABLE_MICROLIDIAN
        behaviour_microlidian->source_id     = midi_matrix_manager->register_source("ulidian ch10");
        behaviour_microlidian->source_id_2   = midi_matrix_manager->register_source("ulidian ch1");
        //behaviour_microlidian->target_id     = midi_matrix_manager->register_target(behaviour_microlidian, "ulidian ch10");
        /*#ifdef ENABLE_DRUMKIT
            // TODO: connect drumkit input to ulidian output
            midi_matrix_manager->connect(behaviour_drumkit, "ulidian ch10");
        #endif*/
        #ifdef ENABLE_BAMBLE
            midi_matrix_manager->connect("ulidian ch10", "USB : Bamble : drums");
        #endif
    #endif

    #ifdef ENABLE_EUCLIDIAN
        behaviour_euclidianrhythms->source_id   = midi_matrix_manager->register_source("EucRhythms ch10");
        behaviour_euclidianrhythms->source_id_2 = midi_matrix_manager->register_source("EucRhythms ch1");
        behaviour_euclidianrhythms->source_id_3 = midi_matrix_manager->register_source("EucRhythms ch8");
        behaviour_euclidianrhythms->source_id_4 = midi_matrix_manager->register_source("EucRhythms ch9");
        //Serial.printf("ENABLE_EUCLIDIAN: connecting source_id=%i to target_id=%i\n", behaviour_euclidianrhythms->source_id, behaviour_sequencer_gates->target_id);
        midi_matrix_manager->connect(behaviour_euclidianrhythms->source_id, behaviour_sequencer_gates->target_id);
    #endif

    #if defined(ENABLE_APCMINI) && defined(ENABLE_APCMINI_PADS)
        midi_matrix_manager->register_source(behaviour_apcmini, "APCMini Pads");
    #endif

    #if defined(ENABLE_CV_INPUT) && defined(ENABLE_CV_INPUT_PITCH)
        midi_matrix_manager->register_source(behaviour_cvinput_1, "CV input 1");
        midi_matrix_manager->register_source(behaviour_cvinput_2, "CV input 2");
        midi_matrix_manager->register_source(behaviour_cvinput_3, "CV input 3");
        //#ifdef ENABLE_CRAFTSYNTH_USB
        //    midi_matrix_manager->connect("CV input", "USB : CraftSynth : ch 1");
        //#endif
    #endif

    #if defined(ENABLE_USB) && defined(ENABLE_OPENTHEREMIN)
        midi_matrix_manager->register_source(behaviour_opentheremin, "OpenTheremin");
    #endif

    #ifdef ENABLE_CV_OUTPUT
        // add the CV output midi targets
        for (size_t i = 0 ; i < cvoutput_configs_size ; i++) {
            cvoutput_config_t config = cvoutput_configs[i];
            DeviceBehaviour_CVOutput<DAC8574> *behaviour_cvoutput = config.behaviour;
            behaviour_cvoutput->target_id = midi_matrix_manager->register_target(make_midioutputwrapper((String("CV Output ")+String(i)+String(" Auto")).c_str(), behaviour_cvoutput, behaviour_cvoutput->CHANNEL_ROUND_ROBIN));
            behaviour_cvoutput->voice_target_id[0] = midi_matrix_manager->register_target(make_midioutputwrapper((String("CV Output ")+String(i)+String(" A")).c_str(), behaviour_cvoutput, 1));
            behaviour_cvoutput->voice_target_id[1] = midi_matrix_manager->register_target(make_midioutputwrapper((String("CV Output ")+String(i)+String(" B")).c_str(), behaviour_cvoutput, 2));
            behaviour_cvoutput->voice_target_id[2] = midi_matrix_manager->register_target(make_midioutputwrapper((String("CV Output ")+String(i)+String(" C")).c_str(), behaviour_cvoutput, 3));
            behaviour_cvoutput->voice_target_id[3] = midi_matrix_manager->register_target(make_midioutputwrapper((String("CV Output ")+String(i)+String(" D")).c_str(), behaviour_cvoutput, 4));
        }
    #endif

    // set up 'Bass Proxy' behaviour
    midi_matrix_manager->register_source(behaviour_midibassproxy, "Bass Proxy");
    MIDIOutputWrapper *wrapper = make_midioutputwrapper("Bass Proxy", behaviour_midibassproxy);
    behaviour_midibassproxy->target_id = midi_matrix_manager->register_target(wrapper, "Bass Proxy");
    behaviour_midibassproxy->setHighestNote(4*12);
    behaviour_midibassproxy->setHighestNoteMode(NOTE_MODE::TRANSPOSE);
    behaviour_midibassproxy->setLowestNote(1*12);
    behaviour_midibassproxy->setLowestNoteMode(NOTE_MODE::TRANSPOSE);

    midi_matrix_manager->register_source(behaviour_arpeggiator, "Arpeggiator");
    //MIDIOutputWrapper *arp_wrapper = make_midioutputwrapper("Arp Chords", behaviour_arpeggiator);
    behaviour_arpeggiator->target_id = midi_matrix_manager->register_target(
        make_midioutputwrapper("Arp Chords", behaviour_arpeggiator),
        "Arp Chords"
    );
    behaviour_arpeggiator->target_id_trigger = midi_matrix_manager->register_target(
        make_midioutputwrapper("Arp Trigger", behaviour_arpeggiator, 2), 
        "Arp Trigger"
    );

    // this default connection doesn't actually work, i think because cos its overridden by loading project settings
    //midi_matrix_manager->connect(behaviour_progression, "CV Output 1 Auto");

    // connect default mappings
    #ifdef ENABLE_MAMMB33
        midi_matrix_manager->connect("Bass Proxy", "S2 : MAM MB33 : ch 1");
    #else
        //midi_matrix_manager->connect("Bass Proxy", "S2 : MIDIOUT : ch 1");
    #endif
    #ifdef ENABLE_BEATSTEP
        midi_matrix_manager->connect("beatstep", "Bass Proxy");
    #endif
    #ifdef DEBUG_MIDIBASS
        behaviour_midibassproxy->debug = wrapper->debug = true; // debug switch for machinegun not working?!
    #endif
    midi_matrix_manager->disallow(behaviour_midibassproxy->source_id, behaviour_midibassproxy->target_id);  // don't allow it to connect to itself

    //Serial.println(F("##### finished setup_midi_mapper_matrix_manager")); Serial_flush();
    //while(1);
}
