#include "Config.h"
#include "menu.h"
#include "debug.h"
#include "mymenu/menu_usb.h"
#include "mymenu/menu_behaviours.h"
#include "menuitems_numbers.h"
#include "submenuitem_bar.h"

extern bool debug, debug_stress_sequencer_load;

#ifndef GDB_DEBUG
FLASHMEM // void setup_debug_menu() causes a section type conflict with void Menu::start()
#endif
void setup_debug_menu() {
    #ifdef ENABLE_USB
        USBDevicesPanel *usbdevices_panel = new USBDevicesPanel();
    #endif

    BehavioursPanel *behaviours_panel = new BehavioursPanel();

    menu->add_page("Debug");

    menu->add(usbdevices_panel);
    menu->add(behaviours_panel);

    ActionConfirmItem *reset_control = new ActionConfirmItem("RESET TEENSY?", reset_teensy);
    menu->add(reset_control);

    SubMenuItemBar *bar = new SubMenuItemBar("Debug");

    ObjectToggleControl<Menu> *debug_times_control = new ObjectToggleControl<Menu>("Render times", menu, &Menu::setDebugTimes, &Menu::isDebugTimes, nullptr);
    bar->add(debug_times_control);
    bar->add(new NumberControl<bool>("Extra", (bool*)&debug, debug, false, true));
    bar->add(new NumberControl<bool>("InSaNe", (bool*)&debug_stress_sequencer_load, debug, false, true));
    menu->add(bar);
}