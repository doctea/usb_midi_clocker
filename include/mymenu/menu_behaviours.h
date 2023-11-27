#include "menuitems.h"

#include "behaviours/behaviour_base.h"
#include "behaviours/behaviour_manager.h"

class BehavioursPanel : /*virtual*/ public MenuItem {
    public:
        BehavioursPanel() : MenuItem("Behaviours") {}

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft->setCursor(pos.x,pos.y);
            header("MIDI-USB Behaviours:", pos, selected, opened);
            colours(opened);
            tft->setTextSize(1);
            //int connected = 0;

            const char *standard_format_connected    = "%i %19s %s\n";
            const char *standard_format_disconnected = "%i %10s, disconnected\n";

            #define MAX_LENGTH 100
            char buf[MAX_LENGTH];
            for (unsigned int i = 0 ; i < behaviour_manager->behaviours_usb->size() ; i++) {
                DeviceBehaviourUSBBase *usb_behaviour = behaviour_manager->behaviours_usb->get(i);
                tft->setTextColor(usb_behaviour->colour, BLACK);
                if (usb_behaviour->is_connected())
                    snprintf(buf, MAX_LENGTH, standard_format_connected, i, usb_behaviour->device->product(), usb_behaviour->get_indicator());
                else
                    snprintf(buf, MAX_LENGTH, standard_format_disconnected, i, usb_behaviour->get_label());
                tft->printf(buf);
            }
            
            #ifdef ENABLE_USBSERIAL
                pos.y = tft->getCursorY();
                header("Serial-USB Behaviours:", pos, selected, opened);
                for (unsigned int i = 0 ; i < behaviour_manager->behaviours_usbserial->size() ; i++) {
                    DeviceBehaviourUSBSerialBase *usbserial_behaviour = behaviour_manager->behaviours_usbserial->get(i);
                    tft->setTextColor(usbserial_behaviour->colour, BLACK);
                    if (usbserial_behaviour->is_connected())
                        snprintf(buf, MAX_LENGTH, standard_format_connected, i, usbserial_behaviour->get_label(), usbserial_behaviour->get_indicator());
                    else
                        snprintf(buf, MAX_LENGTH, standard_format_disconnected, i, usbserial_behaviour->get_label(), usbserial_behaviour->get_indicator());

                    tft->printf(buf);
                }
            #endif          

            pos.y = tft->getCursorY();
            header("MIDI DIN Behaviours:    i O", pos, selected, opened);
            for (unsigned int i = 0 ; i < behaviour_manager->behaviours_serial->size() ; i++) {
                DeviceBehaviourSerialBase *serial_behaviour = behaviour_manager->behaviours_serial->get(i);
                tft->setTextColor(serial_behaviour->colour, BLACK);
                snprintf(buf, MAX_LENGTH, "%i %19s   %s\n", i, serial_behaviour->get_label(), serial_behaviour->get_indicator());
                tft->printf(buf);
            }

            pos.y = tft->getCursorY();
            header("Virtual Behaviours:", pos, selected, opened);
            for (unsigned int i = 0 ; i < behaviour_manager->behaviours_virtual->size() ; i++) {
                DeviceBehaviourUltimateBase *virtual_behaviour = behaviour_manager->behaviours_virtual->get(i);
                tft->setTextColor(virtual_behaviour->colour, BLACK);
                snprintf(buf, MAX_LENGTH, standard_format_connected, i, virtual_behaviour->get_label(), virtual_behaviour->get_indicator());
                tft->printf(buf);
            }

            /*for (unsigned int i = 0 ; i < (NUM_USB_MIDI_DEVICES - connected) ; i++) { // blank unused rows
                tft->printf("%21s\n","");
            }*/
            return tft->getCursorY();
        }
};