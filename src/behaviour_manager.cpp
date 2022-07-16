#include "behaviour_manager.h"

#include "behaviour_apcmini.h"
#include "behaviour_bamble.h"
#include "behaviour_beatstep.h"
#include "behaviour_keystep.h"
#include "behaviour_mpk49.h"
#include "behaviour_subclocker.h"
#include "behaviour_craftsynth.h"

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
        behaviour_apcmini->loop_track = &mpk49_loop_track;
        behaviour_manager->registerDevice(behaviour_apcmini);
    #endif

    #ifdef ENABLE_BAMBLE
        //behaviour_bamble = new DeviceBehaviour_Bamble();      // can be used as an output so has to be instantiated at compile time
        behaviour_manager->registerDevice(behaviour_bamble);
    #endif

    #ifdef ENABLE_BEATSTEP
        behaviour_beatstep = new DeviceBehaviour_Beatstep();
        behaviour_manager->registerDevice(behaviour_beatstep);
    #endif

    #ifdef ENABLE_KEYSTEP
        behaviour_keystep = new DeviceBehaviour_Keystep();
        behaviour_manager->registerDevice(behaviour_keystep);
    #endif
    
    #ifdef ENABLE_MPK49
        behaviour_mpk49 = new DeviceBehaviour_mpk49();
        behaviour_mpk49->loop_track = &mpk49_loop_track;
        behaviour_manager->registerDevice(behaviour_mpk49);
    #endif

    #ifdef ENABLE_SUBCLOCKER
        behaviour_subclocker = new DeviceBehaviour_Subclocker();
        behaviour_manager->registerDevice(behaviour_subclocker);
    #endif

    #ifdef ENABLE_CRAFTSYNTH
        Serial.println("about to register DeviceBehaviour_CraftSynth..."); Serial.flush();
        //behaviour_craftsynth = new DeviceBehaviour_CraftSynth();  // can be used as an output so has to be instantiated at compile time
        behaviour_manager->registerDevice(behaviour_craftsynth);
        Serial.println("Finished registering"); Serial.flush();
    #endif
    
    /*usb_manager->registerDevice(new USBDevice_Bamble());
    usb_manager->registerDevice(new USBDevice_Beatstep());
    usb_manager->registerDevice(new USBDevice_Keystep());
    usb_manager->registerDevice(new USBDevice_MPK49());
    usb_manager->registerDevice(new USBDevice_Subclocker());*/
}
