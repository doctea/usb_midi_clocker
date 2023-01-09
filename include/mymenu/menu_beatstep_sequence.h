#include "behaviours/behaviour_beatstep.h"
#include "menuitems.h"
//#include "submenuitem_bar.h"
//#include "menuitems_object_selector.h"

//#define NUM_STEPS 16

class BeatstepSequenceDisplay : public MenuItem {
    public:
        DeviceBehaviour_Beatstep *behaviour_beatstep = nullptr;
        BeatstepSequenceDisplay(const char *label, DeviceBehaviour_Beatstep *behaviour_beatstep) : MenuItem(label) {
            this->behaviour_beatstep = behaviour_beatstep;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);

            tft->setCursor(pos.x, pos.y);

            uint16_t base_row = pos.y;
            //static float ticks_per_pixel = (float)LOOP_LENGTH_TICKS / (float)tft->width();

            // we're going to use direct access to the underlying Adafruit library here
            const DisplayTranslator_STeensy *tft2 = (DisplayTranslator_STeensy*)tft;
            ST7789_t3 *actual = tft2->tft;

            #define STEP_WIDTH 8
            #define STEP_HEIGHT 8
            #define STEP_GAP 2

            for (int i = 0 ; i < NUM_BEATSTEP_STEPS ; i++) {
                int row = ((i / 8));
                int col = i % 8;

                int x = col * (STEP_WIDTH+STEP_GAP);
                int y = base_row + (row*(STEP_HEIGHT+STEP_GAP));
                if (behaviour_beatstep->sequence->get_state_at_step(i)) 
                    actual->fillRect(x, y, STEP_WIDTH, STEP_HEIGHT, BLUE);
                else
                    actual->drawRect(x, y, STEP_WIDTH, STEP_HEIGHT, GREY);
            }

            base_row += 2 * (STEP_HEIGHT + STEP_GAP);
            tft->setCursor(0, base_row);

            for (int i = 0 ; i < NUM_BEATSTEP_STEPS ; i++) {
                tft->printf("%02i: ", i);
                tft->printf(get_note_name_c(behaviour_beatstep->sequence->get_pitch_at_step(i)));
                tft->println();
            }
            pos.y = tft->getCursorY();

            return pos.y; // + 40; //row + 12; //pos.y + (first_found - last_found); //127;  
        }
};