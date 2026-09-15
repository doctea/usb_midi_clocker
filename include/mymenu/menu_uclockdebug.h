#include "Config.h"

#include "menuitems.h"

#ifdef USE_UCLOCK

#include "uClock.h"
class UClockDebugPanel : public MenuItem {
public:
    UClockDebugPanel() : MenuItem("Debug") {
        this->flags.selectable = false;
    }
    virtual int display(Coord pos, bool selected, bool opened) override {
        unsigned long time = millis()/1000;
        tft->setCursor(pos.x,pos.y);

        // live stats
        pos.y = header("uClock:", pos, selected, opened);

        tft->printf("clock_state: %i\n", uClock.clock_state);

        tft->printf("tick_immediately: %lu\n", uClock.tick_immediately);

        tft->printf("PPQN: input %i, output %i\n", uClock.input_ppqn, uClock.output_ppqn);
        tft->printf("Tempo: %.2f\n", uClock.getTempo());

        tft->printf("tick=%i, int_clock_tick=%i\n", uClock.tick, uClock.int_clock_tick);
        tft->printf("ext_clock_tick=%i, ext_interval=%i\n", uClock.ext_clock_tick, uClock.ext_interval);

        tft->printf("mod_step_ref=%i, mod_clock_counter=%i, mod_clock_ref=%i\n", uClock.mod_step_ref, uClock.mod_clock_counter, uClock.mod_clock_ref);

        tft->printf("ext_clock_us=%i, phase_lock_quarters=%i\n", uClock.ext_clock_us, uClock.phase_lock_quarters);

        // for (int i = 0 ; i < uClock.track_slots_size; i++) {
        //     tft->printf("Track %i: step_counter=%i, mod_step_counter=%i\n", i, uClock.tracks[i].step_counter, uClock.tracks[i].mod_step_counter);
        // }
        tft->printf("Track %i: step_counter=%i, mod_step_counter=%i\n", 1, uClock.tracks[1].step_counter, uClock.tracks[1].mod_step_counter);

        for (int i = 0 ; i < uClock.sync_callback_size; i++) {
            if (uClock.sync_callbacks[i].callback)
                tft->printf("Sync Callback %i: tick=%i, mod_counter=%i, sync_ref=%i\n", i, uClock.sync_callbacks[i].tick, uClock.sync_callbacks[i].mod_counter, uClock.sync_callbacks[i].sync_ref);
        }

        return tft->getCursorY();
    }

};



#endif