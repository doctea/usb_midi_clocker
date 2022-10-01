#ifndef BEHAVIOUR_SUBCLOCKER__INCLUDED
#define BEHAVIOUR_SUBCLOCKER__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviours/behaviour_base_usb.h"
#include "behaviours/behaviour_clocked.h"
#include "bpm.h"

#include "multi_usb_handlers.h"

/*void subclocker_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void subclocker_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void subclocker_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);*/
/*void on_subclocker_divisor_changed(int new_value, int old_value) {
}*/

class DeviceBehaviour_Subclocker : public DeviceBehaviourUSBBase, public DividedClockedBehaviour {
    using DividedClockedBehaviour::on_restart;
    public:
        uint16_t vid = 0x1337, pid = 0x1337;
        virtual uint32_t get_packed_id() override { return (this->vid<<16 | this->pid); }

        virtual const char *get_label() override {
            return "Subclocker";
        }
};

extern DeviceBehaviour_Subclocker *behaviour_subclocker;

#endif