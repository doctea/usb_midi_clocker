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
            unsigned int i = 0;
            for (auto* usb_behaviour : *behaviour_manager->behaviours_usb) {
                tft->setTextColor(usb_behaviour->colour, BLACK);
                if (usb_behaviour->is_connected())
                    snprintf(buf, MAX_LENGTH, standard_format_connected, i, usb_behaviour->device->product(), usb_behaviour->get_indicator());
                else
                    snprintf(buf, MAX_LENGTH, standard_format_disconnected, i, usb_behaviour->get_label());
                tft->printf(buf);
                ++i;
            }
            
            #ifdef ENABLE_USBSERIAL
                pos.y = tft->getCursorY();
                header("Serial-USB Behaviours:", pos, selected, opened);
                unsigned int j = 0;
                for (auto* usbserial_behaviour : *behaviour_manager->behaviours_usbserial) {
                    tft->setTextColor(usbserial_behaviour->colour, BLACK);
                    if (usbserial_behaviour->is_connected())
                        snprintf(buf, MAX_LENGTH, standard_format_connected, j, usbserial_behaviour->get_label(), usbserial_behaviour->get_indicator());
                    else
                        snprintf(buf, MAX_LENGTH, standard_format_disconnected, j, usbserial_behaviour->get_label(), usbserial_behaviour->get_indicator());

                    tft->printf(buf);
                    ++j;
                }
            #endif          

            pos.y = tft->getCursorY();
            header("MIDI DIN Behaviours:    i O", pos, selected, opened);
            unsigned int k = 0;
            for (auto* serial_behaviour : *behaviour_manager->behaviours_serial) {
                tft->setTextColor(serial_behaviour->colour, BLACK);
                snprintf(buf, MAX_LENGTH, "%i %19s   %s\n", k, serial_behaviour->get_label(), serial_behaviour->get_indicator());
                tft->printf(buf);
                ++k;
            }

            pos.y = tft->getCursorY();
            header("Virtual Behaviours:", pos, selected, opened);
            unsigned int l = 0;
            for (auto* virtual_behaviour : *behaviour_manager->behaviours_virtual) {
                tft->setTextColor(virtual_behaviour->colour, BLACK);
                snprintf(buf, MAX_LENGTH, standard_format_connected, l, virtual_behaviour->get_label(), virtual_behaviour->get_indicator());
                tft->printf(buf);
                ++l;
            }

            /*for (unsigned int i = 0 ; i < (NUM_USB_MIDI_DEVICES - connected) ; i++) { // blank unused rows
                tft->printf("%21s\n","");
            }*/
            return tft->getCursorY();
        }
};