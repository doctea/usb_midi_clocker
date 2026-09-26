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
        tft->setCursor(pos.x,pos.y);

        // live stats
        pos.y = header("uClock:", pos, selected, opened);

        tft->printf("clock_state: %i, tick_immediately: %i\n", uClock.clock_state, uClock.tick_immediately);

        tft->printf("PPQN: input %i, output %i, tempo %.2f\n", uClock.input_ppqn, uClock.output_ppqn, uClock.getTempo());

        tft->printf("ticks: tick=%ul | int=%ul | ext=%ul\n", uClock.tick, uClock.int_clock_tick, uClock.ext_clock_tick);
        tft->printf("ext_clock_us=%i, ext_interval=%ul\n", uClock.ext_clock_us, uClock.ext_interval);

        tft->printf("mod_clock_ref=%i, phase_lock_quarters=%u\n", uClock.mod_clock_ref, uClock.phase_lock_quarters);
        tft->printf("mod: step_ref=%i, clock_counter=%i\n", uClock.mod_step_ref, uClock.mod_clock_counter);

        // for (uint8_t i=0; i < uClock.ext_interval_buffer_size; i++) {
        //     tft->printf("  ext interval buffer[%i]: %lu\n", i, uClock.ext_interval_buffer[i]);
        // }

        for (int i = 0 ; i < 2 /*uClock.track_slots_size*/; i++) {
            //tft->printf("Track %i: step_counter=%i, mod_step_counter=%u\n", i, uClock.tracks[i].step_counter, uClock.tracks[i].mod_step_counter);
            tft->printf("Tr %i: s_c=", i);
            if (uClock.tracks[i].step_counter > uClock.tracks[0].step_counter)
                this->colours(false, GREEN);
            else if (uClock.tracks[i].step_counter < uClock.tracks[0].step_counter)
                this->colours(false, RED);
            else
                this->colours(false, C_WHITE);
            tft->printf("%lu\n", uClock.tracks[i].step_counter);
            this->colours(false, C_WHITE); // Reset text color after printing each track's step counter
            if (uClock.tracks[i].step_counter > uClock.tracks[0].step_counter)
                tft->printf(" (>>)");   // ahead
            else if (uClock.tracks[i].step_counter < uClock.tracks[0].step_counter)
                tft->printf(" (<<)");   // behind

            tft->printf(" mod_s_c=");
            if (uClock.tracks[i].mod_step_counter > uClock.tracks[0].mod_step_counter)
                this->colours(false, GREEN);
            else if (uClock.tracks[i].mod_step_counter < uClock.tracks[0].mod_step_counter)
                this->colours(false, RED);
            else
                this->colours(false, C_WHITE);
            tft->printf("%lu", uClock.tracks[i].mod_step_counter);
            this->colours(false, C_WHITE); // Reset text color after printing each track's mod step counter           
            if (uClock.tracks[i].mod_step_counter > uClock.tracks[0].mod_step_counter)
                tft->printf(" (>>)");   // ahead
            else if (uClock.tracks[i].mod_step_counter < uClock.tracks[0].mod_step_counter)
                tft->printf(" (<<)");   // behind

            // display the current shuffle pattern template, and indicate which step is currently active
            tft->printf(" shuffle_pattern=");
            for (int j = 0; j < uClock.tracks[i].shuffle.tmplt.size; j++) {
                if (uClock.tracks[i].shuffle.tmplt.step[j]<0)
                    this->colours(j == uClock.tracks[i].step_counter, RED);
                else if (uClock.tracks[i].shuffle.tmplt.step[j]>0)
                    this->colours(j == uClock.tracks[i].step_counter, GREEN);
                else
                    this->colours(j == uClock.tracks[i].step_counter, C_WHITE);
                
                tft->printf("%i ", uClock.tracks[i].shuffle.tmplt.step[j]);
                this->colours(false, C_WHITE);
            }
            tft->printf("\n");

        }
        // tft->printf("Track %i: step_counter=%i, mod_step_counter=%i\n", 1, uClock.tracks[1].step_counter, uClock.tracks[1].mod_step_counter);

        for (int i = 0 ; i < uClock.sync_callback_size; i++) {
            if (uClock.sync_callbacks[i].callback)
                tft->printf("Sync Callback %i: tick=%ul, mod_counter=%u, sync_ref=%ul\n", i, uClock.sync_callbacks[i].tick, uClock.sync_callbacks[i].mod_counter, uClock.sync_callbacks[i].sync_ref);
        }

        return tft->getCursorY();
    }

};



#endif