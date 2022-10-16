#include "behaviours/behaviour_manager.h"

#include "behaviours/behaviour_apcmini.h"
#include "behaviours/behaviour_bamble.h"
#include "behaviours/behaviour_beatstep.h"
#include "behaviours/behaviour_keystep.h"
#include "behaviours/behaviour_mpk49.h"
#include "behaviours/behaviour_subclocker.h"
#include "behaviours/behaviour_craftsynth.h"
#include "behaviours/behaviour_chocolate.h"

#include "behaviours/behaviour_bitbox.h"
#include "behaviours/behaviour_neutron.h"
#include "behaviours/behaviour_lestrum.h"
#include "behaviours/behaviour_drumkit.h"

#include "behaviours/behaviour_cvinput.h"

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
        Serial.println(F("about to register DeviceBehaviour_CraftSynth...")); Serial.flush();
        //behaviour_craftsynth = new DeviceBehaviour_CraftSynth();  // can be used as an output so has to be instantiated at compile time
        behaviour_manager->registerBehaviour(behaviour_craftsynth);
        Serial.println(F("Finished registering")); Serial.flush();
    #endif

    #ifdef ENABLE_CHOCOLATEFEET_USB
        Serial.println(F("about to register DeviceBehaviour_Chocolate...")); Serial.flush();
        behaviour_chocolate = new DeviceBehaviour_Chocolate();
        behaviour_manager->registerBehaviour(behaviour_chocolate);
        Serial.println(F("Finished registering")); Serial.flush();
    #endif

    #ifdef ENABLE_BITBOX
        Serial.println(F("about to register behaviour_bitbox...")); Serial.flush();
        behaviour_manager->registerBehaviour(behaviour_bitbox);
        Serial.println(F("connecting device output..")); Serial.flush();
        behaviour_bitbox->connect_device_output(&ENABLE_BITBOX);
        Serial.println(F("Finished registering")); Serial.flush();
    #endif

    #ifdef ENABLE_BASS
        Serial.println(F("about to register behaviour_neutron...")); Serial.flush();
        behaviour_manager->registerBehaviour(behaviour_neutron);
        behaviour_neutron->connect_device_output(&ENABLE_BASS);
        Serial.println(F("Finished registering")); Serial.flush();
    #endif

    #ifdef ENABLE_LESTRUM
        Serial.println(F("about to register behaviour_lestrum...")); Serial.flush();
        behaviour_manager->registerBehaviour(behaviour_lestrum);
        Serial.println(F("Finished registering - connecting!")); Serial.flush();
        behaviour_lestrum->connect_device_input(&ENABLE_LESTRUM);
        Serial.println(F("Finished connect_device_input")); Serial.flush();
    #endif
    
    #ifdef ENABLE_DRUMKIT
        Serial.println(F("about to register behaviour_drumkit...")); Serial.flush();
        behaviour_manager->registerBehaviour(behaviour_drumkit);
        behaviour_drumkit->connect_device_input(&ENABLE_DRUMKIT);
        Serial.println(F("Finished registering")); Serial.flush();
    #endif

    #ifdef ENABLE_CV_INPUT_PITCH
        Serial.println(F("about to register behaviour_cvinput...")); Serial.flush();
        behaviour_manager->registerBehaviour(behaviour_cvinput);
        Serial.println(F("Finished registering")); Serial.flush();
    #endif
    
    Serial.println(F("Exiting setup_behaviour_manager()"));

    /*usb_manager->registerBehaviour(new USBDevice_Bamble());
    usb_manager->registerBehaviour(new USBDevice_Beatstep());
    usb_manager->registerBehaviour(new USBDevice_Keystep());
    usb_manager->registerBehaviour(new USBDevice_MPK49());
    usb_manager->registerBehaviour(new USBDevice_Subclocker());*/
}


#ifdef ENABLE_SCREEN
    void DeviceBehaviourManager::create_behaviour_menu_items(Menu *menu) {
        for (int i = 0 ; i < behaviours->size() ; i++) {
            Serial.printf(F("\tDeviceBehaviourManager::make_menu_items %i: calling make_menu_items on behaviour '%s'\n"), i, behaviours->get(i)->get_label()); Serial.flush(); 
            LinkedList<MenuItem *> *menuitems = behaviours->get(i)->make_menu_items();
            if (menuitems->size()>0) {
                String s = String((char*)(behaviours->get(i)->get_label())) + String(" >>>");
                menu->add(new SeparatorMenuItem((char*)s.c_str(), BLUE));

                Serial.printf(F("\t\tGot %i items, adding them to menu...\n"), menuitems->size()); Serial.flush();
                for (int n = 0 ; n < menuitems->size() ; n++) {
                    Serial.printf(F("\t\tadding menuitem '%s'\n"), menuitems->get(n)->label); Serial.flush();
                    menu->add(menuitems->get(n));
                }
            }
        }
    }
#endif