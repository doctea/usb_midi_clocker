
#ifdef ENABLE_USB
    #include "multi_usb_handlers.h"
    #ifdef ENABLE_USBSERIAL
        #include "multi_usbserial_handlers.h"
    #endif
    #ifdef ENABLE_TYPING_KEYBOARD
        //#include "input_keyboard.h"
        #include "USBHost_t36.h"
        extern KeyboardController keyboard1;
    #endif
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
                #ifdef ENABLE_USBSERIAL
                    pos.y = tft->getCursorY();
                    header("Serial-USB devices:", pos, selected, opened);
                    for (int i = 0 ; i < NUM_USB_SERIAL_DEVICES ; i++) {
                        if (usb_serial_slots[i].packed_id && usb_serial_slots[i].usbdevice && usb_serial_slots[i].usbdevice->idVendor()>0) {
                            connected++;
                            char buf[100];
                            sprintf(buf, "%i %19s\n", i, usb_serial_slots[i].usbdevice->product());
                            tft->printf(buf);
                        }
                    }
                #endif
                #ifdef ENABLE_TYPING_KEYBOARD
                    if (keyboard1)
                        tft->printf("K %19s\n", (char*)keyboard1.product());
                    else
                        tft->printf("K (Typing disconnected)\n");
                #endif
                tft->println();
                tft->printf("Free RAM: %u bytes\n", freeRam());

                return tft->getCursorY();
            }
    };
#endif
