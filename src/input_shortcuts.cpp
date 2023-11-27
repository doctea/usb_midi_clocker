#ifdef ENABLE_SCREEN
    #include "menu.h"
    #include "menuitems_object_multitoggle.h"

    extern ObjectMultiToggleControl *project_multi_recall_options;
    extern ObjectMultiToggleControl *project_multi_autoadvance;

    void toggle_recall(bool on = false) {
        project_multi_recall_options->switch_all(on);
    }

    void toggle_autoadvance(bool on = false) {
        project_multi_autoadvance->switch_all(on);
    }

#endif
