#include "Config.h"

#if defined(ENABLE_SCREEN) && defined(ENABLE_STORAGE)

#include "menu.h"
#include "submenuitem_bar.h"
#include "menuitems_lambda.h"
#include "menuitems_pageviewer.h"
#include "mymenu/menu_fileviewers.h"
#include "storage.h"

void setup_system_settings_menu() {
    menu->add_page("System Settings");

    SubMenuItemBar *system_settings_bar = new SubMenuItemBar("System Settings", false, false);
    system_settings_bar->add(new LambdaActionConfirmItem("Save", [=]() -> void {
        storage::save_system_settings();
    }));
    system_settings_bar->add(new LambdaActionConfirmItem("Load", [=]() -> void {
        storage::load_system_settings();
    }));
    menu->add(system_settings_bar);

    #ifdef ENABLE_SD
        system_settings_fileviewer = new PageFileViewerMenuItem("System");
        menu->add(system_settings_fileviewer);
        update_system_settings_filename(String(storage::get_system_settings_filename()));
    #endif
}

#endif