#include "Config.h"

#include "menuitems.h"
#include "bpm.h"

// BPM indicator
class PositionIndicator : public MenuItem {
    public:
        PositionIndicator() : MenuItem("position") {};

        virtual int display(Coord pos, bool selected, bool opened) override {
            //Serial.printf("positionindicator display for %s\n", label);
            tft->setCursor(pos.x,pos.y);
            header("position", pos, selected, opened);
            tft->setTextSize(2);
            if (playing) {
                colours(opened, tft->GREEN, tft->BLACK);
            } else {
                colours(opened, tft->RED, tft->BLACK);
            }
            tft->printf("%04i:%02i:%02i @ %03.2f\n", 
                BPM_CURRENT_PHRASE + 1, 
                BPM_CURRENT_BAR_OF_PHRASE + 1,
                BPM_CURRENT_BEAT_OF_BAR + 1,
                bpm_current
            );

            return tft->getCursorY();
        }

        virtual bool knob_left() {
            set_bpm(bpm_current-1);
            return true;
        }
        virtual bool knob_right() {
            set_bpm(bpm_current+1);
            return true;
        }
};


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
                    if (usb_midi_connected[i] && usb_midi_device[i] && usb_midi_device[i]->idVendor()>0) {
                        connected++;
                        tft->printf("%i %19s\n", i, *(usb_midi_device[i]->product()));
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
