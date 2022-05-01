#include "Config.h"
#include "storage.h"

#ifdef ENABLE_SCREEN
#include "mymenu.h"

#include "menu_looper.h"
#include "menu_sequencer.h"

Menu menu = Menu();

PositionIndicator posbar = PositionIndicator();
//LooperStatus mpk49_looper = LooperStatus();
HarmonyStatus beatstep_notes = HarmonyStatus();
SequencerStatus sequencer_status = SequencerStatus();
LooperStatus    mpk49_looper_status = LooperStatus();
USBDevicesPanel usbdevices_panel = USBDevicesPanel();

//MenuItem test_item_1 = MenuItem("test 1");
//MenuItem test_item_2 = MenuItem("test 2");
//MenuItem test_item_3 = MenuItem("test 3");

void setup_menu() {
    menu.add(&posbar);
    //menu.add(&mpk49_looper);
    menu.add(&beatstep_notes);
    menu.add(&sequencer_status);
    menu.add(&mpk49_looper_status);
    menu.add(&usbdevices_panel);

    //todo: move this to menu
    pinMode(PIN_BUTTON_A, INPUT_PULLUP);
    pinMode(PIN_BUTTON_B, INPUT_PULLUP);
    pinMode(PIN_BUTTON_C, INPUT_PULLUP);

    //menu.add(&test_item_1);
    //menu.add(&test_item_2);
    //menu.add(&test_item_3);
}

#endif