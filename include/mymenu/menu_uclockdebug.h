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

        tft->printf("ticks: tick=%lu | int=%lu | ext=%lu\n",
                (unsigned long)uClock.tick,
                (unsigned long)uClock.int_clock_tick,
                (unsigned long)uClock.ext_clock_tick);
        tft->printf("ext_clock_us=%lu, ext_interval=%lu\n",
                (unsigned long)uClock.ext_clock_us,
                (unsigned long)uClock.ext_interval);
        tft->printf("external: %s budget=%u age=%luus accepted=%luus\n",
            uClock.isExternalClockStalled() ? "waiting" : "active",
            (unsigned int)uClock.getExternalTicksRemaining(),
            (unsigned long)uClock.getExternalClockPulseAge(),
            (unsigned long)uClock.getLastAcceptedExternalInterval());

        tft->printf("mod_clock_ref=%i, phase_lock_quarters=%u\n", uClock.mod_clock_ref, uClock.phase_lock_quarters);
        tft->printf("mod: step_ref=%i, clock_counter=%i\n", uClock.mod_step_ref, uClock.mod_clock_counter);
    #ifdef UCLOCK_ENABLE_TRACE
        tft->printf("trace: %s dropped=%lu, depth=%u/%u\n",
            uClock.isTraceFrozen() ? "frozen" : "rolling",
                (unsigned long)uClock.getTraceDroppedCount(),
                (unsigned int)uClock.getIntOverflowCounter(),
                (unsigned int)uClock.getExtOverflowCounter());
    #endif

        // for (uint8_t i=0; i < uClock.ext_interval_buffer_size; i++) {
        //     tft->printf("  ext interval buffer[%i]: %lu\n", i, uClock.ext_interval_buffer[i]);
        // }

        for (uint8_t i = 0; i < 2 && i < uClock.track_slots_size; i++) {
            //tft->printf("Track %i: step_counter=%i, mod_step_counter=%u\n", i, uClock.tracks[i].step_counter, uClock.tracks[i].mod_step_counter);
            tft->printf("Tr %i: s_c=", i);
            if (uClock.tracks[i].step_counter < uClock.tracks[0].step_counter)
                this->colours(false, RED);
            else if (uClock.tracks[i].step_counter > uClock.tracks[0].step_counter)
                this->colours(false, GREEN);
            else
                this->colours(false, C_WHITE);
            tft->printf("%lu", (unsigned long)uClock.tracks[i].step_counter);
            this->colours(false, C_WHITE); // Reset text color after printing each track's step counter
            if (uClock.tracks[i].step_counter > uClock.tracks[0].step_counter)
                tft->printf(" (>>)");   // ahead
            else if (uClock.tracks[i].step_counter < uClock.tracks[0].step_counter)
                tft->printf(" (<<)");   // behind
            else 
                tft->printf(" (==)");   // in sync

            tft->printf(" mod_s_c=");
            if (uClock.tracks[i].mod_step_counter > uClock.tracks[0].mod_step_counter)
                this->colours(false, RED);
            else if (uClock.tracks[i].mod_step_counter < uClock.tracks[0].mod_step_counter)
                this->colours(false, GREEN);
            else
                this->colours(false, C_WHITE);
            tft->printf("%u", (unsigned int)uClock.tracks[i].mod_step_counter);
            this->colours(false, C_WHITE); // Reset text color after printing each track's mod step counter           
            if (uClock.tracks[i].mod_step_counter > uClock.tracks[0].mod_step_counter)
                tft->printf(" (>>)");   // ahead
            else if (uClock.tracks[i].mod_step_counter < uClock.tracks[0].mod_step_counter)
                tft->printf(" (<<)");   // behind
            else
                tft->printf(" (==)");   // in sync

            tft->printf("\n");

            // display the current shuffle pattern template, and indicate which step is currently active
            tft->printf("[ ");
            uint8_t shuffle_size = uClock.tracks[i].shuffle.tmplt.size;
            bool has_fired_step = uClock.tracks[i].step_counter > 0;
            uint8_t active_shuffle_step = shuffle_size > 0 && has_fired_step
                ? (uClock.tracks[i].step_counter - 1) % shuffle_size
                : 0;
            for (uint8_t j = 0; j < shuffle_size; j++) {
                bool is_current_step = has_fired_step && j == active_shuffle_step;
                if (uClock.tracks[i].shuffle.tmplt.step[j]<0)
                    this->colours(is_current_step, RED);
                else if (uClock.tracks[i].shuffle.tmplt.step[j]>0)
                    this->colours(is_current_step, GREEN);
                else
                    this->colours(is_current_step, C_WHITE);
                
                tft->printf("%x ", abs(uClock.tracks[i].shuffle.tmplt.step[j]));
                this->colours(false, C_WHITE);
            }
            tft->printf("]\n");

        }
        // tft->printf("Track %i: step_counter=%i, mod_step_counter=%i\n", 1, uClock.tracks[1].step_counter, uClock.tracks[1].mod_step_counter);

        for (int i = 0 ; i < uClock.sync_callback_size; i++) {
            if (uClock.sync_callbacks[i].callback)
                tft->printf("Sync Callback %i: tick=%lu, mod_counter=%u, sync_ref=%u\n",
                            i,
                            (unsigned long)uClock.sync_callbacks[i].tick,
                            (unsigned int)uClock.sync_callbacks[i].mod_counter,
                            (unsigned int)uClock.sync_callbacks[i].sync_ref);
        }

        return tft->getCursorY();
    }

};



#endif