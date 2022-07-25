#include "midi_mapper_manager.h"

#include "behaviour_bamble.h"
#include "behaviour_craftsynth.h"

#include "midi_mapper_update_wrapper_menus.h"


MIDIOutputWrapperManager *midi_output_wrapper_manager = nullptr;
MIDIOutputWrapperManager* MIDIOutputWrapperManager::inst_ = nullptr;

MIDIOutputWrapperManager* MIDIOutputWrapperManager::getInstance() {
    if (inst_ == nullptr) {
        inst_ = new MIDIOutputWrapperManager();
    }
    return inst_;
}

// initialise the output pointers, initialise the outputs and assign them to their defaults
void setup_midi_output_wrapper_manager() {
    Serial.println("setup_midi_output_wrapper_manager.."); Serial.flush();
    midi_output_wrapper_manager = MIDIOutputWrapperManager::getInstance();

    midi_output_wrapper_manager->add(new MIDIOutputWrapper((char*)"S1 : Bitbox : ch 1",  midi_out_serial[0], 1));
    midi_output_wrapper_manager->add(new MIDIOutputWrapper((char*)"S1 : Bitbox : ch 2",  midi_out_serial[0], 2));
    midi_output_wrapper_manager->add(new MIDIOutputWrapper((char*)"S1 : Bitbox : ch 3",  midi_out_serial[0], 3));
    midi_output_wrapper_manager->add(new MIDIOutputWrapper((char*)"S2 : unused : ch 1",  midi_out_serial[1], 1));
    midi_output_wrapper_manager->add(new MIDIOutputWrapper((char*)"S3 : Neutron : ch 4", midi_out_serial[2], 4));
    midi_output_wrapper_manager->add(new MIDIOutputWrapper((char*)"S4 : Disting : ch 1", midi_out_serial[3], 1));
    //MIDIOutputWrapper("Serial 4 [unused ch1]", midi_out_serial[3], 1),
    //MIDIOutputWrapper("Serial 5 [unused ch1]", midi_out_serial[4], 1),
    //MIDIOutputWrapper("Serial 6 [unused ch1]", midi_out_serial[5], 1),
    #ifdef ENABLE_BAMBLE
        midi_output_wrapper_manager->add(new MIDIOutputWrapper((char*)"USB : Bamble : ch 1", &(behaviour_bamble->device), 1));
        midi_output_wrapper_manager->add(new MIDIOutputWrapper((char*)"USB : Bamble : ch 2", &(behaviour_bamble->device), 2));
        midi_output_wrapper_manager->add(new MIDIOutputWrapper((char*)"USB : Bamble : ch 10", &(behaviour_bamble->device), 10));
    #endif

    #ifdef ENABLE_CRAFTSYNTH_USB
        midi_output_wrapper_manager->add(new MIDIOutputWrapper((char*)"USB : CraftSynth : ch 1", &(behaviour_craftsynth->device), 1));
    #endif
    #if defined(ENABLE_CRAFTSYNTH) && !defined(ENABLE_CRAFTSYNTH_USB)
        midi_output_wrapper_manager->add(new MIDIOutputWrapper((char*)"S6 : CraftSynth : ch1", midi_out_serial[5], 1));
        midi_out_serial_clock_enabled[5] = true;
    #endif

    pc_usb_1_output = midi_output_wrapper_manager->find((char*)"USB : Bamble : ch 1");
    pc_usb_2_output = midi_output_wrapper_manager->find((char*)"USB : Bamble : ch 2");

    #ifdef ENABLE_LESTRUM
        lestrum_arp_output =    midi_output_wrapper_manager->find((char*)"USB : Bamble : ch 1");
        lestrum_pads_output =   midi_output_wrapper_manager->find((char*)"USB : Bamble : ch 1");
    #endif

    #ifdef ENABLE_MPK49
        mpk49_output = midi_output_wrapper_manager->find((char*)"S1 : Bitbox : ch 3");
        mpk49_loop_track.setOutputWrapper(mpk49_output);
    #endif

    #ifdef ENABLE_BEATSTEP
        beatstep_output = midi_output_wrapper_manager->find((char*)"S3 : Neutron : ch 4");
        //while(1) {};
    #endif

    #ifdef ENABLE_KEYSTEP
        keystep_output = midi_output_wrapper_manager->find((char*)"S1 : Bitbox : ch 3");
    #endif

    #ifdef ENABLE_LOOPER
        mpk49_loop_track = MIDITrack(midi_output_wrapper_manager->find((char*)"S1 : Bitbox : ch 3"));
        drums_loop_track = MIDITrack(midi_output_wrapper_manager->find((char*)"USB : Bamble : ch 10"));
        drums_loop_track.set_quantization_value(0);
        drums_loop_track.debug = true;
    #endif
}

/*void setup_midi_input_wrapper_manager() {
    // set up the inputs...
}*/

#include "behaviour_beatstep.h"
#include "behaviour_keystep.h"
#include "behaviour_mpk49.h"

//TODO: move this into MIDIOutputWrapperManager...
// sets the designated output wrapper pointer to a different actual wrapper; takes the handle names eg 'beatstep_output' as the source
void set_target_wrapper_for_names(String source_label, String target_label) {
    Serial.printf("set_target_wrapper_for_names(%s, %s)\n", source_label.c_str(), target_label.c_str()); Serial.flush();
    MIDIOutputWrapper *target = midi_output_wrapper_manager->find((char*)target_label.c_str());
    int index = midi_output_wrapper_manager->find_index((char*)target_label.c_str());

    #ifdef ENABLE_BEATSTEP
        if (source_label.equals("beatstep_output")) {
            beatstep_setOutputWrapper(target);
            //beatstep_output_selector.actual_value_index = index;
            update_wrapper_menus_for_name(source_label, index);
            return;
        } 
    #endif
    #ifdef ENABLE_KEYSTEP
        if (source_label.equals("keystep_output")) {
            keystep_setOutputWrapper(target);
            //keystep_output_selector.actual_value_index = index;
            update_wrapper_menus_for_name(source_label, index);
            return;
        } 
    #endif
    #ifdef ENABLE_MPK49
        if (source_label.equals("mpk49_output")) {
            mpk49_setOutputWrapper(target);
            //mpk49_output_selector.actual_value_index = index;
            update_wrapper_menus_for_name(source_label, index);
            return;
        } 
    #endif
    #ifdef ENABLE_LESTRUM
        if (source_label.equals("lestrum_pads_output")) {
            lestrum_pads_setOutputWrapper(target);
            //lestrum_pads_output_selector.actual_value_index = index;
            update_wrapper_menus_for_name(source_label, index);
            return;
        } else if (source_label.equals("lestrum_arp_output")) {
            lestrum_arp_setOutputWrapper(target);
            //lestrum_arp_output_selector.actual_value_index = index;
            update_wrapper_menus_for_name(source_label, index);
            return;
        } 
    #endif
    #ifdef ENABLE_LOOPER
        if (source_label.equals("mpk49_loop_track")) {
            Serial.printf("\tsetting for 'mpk49_loop_track' with %s at %i\n", source_label.c_str(), index);
            mpk49_loop_track.setOutputWrapper(target);
            update_wrapper_menus_for_name(source_label, index);
            return;
        }
    #endif
    if (source_label.equals("pc_usb_1_output")) {
        pc_usb_1_setOutputWrapper(target);
        //pc_usb_1_selector.actual_value_index = index;
        update_wrapper_menus_for_name(source_label, index);
        return;
    } else if (source_label.equals("pc_usb_2_output")) {
        pc_usb_2_setOutputWrapper(target);
        //pc_usb_2_selector.actual_value_index = index;
        update_wrapper_menus_for_name(source_label, index);
        return;
    }

    Serial.printf("set_target_wrapper_for_names: Couldn't match source_label '%s' with anything!\n", source_label.c_str()); Serial.flush();
    update_wrapper_menus_for_name(source_label, -1);
}
