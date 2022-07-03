#include "behaviour_manager.h"

#include "behaviour_apcmini.h"

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

    behaviour_apcmini = new DeviceBehaviour_APCMini();
    behaviour_apcmini->loop_track = &mpk49_loop_track;

    behaviour_manager->registerDevice(behaviour_apcmini);
    /*usb_manager->registerDevice(new USBDevice_Bamble());
    usb_manager->registerDevice(new USBDevice_Beatstep());
    usb_manager->registerDevice(new USBDevice_Keystep());
    usb_manager->registerDevice(new USBDevice_MPK49());
    usb_manager->registerDevice(new USBDevice_Subclocker());*/
}
