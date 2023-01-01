#include "Config.h"
#include "menu.h"
#include "debug.h"
#include "mymenu/menu_usb.h"
#include "mymenu/menu_behaviours.h"
#include "menuitems_numbers.h"
#include "submenuitem_bar.h"

extern bool debug, debug_stress_sequencer_load;

class DebugPanel : public MenuItem {
    public:
        DebugPanel() : MenuItem("Debug") {
            this->selectable = false;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft->setCursor(pos.x,pos.y);
            header("Statistics:", pos, selected, opened);
            tft->printf("Built at " __TIME__ " on " __DATE__ "\n");
            tft->printf("Free RAM: %u bytes\n", freeRam());
            tft->printf("Uptime: %02uh %02um %02us\n", millis()/1000/60/60, (millis()/1000/60)%60, (millis()/1000)%60);
            return tft->getCursorY();
        }
};


#ifndef GDB_DEBUG
FLASHMEM // void setup_debug_menu() causes a section type conflict with void Menu::start()
#endif
void setup_debug_menu() {
    menu->add_page("Behaviours/USB");

    #ifdef ENABLE_USB
        USBDevicesPanel *usbdevices_panel = new USBDevicesPanel();
        menu->add(usbdevices_panel);
    #endif

    BehavioursPanel *behaviours_panel = new BehavioursPanel();
    menu->add(behaviours_panel);

    ////

    menu->add_page("Debug");

    ActionConfirmItem *reset_control = new ActionConfirmItem("RESET TEENSY?", reset_teensy);
    menu->add(reset_control);

    SubMenuItemBar *bar = new SubMenuItemBar("Debug");

    ObjectToggleControl<Menu> *debug_times_control = new ObjectToggleControl<Menu>("Render times", menu, &Menu::setDebugTimes, &Menu::isDebugTimes, nullptr);
    bar->add(debug_times_control);
    bar->add(new NumberControl<bool>("Extra", (bool*)&debug, debug, false, true));
    bar->add(new NumberControl<bool>("InSaNe", (bool*)&debug_stress_sequencer_load, debug, false, true));
    menu->add(bar);

    menu->add(new DebugPanel());
}