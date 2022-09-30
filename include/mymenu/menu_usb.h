
#ifdef ENABLE_USB
    #include "multi_usb_handlers.h"
    class USBDevicesPanel : public MenuItem {
        public:
            USBDevicesPanel() : MenuItem("USB Devices") {}

            virtual int display(Coord pos, bool selected, bool opened) override {
                tft->setCursor(pos.x,pos.y);
                header("USB devices:", pos, selected, opened);
                colours(opened);
                tft->setTextSize(1);
                int connected = 0;
                for (int i = 0 ; i < NUM_USB_DEVICES ; i++) {
                    if (usb_midi_slots[i].packed_id && usb_midi_slots[i].device && usb_midi_slots[i].device->idVendor()>0) {
                        connected++;
                        char buf[100];
                        sprintf(buf, "%i %19s\n", i, usb_midi_slots[i].device->product());
                        tft->printf(buf);
                        //tft->printf("%i %19s\n", i, *usb_midi_device[i]->product());
                        //tft->printf("%08x\n", usb_midi_connected[i]);  // packed usb vendor+product id
                    }            
                }
                /*for (int i = 0 ; i < (NUM_USB_DEVICES - connected) ; i++) { // blank unused rows
                    tft->printf("%21s\n","");
                }*/
                return tft->getCursorY();
            }
    };
#endif
