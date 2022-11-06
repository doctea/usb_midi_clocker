
#ifdef ENABLE_USB
    #include "multi_usb_handlers.h"
    #include "multi_usbserial_handlers.h"
    class USBDevicesPanel : public MenuItem {
        public:
            USBDevicesPanel() : MenuItem("USB Devices") {}

            virtual int display(Coord pos, bool selected, bool opened) override {
                tft->setCursor(pos.x,pos.y);
                header("USB devices:", pos, selected, opened);
                colours(opened);
                tft->setTextSize(1);
                int connected = 0;
                for (int i = 0 ; i < NUM_USB_MIDI_DEVICES ; i++) {
                    if (usb_midi_slots[i].packed_id && usb_midi_slots[i].device && usb_midi_slots[i].device->idVendor()>0) {
                        connected++;
                        char buf[100];
                        sprintf(buf, "%i %19s\n", i, usb_midi_slots[i].device->product());
                        tft->printf(buf);
                    }            
                }
                for (int i = 0 ; i < NUM_USB_SERIAL_DEVICES ; i++) {
                    if (usb_serial_slots[i].packed_id && usb_serial_slots[i].usbdevice && usb_serial_slots[i].usbdevice->idVendor()>0) {
                        connected++;
                        char buf[100];
                        sprintf(buf, "%i %19s\n", i, usb_serial_slots[i].usbdevice->product());
                        tft->printf(buf);
                    }
                }
                return tft->getCursorY();
            }
    };
#endif
