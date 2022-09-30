#include "Config.h"

#include "menu.h"
#include "menuitems.h"
#include "bpm.h"

#include "clock.h"

// BPM indicator
class BPMPositionIndicator : public MenuItem {
    public:
        BPMPositionIndicator() : MenuItem("position") {};

        virtual int display(Coord pos, bool selected, bool opened) override {
            //Serial.printf("positionindicator display for %s\n", label);
            tft->setCursor(pos.x,pos.y);
            header("position", pos, selected, opened);
            tft->setTextSize(2);
            if (playing) {
                colours(opened, GREEN,   BLACK);
            } else {
                colours(opened, RED,     BLACK);
            }
            if (clock_mode==CLOCK_INTERNAL) {
                tft->printf((char*)"%04i:%02i:%02i @ %03.2f\n", 
                    BPM_CURRENT_PHRASE + 1, 
                    BPM_CURRENT_BAR_OF_PHRASE + 1,
                    BPM_CURRENT_BEAT_OF_BAR + 1,
                    bpm_current
                );
            } else if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
                tft->printf((char*)"%04i:%02i:%02i\n",
                    BPM_CURRENT_PHRASE + 1, 
                    BPM_CURRENT_BAR_OF_PHRASE + 1,
                    BPM_CURRENT_BEAT_OF_BAR + 1
                );
                tft->setTextSize(1);
                tft->println((char*)"from External USB Host");
            } else {
                tft->printf((char*)"%04i:%02i:%02i\n",
                    BPM_CURRENT_PHRASE + 1, 
                    BPM_CURRENT_BAR_OF_PHRASE + 1,
                    BPM_CURRENT_BEAT_OF_BAR + 1
                );
            }

            return tft->getCursorY();
        }

        virtual bool knob_left() {
            if (clock_mode==CLOCK_INTERNAL)
                set_bpm(bpm_current-1);
            return true;
        }
        virtual bool knob_right() {
            if (clock_mode==CLOCK_INTERNAL)
                set_bpm(bpm_current+1);
            return true;
        }
};

