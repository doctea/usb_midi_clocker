#ifndef MIDI_MAPPER_MATRIX_MANAGER__INCLUDED
#define MIDI_MAPPER_MATRIX_MANAGER__INCLUDED

#include "Config.h"

#include "midi/midi_mapper_matrix_manager.h"

#include "behaviours/behaviour_bamble.h"
#include "behaviours/behaviour_craftsynth.h"
#include "behaviours/behaviour_mpk49.h"
#include "behaviours/behaviour_beatstep.h"
#include "behaviours/behaviour_keystep.h"

#include "behaviours/behaviour_bitbox.h"
#include "behaviours/behaviour_neutron.h"
#include "behaviours/behaviour_lestrum.h"
#include "behaviours/behaviour_drumkit.h"
#include "behaviours/behaviour_dptlooper.h"

#include "behaviours/behaviour_cvinput.h"

#include "behaviours/behaviour_opentheremin.h"

#include "midi/midi_mapper_update_wrapper_menus.h"

#include "midi/midi_pc_usb.h"

MIDIMatrixManager *midi_matrix_manager = nullptr;
MIDIMatrixManager* MIDIMatrixManager::inst_ = nullptr;

MIDIMatrixManager* MIDIMatrixManager::getInstance() {
    if (inst_ == nullptr) {
        inst_ = new MIDIMatrixManager();
    }
    return inst_;
}

source_id_t MIDIMatrixManager::register_source(MIDITrack *loop_track, const char *handle) {
    strcpy(sources[sources_count].handle, handle);
    loop_track->source_id = sources_count;
    return sources_count++;
}
source_id_t MIDIMatrixManager::register_source(DeviceBehaviourUltimateBase *device, const char *handle) {
    strcpy(sources[sources_count].handle, handle);
    device->source_id = sources_count;
    return sources_count++;
}

void MIDIMatrixManager::connect(MIDITrack *track, DeviceBehaviourUltimateBase *device) {
    this->connect(track->source_id, device->target_id);
}
void MIDIMatrixManager::connect(DeviceBehaviourUltimateBase *device, const char *handle) {
    this->connect(device->source_id, this->get_target_id_for_handle(handle)); //this->get_target_id_for_handle->target_id);
}

// initialise the output pointers, initialise the outputs and assign them to their defaults
FLASHMEM void setup_midi_mapper_matrix_manager() {
    Serial.println(F("##### setup_midi_mapper_matrix_manager..")); Serial_flush();
    midi_matrix_manager = MIDIMatrixManager::getInstance();

    // first, add all the output options that will exist

    #ifdef ENABLE_BITBOX
        midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"S1 : Bitbox : ch 1", behaviour_bitbox, 1));
        midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"S1 : Bitbox : ch 2", behaviour_bitbox, 2));
        midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"S1 : Bitbox : ch 3", behaviour_bitbox, 3));
    #endif
    midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"S2 : unused : ch 1", midi_out_serial[1], 1));
    #ifdef ENABLE_NEUTRON
        behaviour_neutron->target_id = midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"S3 : Neutron : ch 4", behaviour_neutron, 4));
    #endif
    #ifdef ENABLE_DISTING
        midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"S4 : Disting : ch 1", ENABLE_DISTING, 1));
    #endif

    /*midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"S5 : DPT : ch 1", midi_out_serial[4], 1));
    midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"S6 : DPT : ch 1", midi_out_serial[5], 1));
    midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"S7 : DPT : ch 1", midi_out_serial[6], 1));
    midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"S8 : DPT : ch 1", midi_out_serial[7], 1));*/
    #ifdef ENABLE_DPT_LOOPER
        behaviour_dptlooper->target_id = midi_matrix_manager->register_target(make_midioutputwrapper("DPT Looper", behaviour_dptlooper));
    #endif

    //MIDIOutputWrapper("Serial 4 [unused ch1]", midi_out_serial[3], 1),
    //MIDIOutputWrapper("Serial 5 [unused ch1]", midi_out_serial[4], 1),
    //MIDIOutputWrapper("Serial 6 [unused ch1]", midi_out_serial[5], 1),

    #ifdef DEFAULT_NEUTRON_OCTAVE
        midi_matrix_manager->get_target_for_handle((char*)"S3 : Neutron : ch 4")->setForceOctave(DEFAULT_NEUTRON_OCTAVE);
    #endif

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

    #ifdef ENABLE_DRUMKIT
        //drumkit_source_id = midi_matrix_manager->register_source("drumkit");
        midi_matrix_manager->register_source(behaviour_drumkit, "drumkit");
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

        // then, set the output wrapper pointers to the default wrappers (again, from PC to teensy)
        midi_matrix_manager->connect("pc_usb_1", "USB : Bamble : ch 1");
        midi_matrix_manager->connect("pc_usb_2", "USB : Bamble : ch 2");
        midi_matrix_manager->connect("pc_usb_3", "USB : Bamble : drums");
        midi_matrix_manager->connect("pc_usb_4", "USB : Bamble : ch 4");

        // other direction -- from Teensy to PC
        midi_matrix_manager->register_target(new MIDIOutputWrapper_PC((const char*)"PC : Host : 1", 0, 1));
        midi_matrix_manager->register_target(new MIDIOutputWrapper_PC((const char*)"PC : Host : 2", 1, 1));
        midi_matrix_manager->register_target(new MIDIOutputWrapper_PC((const char*)"PC : Host : 3", 2, 1));
        midi_matrix_manager->register_target(new MIDIOutputWrapper_PC((const char*)"PC : Host : 4", 3, 1));

        behaviour_beatstep->target_id = midi_matrix_manager->register_target(make_midioutputwrapper((const char*)"USB : Beatstep trans", behaviour_beatstep, 1));
    #endif

    #ifdef ENABLE_LESTRUM
        behaviour_lestrum->source_id     = midi_matrix_manager->register_source("lestrum_arp");
        behaviour_lestrum->source_id_2   = midi_matrix_manager->register_source("lestrum_pads");
        midi_matrix_manager->connect("lestrum_arp",         "USB : Bamble : ch 1");
        midi_matrix_manager->connect("lestrum_pads",        "USB : Bamble : ch 2");
    #endif

    #ifdef ENABLE_MPK49
        midi_matrix_manager->register_source(behaviour_mpk49, "mpk49");
        midi_matrix_manager->connect(behaviour_mpk49,       "S1 : Bitbox : ch 3");
    #endif

    #ifdef ENABLE_BEATSTEP
        midi_matrix_manager->register_source(behaviour_beatstep, "beatstep");
        midi_matrix_manager->connect(behaviour_beatstep,    "S3 : Neutron : ch 4");
    #endif

    #ifdef ENABLE_KEYSTEP
        midi_matrix_manager->register_source(behaviour_keystep, "keystep");
        midi_matrix_manager->connect(behaviour_keystep,     "S1 : Bitbox : ch 3");
    #endif

    // instantiate the loop tracks and point them at their default output wrappers
    #ifdef ENABLE_LOOPER
        midi_matrix_manager->register_source(&mpk49_loop_track, "loop_track_1");
        midi_matrix_manager->register_target(&mpk49_loop_track, "loop_track_1");
        midi_matrix_manager->connect("loop_track_1",            "S1 : Bitbox : ch3");
        midi_matrix_manager->connect(behaviour_mpk49,           "loop_track_1");
    #endif

    #ifdef ENABLE_DRUM_LOOPER
        midi_matrix_manager->register_source(&drums_loop_track, "drumkit");
        midi_matrix_manager->register_target(&drums_loop_track, "loop_track_drums");
        midi_matrix_manager->connect("drumkit", "USB : Bamble : drums");
        midi_matrix_manager->connect("loop_track_drums", "USB : Bamble : drums");
        drums_loop_track.set_quantization_value(0);
        //drums_loop_track.debug = true;
    #endif

    #ifdef ENABLE_CV_INPUT_PITCH
        midi_matrix_manager->register_source(behaviour_cvinput, "CV input");
    #endif

    #if defined(ENABLE_USB) && defined(ENABLE_OPENTHEREMIN)
        midi_matrix_manager->register_source(behaviour_opentheremin, "OpenTheremin");
    #endif

    Serial.println(F("##### finished setup_midi_mapper_matrix_manager")); Serial_flush();
    //while(1);
}

#endif