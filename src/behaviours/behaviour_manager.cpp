#include "Config.h"

#include "behaviours/behaviour_manager.h"

#include "behaviours/behaviour_apcmini.h"
#include "behaviours/behaviour_bamble.h"
#include "behaviours/behaviour_beatstep.h"
#include "behaviours/behaviour_keystep.h"
#include "behaviours/behaviour_mpk49.h"
#include "behaviours/behaviour_subclocker.h"
#include "behaviours/behaviour_skulptsynth.h"
#include "behaviours/behaviour_craftsynth.h"
#include "behaviours/behaviour_chocolate.h"

#include "behaviours/behaviour_microlidian.h"
#include "behaviours/behaviour_xiaoserial.h"

#include "behaviours/behaviour_midilights.h"

#include "behaviours/behaviour_bitbox.h"
#include "behaviours/behaviour_neutron.h"
#include "behaviours/behaviour_lestrum.h"
#include "behaviours/behaviour_drumkit.h"

#include "behaviours/behaviour_cvinput.h"
#include "behaviours/behaviour_cvoutput.h"

#include "behaviours/behaviour_dptlooper.h"
#include "behaviours/behaviour_midimuso.h"
#include "behaviours/behaviour_midimuso_4pv.h"
#include "behaviours/behaviour_midimuso_4mv.h"

#include "behaviours/behaviour_bedge.h"

#include "behaviours/behaviour_opentheremin.h"

#include "behaviours/behaviour_midibassproxy.h"

#include "behaviours/behaviour_euclidianrhythms.h"

DeviceBehaviourManager *behaviour_manager = nullptr;

DeviceBehaviourManager* DeviceBehaviourManager::inst_ = nullptr;
DeviceBehaviourManager* DeviceBehaviourManager::getInstance() {
    if (inst_ == nullptr) {
        inst_ = new DeviceBehaviourManager();
    }
    return inst_;
}

// convenience function
void behaviour_manager_kill_all_current_notes () {
    behaviour_manager->kill_all_current_notes();
}

//FLASHMEM
void setup_behaviour_manager() {
    behaviour_manager = DeviceBehaviourManager::getInstance();

    #ifdef ENABLE_CLOCKS
        behaviour_clock_gates = new VirtualBehaviour_ClockGates(gate_manager, BANK_CLOCK);
        behaviour_manager->registerBehaviour(behaviour_clock_gates);
    #endif
    #ifdef ENABLE_SEQUENCER
        behaviour_sequencer_gates = new VirtualBehaviour_SequencerGates(gate_manager, BANK_SEQ);
        behaviour_manager->registerBehaviour(behaviour_sequencer_gates);
    #endif

    #ifdef ENABLE_EUCLIDIAN
        behaviour_euclidianrhythms = new VirtualBehaviour_EuclidianRhythms();
        behaviour_manager->registerBehaviour(behaviour_euclidianrhythms);
    #endif

    #ifdef ENABLE_APCMINI
        behaviour_apcmini = new DeviceBehaviour_APCMini();
        #ifdef ENABLE_LOOPER
            behaviour_apcmini->loop_track = &midi_loop_track;
        #endif
        behaviour_manager->registerBehaviour(behaviour_apcmini);
    #endif

    #ifdef ENABLE_BAMBLE
        behaviour_manager->registerBehaviour(behaviour_bamble);
    #endif

    #ifdef ENABLE_BEATSTEP
        behaviour_manager->registerBehaviour(behaviour_beatstep);
        #ifdef ENABLE_BEATSTEP_2
            behaviour_manager->registerBehaviour(behaviour_beatstep_2);
        #endif
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

    #ifdef ENABLE_MICROLIDIAN
        behaviour_microlidian = new DeviceBehaviour_Microlidian();
        behaviour_manager->registerBehaviour(behaviour_microlidian);
        #if defined(ENABLE_MICROLIDIAN) && defined(ENABLE_XIAOSERIAL) && defined(ENABLE_USBSERIAL)
            behaviour_manager->registerBehaviour(behaviour_xiaoserial);
        #endif
    #endif

    #ifdef ENABLE_MIDILIGHTS
        #ifdef ENABLE_MIDILIGHTS_DEDICATED
            behaviour_midilights = new DeviceBehaviour_MIDILights();
            behaviour_manager->registerBehaviour(behaviour_midilights);
        #else
            //behaviour_manager->registerBehaviour(new Behaviour_USBSimpleClockedWrapper<>("MIDILights", 0x1337, 0x117e));
            behaviour_manager->registerBehaviour(new Behaviour_SimpleWrapperUSB<DividedClockedBehaviour>("MIDILights", 0x1337, 0x117e));
        #endif
    #endif

    #ifdef ENABLE_SUBCLOCKER_DEDICATED
        behaviour_subclocker = new DeviceBehaviour_Subclocker();
        behaviour_manager->registerBehaviour(behaviour_subclocker);
    #elif ENABLE_SUBCLOCKER
        behaviour_manager->registerBehaviour(new Behaviour_USBSimpleDividedClockedWrapper<>("Subclocker", 0x1337, 0x1337));
    #endif

    #ifdef ENABLE_CHOCOLATEFEET_USB
        Serial.println(F("about to register DeviceBehaviour_Chocolate...")); Serial_flush();
        behaviour_chocolate = new DeviceBehaviour_Chocolate();
        behaviour_manager->registerBehaviour(behaviour_chocolate);
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

    #if defined(ENABLE_CV_INPUT) && defined(ENABLE_CV_INPUT_PITCH)
        Serial.println(F("about to register behaviour_cvinput...")); Serial_flush();
        behaviour_manager->registerBehaviour(behaviour_cvinput_1);
        behaviour_manager->registerBehaviour(behaviour_cvinput_2);
        behaviour_manager->registerBehaviour(behaviour_cvinput_3);
        Serial.println(F("Finished registering")); Serial_flush();
    #endif

    #if defined(ENABLE_CV_OUTPUT)
        Serial.println(F("about to register behaviour_cvoutput...")); Serial_flush();
        behaviour_manager->registerBehaviour(behaviour_cvoutput_1);
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

    behaviour_midibassproxy = new MIDIBassBehaviourProxy();
    behaviour_manager->registerBehaviour(behaviour_midibassproxy);

    #ifdef ENABLE_CRAFTSYNTH_USB
        Serial.println(F("about to register DeviceBehaviour_CraftSynth...")); Serial_flush();
        behaviour_manager->registerBehaviour(behaviour_craftsynth);
        Serial.println(F("Finished registering")); Serial_flush();
    #endif

    #ifdef ENABLE_SKULPTSYNTH_USB
        Serial.println(F("about to register DeviceBehaviour_SkulptSynth...")); Serial_flush();
        behaviour_manager->registerBehaviour(behaviour_skulptsynth);
        Serial.println(F("Finished registering")); Serial_flush();
    #endif
    
    #ifdef ENABLE_BITBOX
        Serial.println(F("about to register behaviour_bitbox...")); Serial_flush();
        //behaviour_manager->registerBehaviour(behaviour_bitbox);
        #ifdef ENABLE_BITBOX_DEDICATED
            //
        #else
            behaviour_bitbox = new Behaviour_SimpleWrapper<DeviceBehaviourSerialBase,DividedClockedBehaviour>("BitBox", false, true);
        #endif
        behaviour_manager->registerBehaviour(behaviour_bitbox);
        Serial.println(F("Bitbox: connecting device output..")); Serial_flush();
        behaviour_bitbox->connect_device_output(&ENABLE_BITBOX);
        Serial.println(F("Bitbox: Finished registering.")); Serial_flush();
    #endif

    #ifdef ENABLE_NEUTRON
        Serial.println(F("about to register behaviour_neutron...")); Serial_flush();
        behaviour_manager->registerBehaviour(behaviour_neutron);
        behaviour_neutron->connect_device_output(&ENABLE_NEUTRON);
        Serial.println(F("Finished registering")); Serial_flush();
    #endif

    #ifdef ENABLE_MIDIMUSO
        behaviour_manager->registerBehaviour(behaviour_midimuso);
        behaviour_midimuso->connect_device_output(&ENABLE_MIDIMUSO);
    #endif
    #ifdef ENABLE_MIDIMUSO_4PV
        behaviour_midimuso_4pv = new Behaviour_SimpleWrapper<DividedClockedBehaviour,DeviceBehaviourSerialBase>("MIDIMUSO CV-12 4PV", false, true);
        behaviour_midimuso_4pv->TUNING_OFFSET = -3; // because MIDI MUSO CV12's tuning is based on 1V=A, not 1V=C
        behaviour_midimuso_4pv->connect_device_output(&ENABLE_MIDIMUSO_4PV);
        behaviour_manager->registerBehaviour(behaviour_midimuso_4pv);        
    #endif
    #ifdef ENABLE_MIDIMUSO_4MV
        behaviour_midimuso_4mv->connect_device_output(&ENABLE_MIDIMUSO_4MV);
        behaviour_manager->registerBehaviour(behaviour_midimuso_4mv);
    #endif

    #ifdef ENABLE_BEHRINGER_EDGE_USB
        behaviour_manager->registerBehaviour(new Behaviour_SimpleWrapperUSB<DividedClockedBehaviour>("BEdge", 0x1397, 0x125A));
    #elif defined(ENABLE_BEHRINGER_EDGE_SERIAL)
        Behaviour_SimpleWrapper<DividedClockedBehaviour,DeviceBehaviourSerialBase> *bedge = new Behaviour_SimpleWrapper<DividedClockedBehaviour,DeviceBehaviourSerialBase>("Bedge", false, false);
        bedge->connect_device_output(&ENABLE_BEHRINGER_EDGE_SERIAL);
        behaviour_manager->registerBehaviour(bedge);
    #elif defined(ENABLE_BEHRINGER_EDGE_SERIAL_DEDICATED)
        DeviceBehaviour_Bedge_Serial *bedge = new DeviceBehaviour_Bedge_Serial();
        bedge->connect_device_output(&ENABLE_BEHRINGER_EDGE_SERIAL_DEDICATED);
        behaviour_manager->registerBehaviour(bedge);
    #elif defined(ENABLE_BEHRINGER_EDGE_USB_DEDICATED)
        behaviour_bedge = new DeviceBehaviour_Bedge();
        behaviour_manager->registerBehaviour(behaviour_bedge);            
    #endif
    
    Serial.println(F("Exiting setup_behaviour_manager()"));
}


#ifdef ENABLE_SCREEN
    #include "menuitems.h"
    #include "menuitems_lambda.h"
    #include "menuitems_quickpage.h"

    //FLASHMEM  causes a section type conflict with virtual void DeviceBehaviourUltimateBase::setup_callbacks()
    void DeviceBehaviourManager::create_all_behaviour_menu_items(Menu *menu) {
        Serial_println("Starting create_all_behaviour_menu_items"); Serial_flush();

        menu->add_page("QuickJumpBehaviours");
        CustomQuickPagesMenuItem *quickjump = new CustomQuickPagesMenuItem("QuickJump to Behaviours");
        menu->add(quickjump);
        page_t *started_page = menu->get_selected_page();   // for remembering what page the quickjump menu itself is
        
        // add the behaviours quickjump page to the 'main' menu quickjump list
        menu->remember_opened_page(menu->get_page_index_for_name(menu->get_selected_page()->title));

        for (unsigned int i = 0 ; i < behaviours->size() ; i++) {
            DeviceBehaviourUltimateBase *behaviour = behaviours->get(i);
            Serial_printf("about to create_single_behaviour_menu_items() for behaviour %i/%i...", i+1, behaviours->size());
            Serial_flush();
            if (behaviour==nullptr) {
                Serial.println("\tgot a nullptr behaviour!");
                continue;
            } else {
                Serial.printf(" ('%s')\n", behaviour->get_label());
            }
            this->create_single_behaviour_menu_items(menu, behaviour);

            // add page to behaviour quickjump, so long as isn't itself
            if (started_page < menu->get_selected_page()) {
                // todo: figure out a better way of adding the correct page to the quickjump
                quickjump->add_page(menu->get_selected_page());
                //quickjump->add_page(started_page+1);
                //started_page = menu->get_selected_page();
            }
            Serial_println("...created.");
            //started_page = menu->get_selected_page();
        }

        // create a page for holding recall/save options from every behaviour
        menu->add_page("Recall parameters");
        for (unsigned int i = 0 ; i < behaviours->size() ; i++) {
            Serial_printf("about to set up Recall parameters () for behaviour %i/%i\n", i+1, behaviours->size());
            Serial_flush();
            DeviceBehaviourUltimateBase *behaviour = behaviours->get(i);
            if (behaviour==nullptr) {
                //Serial.println("\tgot a nullptr behaviour!");
                continue;
            } else {
                //Serial.printf("\tdoing for %s\n", behaviour->get_label());
            }
            LinkedList<SaveableParameterBase*> *saveables = behaviour->saveable_parameters;
            if(saveables==nullptr || saveables->size()==0) 
                continue;
            menu->add(
                new SeparatorMenuItem(behaviour->get_label()),
                behaviour->colour
            );
            const char *last_category = nullptr;
            for (unsigned int i = 0 ; i < saveables->size() ; i++) {
                SaveableParameterBase *p = saveables->get(i);
                if (last_category!=p->category_name) {
                    menu->add(new SeparatorMenuItem(p->category_name), behaviour->colour);
                }
                menu->add(
                    new LambdaToggleControl(p->niceify(), 
                        [p](bool v) -> void { p->set_recall_enabled(v); },
                        [p]() -> bool { return p->is_recall_enabled(); }
                    ),
                    behaviour->colour
                );
                last_category = p->category_name;
            }
        }

        Serial_println("Finished in create_all_behaviour_menu_items"); Serial_flush();
    }

    //FLASHMEM
    // create the menuitems relating to a behaviour's parameters and saveableparameters
    //FLASHMEM
    void DeviceBehaviourManager::create_single_behaviour_menu_items(Menu *menu, DeviceBehaviourUltimateBase *behaviour) {
            Serial_printf(F("\tDeviceBehaviourManager::make_menu_items: calling make_menu_items on behaviour '%s'...\n"), behaviour->get_label()); Serial_flush(); 
            //debug_free_ram();
            LinkedList<MenuItem *> *menuitems = behaviour->make_menu_items();
            Serial_printf(F("\tDeviceBehaviourManager::make_menu_items: done calling make_menu_items on behaviour '%s'\n"), behaviour->get_label()); Serial_flush(); 

            uint16_t group_colour = C_WHITE;
            if (menuitems->size()>0 || behaviour->has_parameters() || behaviour->has_saveable_parameters()) {
                group_colour = behaviour->colour = menu->get_next_colour();

                menu->add_page(behaviour->get_label(), group_colour);

                // add a separator bar
                SeparatorMenuItem *separator = new SeparatorMenuItem(behaviour->get_label());
                menu->add(separator, group_colour);
            }

            if (menuitems->size()>0) {
                Serial_printf(F("\t\tGot %i items, adding them to menu...\n"), menuitems->size()); Serial_flush();
                menu->add(menuitems, group_colour);
                Serial_println("added.");
            }

            // todo: move this into behaviour's make_menu_items? not doing this currently because would need to add it manually to every subclass's make_menu_items...
            if (behaviour->has_parameters()) {
                Serial_print("doing addParameterSubMenuItems.."); Serial_flush(); 
                parameter_manager->addParameterSubMenuItems(
                    menu, 
                    behaviour->get_label(), 
                    behaviour->get_parameters(),
                    group_colour
                );
                Serial_println("done."); Serial_flush(); 
            }

            // todo: move this into behaviour's make_menu_items? not doing this currently because would need to add it manually to every subclass's make_menu_items...
            if (behaviour->has_saveable_parameters()) {
                Serial_print("doing create_saveable_parameters_recall_selector.."); Serial_flush(); 
                menu->add(
                    behaviour->create_saveable_parameters_recall_selector(), 
                    group_colour
                );
                Serial_println("done."); Serial_flush(); 
            }

    }
#endif