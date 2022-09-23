#include "behaviour_manager.h"

#include "behaviour_apcmini.h"
#include "behaviour_bamble.h"
#include "behaviour_beatstep.h"
#include "behaviour_keystep.h"
#include "behaviour_mpk49.h"
#include "behaviour_subclocker.h"
#include "behaviour_craftsynth.h"
#include "behaviour_chocolate.h"

#include "behaviour_bitbox.h"
#include "behaviour_neutron.h"
#include "behaviour_lestrum.h"
#include "behaviour_drumkit.h"

DeviceBehaviourManager *behaviour_manager = nullptr;

DeviceBehaviourManager* DeviceBehaviourManager::inst_ = nullptr;

DeviceBehaviourManager* DeviceBehaviourManager::getInstance() {
    if (inst_ == nullptr) {
        inst_ = new DeviceBehaviourManager();
    }
    return inst_;
}

void setup_behaviour_manager() {
    behaviour_manager = DeviceBehaviourManager::getInstance();

    #ifdef ENABLE_APCMINI
        behaviour_apcmini = new DeviceBehaviour_APCMini();
        #ifdef ENABLE_LOOPER
            behaviour_apcmini->loop_track = &mpk49_loop_track;
        #endif
        behaviour_manager->registerBehaviour(behaviour_apcmini);
    #endif

    #ifdef ENABLE_BAMBLE
        //behaviour_bamble = new DeviceBehaviour_Bamble();      // can be used as an output so has to be instantiated at compile time
        behaviour_manager->registerBehaviour(behaviour_bamble);
    #endif

    #ifdef ENABLE_BEATSTEP
        //behaviour_beatstep = new DeviceBehaviour_Beatstep();  // referred to by menu controls so has to be instantiated at compile time
        behaviour_manager->registerBehaviour(behaviour_beatstep);
    #endif

    #ifdef ENABLE_KEYSTEP
        behaviour_keystep = new DeviceBehaviour_Keystep();
        behaviour_manager->registerBehaviour(behaviour_keystep);
    #endif
    
    #ifdef ENABLE_MPK49
        behaviour_mpk49 = new DeviceBehaviour_mpk49();
        #ifdef ENABLE_LOOPER
            behaviour_mpk49->loop_track = &mpk49_loop_track;
        #endif
        behaviour_manager->registerBehaviour(behaviour_mpk49);
    #endif

    #ifdef ENABLE_SUBCLOCKER
        behaviour_subclocker = new DeviceBehaviour_Subclocker();
        behaviour_manager->registerBehaviour(behaviour_subclocker);
    #endif

    #ifdef ENABLE_CRAFTSYNTH_USB
        Serial.println("about to register DeviceBehaviour_CraftSynth..."); Serial.flush();
        //behaviour_craftsynth = new DeviceBehaviour_CraftSynth();  // can be used as an output so has to be instantiated at compile time
        behaviour_manager->registerBehaviour(behaviour_craftsynth);
        Serial.println("Finished registering"); Serial.flush();
    #endif

    #ifdef ENABLE_CHOCOLATEFEET_USB
        Serial.println("about to register DeviceBehaviour_Chocolate..."); Serial.flush();
        behaviour_chocolate = new DeviceBehaviour_Chocolate();
        behaviour_manager->registerBehaviour(behaviour_chocolate);
        Serial.println("Finished registering"); Serial.flush();
    #endif

    #ifdef ENABLE_BITBOX
        behaviour_manager->registerBehaviour(&behaviour_bitbox);
        behaviour_bitbox.connect_device_output(&ENABLE_BITBOX);
    #endif

    #ifdef ENABLE_BASS
        behaviour_manager->registerBehaviour(&behaviour_neutron);
        behaviour_neutron.connect_device_output(&ENABLE_BASS);
    #endif

    #ifdef ENABLE_LESTRUM
        behaviour_manager->registerBehaviour(&behaviour_lestrum);
        behaviour_lestrum.connect_device_input(&ENABLE_LESTRUM);
    #endif
    
    #ifdef ENABLE_DRUMKIT
        behaviour_manager->registerBehaviour(&behaviour_drumkit);
        behaviour_drumkit.connect_device_input(&ENABLE_DRUMKIT);
    #endif
    
    /*usb_manager->registerBehaviour(new USBDevice_Bamble());
    usb_manager->registerBehaviour(new USBDevice_Beatstep());
    usb_manager->registerBehaviour(new USBDevice_Keystep());
    usb_manager->registerBehaviour(new USBDevice_MPK49());
    usb_manager->registerBehaviour(new USBDevice_Subclocker());*/
}
