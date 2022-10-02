#ifndef BEHAVIOUR_CLOCKED__INCLUDED
#define BEHAVIOUR_CLOCKED__INCLUDED

#include "Config.h"
#include "behaviour_base.h"

class ClockedBehaviour : virtual public DeviceBehaviourUltimateBase {
    public:
        bool restart_on_bar = true;
        bool started = false;
        bool clock_enabled = true;

        virtual bool should_show_restart_option() {
            return false;
        }

        virtual void send_clock(uint32_t ticks) override {
            if (!is_connected()) return;
            this->sendRealTime(midi::Clock);
            this->sendNow();
        }

        virtual void on_bar(int bar_number) override {
            if (this->restart_on_bar) {
                Serial.printf("%s:\tClockedBehaviour#on_bar and restart_on_bar set!\n", this->get_label());
                this->restart_on_bar = false;
                this->on_restart();
            }
        }
        virtual bool is_set_restart_on_bar() {
            return this->restart_on_bar;
        }
        virtual void set_restart_on_bar(bool v = true) {
            this->restart_on_bar = v;
        }
        virtual const char *get_restart_on_bar_status_label(bool value) {
            if (value) 
                return "Restarting on bar..";
            else 
                return "Trigger restart on bar";
        }

        virtual void on_restart() override {
            if (!is_connected()) return;

            if (this->clock_enabled) {
                this->sendRealTime(midi::Stop);
                this->sendRealTime(midi::Start);
                this->sendNow();
                this->started = true;
            }
        }

        virtual void setClockEnabled(bool enabled) {
            this->clock_enabled = enabled;
        }
        virtual bool isClockEnabled() {
            return this->clock_enabled;
        }

        #ifdef ENABLE_SCREEN
            virtual LinkedList<MenuItem*> *make_menu_items() override;
        #endif
};

#include "bpm.h"
#include "MIDI.h"

class DividedClockedBehaviour : public ClockedBehaviour {
    public:
        long clock_delay_ticks = 0; //DEFAULT_DELAY_TICKS;
        int clock_divisor = 1; //DEFAULT_DIVISOR;
        bool auto_restart_on_change = true;

        virtual bool should_show_restart_option() override {
            return true;
        }

        // check if we should set the restart_on_bar flag when one of the delay_ticks or divisor value changes
        bool should_auto_restart_on_change () {
            return this->auto_restart_on_change;
        }
        // set if we should set the restart_on_bar flag when one of the delay_ticks or divisor value changes
        void set_auto_restart_on_change(bool value) {
            this->auto_restart_on_change = value;
        }

        // set how many ticks we should wait after a restart before we start playing (effectively an offset)
        virtual void set_delay_ticks(int delay_ticks) {
            if ((uint16_t)delay_ticks != this->clock_delay_ticks && this->should_auto_restart_on_change())
                this->set_restart_on_bar(true);
            this->clock_delay_ticks = delay_ticks;
        }
        virtual int get_delay_ticks() {
            return this->clock_delay_ticks;
        }

        // set how many real ticks count for one of our internal ticks -- for use in doing half-time, etc 
        virtual void set_divisor (int divisor) {
            if (divisor!=this->get_divisor() && this->should_auto_restart_on_change()) 
                this->set_restart_on_bar(true);
            this->clock_divisor = divisor;
        }
        virtual int get_divisor() {
            return this->clock_divisor;
        }

        int32_t real_ticks = 0;
        bool waiting = true;
        virtual void send_clock(unsigned long ticks) override {
            real_ticks = ticks%(PPQN*BEATS_PER_BAR*BARS_PER_PHRASE);
            if (waiting && (real_ticks) < clock_delay_ticks) {
                if (this->debug) Serial.printf("DividedClockBehaviour with phrase tick %i, not sending because real_ticks %i haven't reached clock_delay_ticks of %i\n", ticks%(PPQN*BEATS_PER_BAR*BARS_PER_PHRASE), real_ticks, clock_delay_ticks);
                return;
            }
            if (waiting) {
                Serial.printf("%s: DividedClockBehaviour with real_ticks %i and clock_delay_ticks %i was waiting\n", this->get_label(), real_ticks, clock_delay_ticks);
                //this->on_restart(); = true;
                this->started = true;
                this->sendRealTime((uint8_t)(midi::Start)); //sendStart();
                if (this->debug) Serial.println("\tnot waiting anymore!\n");
                waiting = false;
            }
            /*if (real_ticks++ < clock_delay_ticks && clock_delay_ticks>0) {
                Serial.printf("DividedClockBehaviour with tick %i, not sending because real_ticks %i haven't reached clock_delay_ticks of %i\n", ticks, real_ticks, clock_delay_ticks);
                return;
            }*/
            //this->real_ticks++; // = ticks;

            /*if (is_bpm_on_phrase(real_ticks - clock_delay_ticks)) {
                DeviceBehaviourUSBBase::on_phrase(BPM_CURRENT_PHRASE);
            }*/
            if (is_bpm_on_bar(real_ticks, clock_delay_ticks)) {
                Serial.printf("%s: DividedClockBehaviour with real_ticks %i and clock_delay_ticks %i confirmed yes for is_bpm_on_bar, called ClockedBehaviour::on_bar\n", this->get_label(), real_ticks, clock_delay_ticks);
                ClockedBehaviour::on_bar(BPM_CURRENT_BAR_OF_PHRASE);
            }

            if (/*ticks==0 || */ticks % clock_divisor == 0)
                ClockedBehaviour::send_clock(ticks - clock_delay_ticks);
        }
        
        // actually, we do want to respond to on_bar, because that's when we actually need to start the delay from!
        virtual void on_bar(int bar_number) override {
            // don't do anything - handle the delayed clocks in send_clock
            //if (is_bpm_on_bar(real_ticks - clock_delay_ticks))
        }
        virtual void on_phrase(uint32_t phrase_number) override {
            // don't do anything - handle the delayed clocks in send_clock
        }

        virtual void on_restart() override {
            Serial.printf("%s: on_restart() in DividedClockedBehaviour\n", this->get_label());
            if (this->is_connected() && this->clock_enabled) {
                this->sendRealTime((uint8_t)(midi::Stop)); //sendStop();
                //this->sendRealTime((uint8_t)(midi::Start)); //sendStart();
                //this->sendNow();
                this->started = false;
                this->real_ticks = 0;
                this->waiting = true;
            } else {
                Serial.println("\tin DividedClockedBehaviour on_restart, no device!");
            }
        }

        virtual void save_sequence_add_lines(LinkedList<String> *lines) override {
            ClockedBehaviour::save_project_add_lines(lines);
            lines->add(String("divisor=") + String(this->clock_divisor));
            lines->add(String("delay_ticks=") + String(this->clock_delay_ticks));
        }

        // ask behaviour to process the key/value pair
        virtual bool load_parse_key_value(String key, String value) override {
            if (key.equals("divisor")) {
                this->set_divisor((int) value.toInt());
                return true;
            } else if (key.equals("delay_ticks")) {
                this->set_delay_ticks((int) value.toInt());
                return true;
            } else if (ClockedBehaviour::load_parse_key_value(key, value)) {
                return true;
            }

            return false;
        }

        #ifdef ENABLE_SCREEN
            virtual LinkedList<MenuItem*> *make_menu_items() override;
        #endif
};

#endif