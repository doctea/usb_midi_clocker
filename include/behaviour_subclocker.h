#ifndef BEHAVIOUR_SUBCLOCKER__INCLUDED
#define BEHAVIOUR_SUBCLOCKER__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviour_base_usb.h"
#include "bpm.h"

#include "multi_usb_handlers.h"

/*void subclocker_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void subclocker_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void subclocker_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);*/

/*void on_subclocker_divisor_changed(int new_value, int old_value) {

}*/

class DeviceBehaviour_Subclocker : public DeviceBehaviourUSBBase, public ClockedBehaviour {
    public:
        uint16_t vid = 0x1337, pid = 0x1337;
        virtual uint32_t get_packed_id() override { return (this->vid<<16 | this->pid); }

        virtual const char *get_label() override {
            return "Subclocker";
        }

        unsigned long clock_delay_ticks = DEFAULT_SUBCLOCKER_DELAY_TICKS;
        int clock_divisor = DEFAULT_SUBCLOCKER_DIVISOR;

        virtual void set_delay_ticks(int delay_ticks) {
            this->clock_delay_ticks = delay_ticks;
        }
        virtual int get_delay_ticks() {
            return this->clock_delay_ticks;
        }
        virtual void set_divisor (int divisor) {
            this->clock_divisor = divisor;
        }
        virtual int get_divisor() {
            return this->clock_divisor;
        }

        int32_t real_ticks = 0;
        virtual void send_clock(unsigned long ticks) override {
            this->real_ticks = ticks;
            if (ticks<clock_delay_ticks) return;

            /*if (is_bpm_on_phrase(real_ticks - clock_delay_ticks)) {
                DeviceBehaviourUSBBase::on_phrase(BPM_CURRENT_PHRASE);
            }*/
            if (is_bpm_on_bar(real_ticks - clock_delay_ticks)) {
                ClockedBehaviour::on_bar(BPM_CURRENT_BAR_OF_PHRASE);
            }

            if (/*ticks==0 || */ticks % clock_divisor == 0)
                ClockedBehaviour::send_clock(ticks - clock_delay_ticks);
        }
        
        virtual void on_bar(int bar_number) override {
            // don't do anything - handle the delayed clocks in send_clock
            //if (is_bpm_on_bar(real_ticks - clock_delay_ticks))
        }
        virtual void on_phrase(uint32_t phrase_number) override {
            // don't do anything - handle the delayed clocks in send_clock
        }

        virtual void on_restart() override {
            //Serial.println("\ton_restart() in ClockedBehaviour");
            if (DeviceBehaviourUSBBase::is_connected() && this->clock_enabled) {
                DeviceBehaviourUSBBase::sendRealTime(MIDIDevice::Stop); //sendStop();
                DeviceBehaviourUSBBase::sendRealTime(MIDIDevice::Start); //sendStart();
                DeviceBehaviourUSBBase::sendNow();
                this->started = true;
            } else {
                //Serial.println("\tin clocked behaviour on_restart, no device!");
            }
        }

};

extern DeviceBehaviour_Subclocker *behaviour_subclocker;

#endif