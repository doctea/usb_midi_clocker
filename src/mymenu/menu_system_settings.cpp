#include "Config.h"

#if defined(ENABLE_SCREEN) && defined(ENABLE_STORAGE)

#include "menu.h"
#include "submenuitem_bar.h"
#include "menuitems_lambda.h"
#include "menuitems_object_multitoggle.h"
#include "menuitems_pageviewer.h"
#include "mymenu/menu_fileviewers.h"
#include "system_settings.h"
#include "storage.h"

void setup_system_settings_menu() {
    menu->add_page("System", C_WHITE, true, "Settings");

    SubMenuItemBar *system_settings_bar = new SubMenuItemBar("System Settings", false, false);
    system_settings_bar->add(new LambdaActionConfirmItem("Save", [=]() -> void {
        storage::save_system_settings();
    }));
    system_settings_bar->add(new LambdaActionConfirmItem("Load", [=]() -> void {
        storage::load_system_settings();
    }));
    menu->add(system_settings_bar);

    ObjectMultiToggleControl *transport_release = new ObjectMultiToggleControl("Release outputs", true);
    transport_release->addItem(new MultiToggleItemClass<SystemSettings>(
        "MIDI Stop",
        &system_settings,
        &SystemSettings::setReleaseOutputsOnMidiStop,
        &SystemSettings::isReleaseOutputsOnMidiStop
    ));
    transport_release->addItem(new MultiToggleItemClass<SystemSettings>(
        "Clock loss",
        &system_settings,
        &SystemSettings::setReleaseOutputsOnExternalClockLoss,
        &SystemSettings::isReleaseOutputsOnExternalClockLoss
    ));
    menu->add(transport_release);

    #ifdef ENABLE_SD
        system_settings_fileviewer = new PageFileViewerMenuItem("System");
        menu->add(system_settings_fileviewer);
        update_system_settings_filename(String(storage::get_system_settings_filename()));
    #endif
}

#endif