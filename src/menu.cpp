#include "mymenu.h"

Menu menu = Menu();

PositionIndicator posbar = PositionIndicator();
LooperStatus mpk49_looper = LooperStatus();
HarmonyStatus beatstep_notes = HarmonyStatus();

MenuItem test_item_1 = MenuItem("test 1");
MenuItem test_item_2 = MenuItem("test 2");
MenuItem test_item_3 = MenuItem("test 3");

void setup_menu() {
    menu.add(&posbar);
    menu.add(&mpk49_looper);

    menu.add(&test_item_1);
    menu.add(&test_item_2);
    menu.add(&test_item_3);
}
