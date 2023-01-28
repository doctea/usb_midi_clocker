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
#include "behaviours/behaviour_dptlooper.h"

#include "behaviours/behaviour_opentheremin.h"

DeviceBehaviourManager *behaviour_manager = nullptr;

DeviceBehaviourManager* DeviceBehaviourManager::inst_ = nullptr;
DeviceBehaviourManager* DeviceBehaviourManager::getInstance() {
    if (inst_ == nullptr) {
        inst_ = new DeviceBehaviourManager();
    }
    return inst_;
}

//FLASHMEM
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
        behaviour_manager->registerBehaviour(behaviour_bamble);
    #endif

    #ifdef ENABLE_BEATSTEP
        behaviour_manager->registerBehaviour(behaviour_beatstep);
        //behaviour_beatstep->debug = true;
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
        Serial.println(F("about to register DeviceBehaviour_CraftSynth...")); Serial_flush();
        behaviour_manager->registerBehaviour(behaviour_craftsynth);
        Serial.println(F("Finished registering")); Serial_flush();
    #endif

    #ifdef ENABLE_CHOCOLATEFEET_USB
        Serial.println(F("about to register DeviceBehaviour_Chocolate...")); Serial_flush();
        behaviour_chocolate = new DeviceBehaviour_Chocolate();
        behaviour_manager->registerBehaviour(behaviour_chocolate);
        Serial.println(F("Finished registering")); Serial_flush();
    #endif

    #ifdef ENABLE_BITBOX
        Serial.println(F("about to register behaviour_bitbox...")); Serial_flush();
        behaviour_manager->registerBehaviour(behaviour_bitbox);
        Serial.println(F("connecting device output..")); Serial_flush();
        behaviour_bitbox->connect_device_output(&ENABLE_BITBOX);
        Serial.println(F("Finished registering")); Serial_flush();
    #endif

    #ifdef ENABLE_BASS
        Serial.println(F("about to register behaviour_neutron...")); Serial_flush();
        behaviour_manager->registerBehaviour(behaviour_neutron);
        behaviour_neutron->connect_device_output(&ENABLE_BASS);
        Serial.println(F("Finished registering")); Serial_flush();
    #endif

    #ifdef ENABLE_LESTRUM
        Serial.println(F("about to register behaviour_lestrum...")); Serial_flush();
        behaviour_manager->registerBehaviour(behaviour_lestrum);
        Serial.println(F("Finished registering - connecting!")); Serial_flush();
        behaviour_lestrum->connect_device_input(&ENABLE_LESTRUM);
        Serial.println(F("Finished connect_device_input")); Serial_flush();
    #endif
    
    #ifdef ENABLE_DRUMKIT
        Serial.println(F("about to register behaviour_drumkit...")); Serial_flush();
        behaviour_manager->registerBehaviour(behaviour_drumkit);
        behaviour_drumkit->connect_device_input(&ENABLE_DRUMKIT);
        Serial.println(F("Finished registering")); Serial_flush();
    #endif

    #ifdef ENABLE_CV_INPUT_PITCH
        Serial.println(F("about to register behaviour_cvinput...")); Serial_flush();
        behaviour_manager->registerBehaviour(behaviour_cvinput);
        Serial.println(F("Finished registering")); Serial_flush();
    #endif

    #ifdef ENABLE_OPENTHEREMIN
        behaviour_opentheremin = new DeviceBehaviour_OpenTheremin();
        behaviour_manager->registerBehaviour(behaviour_opentheremin);
    #endif

    #ifdef ENABLE_DPT_LOOPER
        behaviour_manager->registerBehaviour(behaviour_dptlooper);
        behaviour_dptlooper->connect_device_output(&ENABLE_DPT_LOOPER);
    #endif
    
    Serial.println(F("Exiting setup_behaviour_manager()"));
}


#ifdef ENABLE_SCREEN
    #include "menuitems.h"
    //FLASHMEM  causes a section type conflict with virtual void DeviceBehaviourUltimateBase::setup_callbacks()
    void DeviceBehaviourManager::create_all_behaviour_menu_items(Menu *menu) {
        for (unsigned int i = 0 ; i < behaviours->size() ; i++) {
            this->create_single_behaviour_menu_items(menu, behaviours->get(i));
        }
    }

    //FLASHMEM 
    inline void DeviceBehaviourManager::create_single_behaviour_menu_items(Menu *menu, DeviceBehaviourUltimateBase *behaviour) {
            Debug_printf(F("\tDeviceBehaviourManager::make_menu_items: calling make_menu_items on behaviour '%s'\n"), behaviour->get_label()); Serial_flush(); 
            LinkedList<MenuItem *> *menuitems = behaviour->make_menu_items();

            uint16_t group_colour = C_WHITE;
            if (menuitems->size()>0 || behaviour->has_parameters()) {
                group_colour = behaviour->colour = menu->get_next_colour();

                menu->add_page(behaviour->get_label(), group_colour);

                // add a separator bar
                //String s = String((char*)(behaviour->get_label())) + String(" >>>");
                //SeparatorMenuItem *separator = new SeparatorMenuItem((char*)s.c_str());
                SeparatorMenuItem *separator = new SeparatorMenuItem((char*)behaviour->get_label());
                separator->set_default_colours(group_colour, BLACK);
                menu->add(separator);
            }

            if (menuitems->size()>0) {
                Debug_printf(F("\t\tGot %i items, adding them to menu...\n"), menuitems->size()); Serial_flush();
                menu->add(menuitems, group_colour);
            }

            if (behaviour->has_parameters()) {
                parameter_manager->addParameterSubMenuItems(
                    menu, 
                    behaviour->get_label(), 
                    behaviour->get_parameters(),
                    group_colour
                );
            }

            if (behaviour->saveable_parameters!=nullptr && behaviour->saveable_parameters->size()>0) {
                menu->add(behaviour->create_saveable_parameters_recall_selector());
            }

    }
#endif