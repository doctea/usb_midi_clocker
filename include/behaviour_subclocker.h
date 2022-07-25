#ifndef BEHAVIOUR_SUBCLOCKER__INCLUDED
#define BEHAVIOUR_SUBCLOCKER__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviour_base.h"
#include "bpm.h"

#include "multi_usb_handlers.h"

/*void subclocker_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void subclocker_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void subclocker_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);*/

/*void on_subclocker_divisor_changed(int new_value, int old_value) {

}*/

class DeviceBehaviour_Subclocker : public ClockedBehaviour {
    public:
        uint16_t vid = 0x1337, pid = 0x1337;
        virtual uint32_t get_packed_id() override { return (this->vid<<16 | this->pid); }

        unsigned long clock_delay_ticks = DEFAULT_SUBCLOCKER_DELAY_TICKS;
        int clock_divisor = DEFAULT_SUBCLOCKER_DIVISOR;

        void set_delay_ticks(int delay_ticks) {
            this->clock_delay_ticks = delay_ticks;
        }
        int get_delay_ticks() {
            return this->clock_delay_ticks;
        }
        void set_divisor (int divisor) {
            this->clock_divisor = divisor;
        }
        int get_divisor() {
            return this->clock_divisor;
        }

        void send_clock(unsigned long ticks) override {
            if (ticks<clock_delay_ticks) return;

            if (ticks==0 || ticks % clock_divisor == 0)
                ClockedBehaviour::send_clock(ticks - clock_delay_ticks);
        }
        
};

extern DeviceBehaviour_Subclocker *behaviour_subclocker;

#endif