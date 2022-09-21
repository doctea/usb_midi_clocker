#include "menuitems.h"

#include "behaviour_base.h"
#include "behaviour_manager.h"

class BehavioursPanel : public MenuItem {
    public:
        BehavioursPanel() : MenuItem("Behaviours") {}

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft->setCursor(pos.x,pos.y);
            header("Behaviours:", pos, selected, opened);
            colours(opened);
            tft->setTextSize(1);
            //int connected = 0;
            char buf[100];
            for (int i = 0 ; i < behaviour_manager->behaviours_usb.size() ; i++) {
                DeviceBehaviourUSBBase *usb_behaviour = behaviour_manager->behaviours_usb.get(i);
                if (usb_behaviour->is_connected())
                    sprintf(buf, "%i %19s [usb]\n", i, usb_behaviour->device->product());
                else
                    sprintf(buf, "%i %10s [usb, disconnected]\n", i, usb_behaviour->get_label());
                tft->printf(buf);
            }
            for (int i = 0 ; i < behaviour_manager->behaviours_serial.size() ; i++) {
                sprintf(buf, "%i %15s [serial]\n", i, behaviour->manager->behaviours_serial.get(i)->get_label());
                tft->printf(buf);
            }

            /*for (int i = 0 ; i < (NUM_USB_DEVICES - connected) ; i++) { // blank unused rows
                tft->printf("%21s\n","");
            }*/
            return tft->getCursorY();
        }
};