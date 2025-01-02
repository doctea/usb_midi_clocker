#include "Config.h"

#include "behaviours/behaviour_midimuso_4mv.h"

#ifdef ENABLE_MIDIMUSO_4MV
    Behaviour_MIDIMuso_4MV *behaviour_midimuso_4mv = new Behaviour_MIDIMuso_4MV();

    #ifdef ENABLE_SCREEN
        #include "menuitems.h"
        #include "submenuitem_bar.h"
        #include "menuitems_lambda.h"
        #include "mymenu/menuitems_harmony.h"

        //FLASHMEM
        LinkedList<MenuItem *> *Behaviour_MIDIMuso_4MV::make_menu_items() {
            LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();

            //return menuitems;

            // todo: remove the sort-of useless default HarmonyStatus from the menuitems...?

            /*SubMenuItemBar *allow_voice_toggles = new SubMenuItemBar("Allowed by Auto");

            for (int i = 0 ; i < max_voice_count ; i++) {
                char label[MENU_C_MAX];
                snprintf(label, MENU_C_MAX, "Output %i", i+1);

                Serial_printf("%s#make_menu_items: Creating controls for %s\n", this->get_label(), label); Serial_flush();

                HarmonyStatus *status = new HarmonyStatus(label, &last_voices[i], &voices[i]);
                menuitems->add(status);

                allow_voice_toggles->add(new LambdaToggleControl(label, 
                    [=](bool v) -> void { this->allow_voice_for_auto[i] = v;    },
                    [=]()       -> bool { return this->allow_voice_for_auto[i]; }
                ));
            }

            menuitems->add(allow_voice_toggles);*/
            PolyphonicBehaviour::make_menu_items();

            DividedClockedBehaviour::make_menu_items();
            MIDIBassBehaviour::make_menu_items();

            return menuitems;
        }
    #endif
#endif