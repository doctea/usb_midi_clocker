#include "menuitems.h"

#include "behaviours/behaviour_base.h"
#include "behaviours/behaviour_manager.h"

class BehavioursPanel : public MenuItem {
    public:
        BehavioursPanel() : MenuItem("Behaviours") {}

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft->setCursor(pos.x,pos.y);
            header("USB Behaviours:", pos, selected, opened);
            colours(opened);
            tft->setTextSize(1);
            //int connected = 0;
            char buf[100];
            for (int i = 0 ; i < behaviour_manager->behaviours_usb->size() ; i++) {
                DeviceBehaviourUSBBase *usb_behaviour = behaviour_manager->behaviours_usb->get(i);
                if (usb_behaviour->is_connected())
                    sprintf(buf, "%i %19s %s\n", i, usb_behaviour->device->product(), usb_behaviour->get_indicator());
                else
                    sprintf(buf, "%i %10s, disconnected\n", i, usb_behaviour->get_label());
                tft->printf(buf);
            }
            #ifdef ENABLE_USBSERIAL
                pos.y = tft->getCursorY();
                header("Serial-USB Behaviours:", pos, selected, opened);
                for (int i = 0 ; i < behaviour_manager->behaviours_usbserial->size() ; i++) {
                    DeviceBehaviourUSBSerialBase *usbserial_behaviour = behaviour_manager->behaviours_usbserial->get(i);
                    if (usbserial_behaviour->is_connected())
                        sprintf(buf, "%i %19s %s\n", i, usbserial_behaviour->get_label(), usbserial_behaviour->get_indicator());
                    else
                        sprintf(buf, "%i %10s %s, disconnected\n", i, usbserial_behaviour->get_label(), usbserial_behaviour->get_indicator());

                    tft->printf(buf);
                }
            #endif          
            pos.y = tft->getCursorY();
            header("MIDI DIN Behaviours:", pos, selected, opened);
            for (int i = 0 ; i < behaviour_manager->behaviours_serial->size() ; i++) {
                DeviceBehaviourSerialBase *serial_behaviour = behaviour_manager->behaviours_serial->get(i);
                sprintf(buf, "%i %19s %s\n", i, serial_behaviour->get_label(), serial_behaviour->get_indicator());
                tft->printf(buf);
            }
                   

            /*for (int i = 0 ; i < (NUM_USB_MIDI_DEVICES - connected) ; i++) { // blank unused rows
                tft->printf("%21s\n","");
            }*/
            return tft->getCursorY();
        }
};