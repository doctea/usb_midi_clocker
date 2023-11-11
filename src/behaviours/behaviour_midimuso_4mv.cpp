#include "Config.h"

#include "behaviours/behaviour_midimuso_4mv.h"

#ifdef ENABLE_MIDIMUSO_4MV
    Behaviour_MIDIMuso_4MV *behaviour_midimuso_4mv = new Behaviour_MIDIMuso_4MV();

    #ifdef ENABLE_SCREEN
        #include "menuitems.h"

        FLASHMEM
        LinkedList<MenuItem *> *Behaviour_MIDIMuso_4MV::make_menu_items() {
            LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();

            // todo: remove the sort-of useless default HarmonyStatus from the menuitems...?

            for (int i = 0 ; i < max_voice_count ; i++) {
                char label[MENU_C_MAX];
                snprintf(label, MENU_C_MAX, "Output %i", i+1);
                HarmonyStatus *status = new HarmonyStatus(label, &last_voices[i], &voices[i]);
                menuitems->add(status);
            }

            return menuitems;
        }
    #endif
#endif