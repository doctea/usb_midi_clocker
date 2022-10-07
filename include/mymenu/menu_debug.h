#include "Config.h"
#include "menu.h"
#include "debug.h"
#include "mymenu/menu_usb.h"
#include "mymenu/menu_behaviours.h"

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
}