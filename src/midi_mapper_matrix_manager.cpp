#ifndef MIDI_MAPPER_MATRIX_MANAGER__INCLUDED
#define MIDI_MAPPER_MATRIX_MANAGER__INCLUDED

#include "midi_mapper_matrix_manager.h"

#include "behaviour_bamble.h"
#include "behaviour_craftsynth.h"

#include "midi_mapper_update_wrapper_menus.h"

#include "midi_pc_usb.h"

#include "midi_lestrum.h"

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
source_id_t MIDIMatrixManager::register_source(DeviceBehaviourBase *device, const char *handle) {
    strcpy(sources[sources_count].handle, handle);
    device->source_id = sources_count;
    return sources_count++;
}

void MIDIMatrixManager::connect_source_target(MIDITrack *track, DeviceBehaviourBase *device) {
    this->connect_source_target(track->source_id, device->target_id);
}
void MIDIMatrixManager::connect_source_target(DeviceBehaviourBase *device, const char *handle) {
    this->connect_source_target(device->source_id, this->get_target_id_for_handle(handle)); //this->get_target_id_for_handle->target_id);
}

// initialise the output pointers, initialise the outputs and assign them to their defaults
void setup_midi_mapper_matrix_manager() {
    Serial.println("##### setup_midi_mapper_matrix_manager.."); Serial.flush();
    midi_matrix_manager = MIDIMatrixManager::getInstance();

    // first, add all the output options that will exist

    midi_matrix_manager->register_target(new MIDIOutputWrapper((char*)"S1 : Bitbox : ch 1",  midi_out_serial[0], 1));
    midi_matrix_manager->register_target(new MIDIOutputWrapper((char*)"S1 : Bitbox : ch 2",  midi_out_serial[0], 2));
    midi_matrix_manager->register_target(new MIDIOutputWrapper((char*)"S1 : Bitbox : ch 3",  midi_out_serial[0], 3));
    midi_matrix_manager->register_target(new MIDIOutputWrapper((char*)"S2 : unused : ch 1",  midi_out_serial[1], 1));
    midi_matrix_manager->register_target(new MIDIOutputWrapper((char*)"S3 : Neutron : ch 4", midi_out_serial[2], 4));
    midi_matrix_manager->register_target(new MIDIOutputWrapper((char*)"S4 : Disting : ch 1", midi_out_serial[3], 1));
    //MIDIOutputWrapper("Serial 4 [unused ch1]", midi_out_serial[3], 1),
    //MIDIOutputWrapper("Serial 5 [unused ch1]", midi_out_serial[4], 1),
    //MIDIOutputWrapper("Serial 6 [unused ch1]", midi_out_serial[5], 1),
    #ifdef ENABLE_BAMBLE
        midi_matrix_manager->register_target(new MIDIOutputWrapper((char*)"USB : Bamble : ch 1", &(behaviour_bamble->device), 1));
        midi_matrix_manager->register_target(new MIDIOutputWrapper((char*)"USB : Bamble : ch 2", &(behaviour_bamble->device), 2));
        midi_matrix_manager->register_target(new MIDIOutputWrapper((char*)"USB : Bamble : drums",&(behaviour_bamble->device), 10));
        midi_matrix_manager->register_target(new MIDIOutputWrapper((char*)"USB : Bamble : ch 4", &(behaviour_bamble->device), 4));
    #endif

    #ifdef ENABLE_CRAFTSYNTH_USB
        midi_matrix_manager->register_target(new MIDIOutputWrapper((char*)"USB : CraftSynth : ch 1", &(behaviour_craftsynth->device), 1));
    #endif
    #if defined(ENABLE_CRAFTSYNTH) && !defined(ENABLE_CRAFTSYNTH_USB)
        midi_matrix_manager->register_target(new MIDIOutputWrapper((char*)"S6 : CraftSynth : ch1", midi_out_serial[5], 1));
        midi_out_serial_clock_enabled[5] = true;
    #endif

    // add the sources
    pc_usb_outputs[0] = midi_matrix_manager->register_source((const char*)"pc_usb_1");
    pc_usb_outputs[1] = midi_matrix_manager->register_source((const char*)"pc_usb_2");
    pc_usb_outputs[2] = midi_matrix_manager->register_source((const char*)"pc_usb_3");
    pc_usb_outputs[3] = midi_matrix_manager->register_source((const char*)"pc_usb_4");

    // then, set the output wrapper pointers to the default wrappers

    midi_matrix_manager->connect_source_target("pc_usb_1", (char*)"USB : Bamble : ch 1");
    midi_matrix_manager->connect_source_target("pc_usb_2", (char*)"USB : Bamble : ch 2");
    midi_matrix_manager->connect_source_target("pc_usb_3", (char*)"USB : Bamble : drums");
    midi_matrix_manager->connect_source_target("pc_usb_4", (char*)"USB : Bamble : ch 4");

    #ifdef ENABLE_LESTRUM
        lestrum_arp_source  = midi_matrix_manager->register_source("lestrum_arp");
        lestrum_pads_source = midi_matrix_manager->register_source("lestrum_pads");
        midi_matrix_manager->connect_source_target("lestrum_arp",  "USB : Bamble : ch 1");
        midi_matrix_manager->connect_source_target("lestrum_pads", "USB : Bamble : ch 2");
        //lestrum_arp_output =    midi_matrix_manager->find((char*)"USB : Bamble : ch 1");
        //lestrum_pads_output =   midi_matrix_manager->find((char*)"USB : Bamble : ch 1");
    #endif

    #ifdef ENABLE_MPK49
        midi_matrix_manager->register_source(behaviour_mpk49, "mpk49");
        midi_matrix_manager->connect_source_target(behaviour_mpk49, "S1 : Bitbox : ch 3");
    #endif

    #ifdef ENABLE_BEATSTEP
        // todo... transposer type..!
        midi_matrix_manager->register_source(behaviour_beatstep, "beatstep");
        //beatstep_output = midi_matrix_manager->find((char*)"S3 : Neutron : ch 4");
        midi_matrix_manager->connect_source_target(behaviour_beatstep, "S3 : Neutron : ch 4");
    #endif

    #ifdef ENABLE_KEYSTEP
        midi_matrix_manager->register_source(behaviour_keystep, "keystep");
        midi_matrix_manager->connect_source_target(behaviour_keystep, "S1 : Bitbox : ch 3");
        //keystep_output = midi_matrix_manager->find((char*)"S1 : Bitbox : ch 3");
    #endif

    // instantiate the loop tracks and point them at their default output wrappers
    #ifdef ENABLE_LOOPER
        //mpk49_loop_track = MIDITrack(midi_matrix_manager->find((char*)"S1 : Bitbox : ch 3"));
        midi_matrix_manager->register_source(&mpk49_loop_track, "loop_track_1");
        midi_matrix_manager->register_target(&mpk49_loop_track, "loop_track_1");
        midi_matrix_manager->connect_source_target("loop_track_1", "S1 : Bitbox : ch3");
        midi_matrix_manager->connect_source_target(behaviour_mpk49, "loop_track_1");
    #endif
    #ifdef ENABLE_DRUM_LOOPER
        drums_loop_track = MIDITrack(midi_output_wrapper_manager->find((char*)"USB : Bamble : drums"));
        drums_loop_track.set_quantization_value(0);
        drums_loop_track.debug = true;
    #endif

    Serial.println("##### finished setup_midi_mapper_matrix_manager"); Serial.flush();
    //while(1);
}

#endif