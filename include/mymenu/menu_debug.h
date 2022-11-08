#include "Config.h"
#include "menu.h"
#include "debug.h"
#include "mymenu/menu_usb.h"
#include "mymenu/menu_behaviours.h"
#include "menuitems_numbers.h"

extern bool debug, debug_insane_sequencer_load;

#ifndef GDB_DEBUG
FLASHMEM // void setup_debug_menu() causes a section type conflict with void Menu::start()
#endif
void setup_debug_menu() {
    #ifdef ENABLE_USB
        USBDevicesPanel *usbdevices_panel = new USBDevicesPanel();
    #endif

    BehavioursPanel *behaviours_panel = new BehavioursPanel();

    menu->add(usbdevices_panel);
    menu->add(behaviours_panel);

    ActionConfirmItem *reset_control = new ActionConfirmItem("RESET TEENSY?", reset_teensy);
    menu->add(reset_control);

    ObjectToggleControl<Menu> *debug_times_control = new ObjectToggleControl<Menu>("Debug: Menu item times", menu, &Menu::setDebugTimes, &Menu::isDebugTimes, nullptr);
    menu->add(debug_times_control);

    menu->add(new NumberControl<bool>("Debug to serial", (bool*)&debug, debug, false, true));
    menu->add(new NumberControl<bool>("Debug insane autoload", (bool*)&debug_insane_sequencer_load, debug, false, true));

}