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
            //Serial.printf("ClockedBehaviour#on_bar in %p\n", this);
            if (this->restart_on_bar) {
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
            virtual LinkedList<MenuItem*> make_menu_items() override;
        #endif
};

#include "bpm.h"
#include "MIDI.h"

class DividedClockedBehaviour : public ClockedBehaviour {
    public:
        unsigned long clock_delay_ticks = 0; //DEFAULT_DELAY_TICKS;
        int clock_divisor = 1; //DEFAULT_DIVISOR;
        
        virtual bool should_show_restart_option() override {
            return true;
        }

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
            Serial.println("\ton_restart() in DividedClockedBehaviour");
            if (this->is_connected() && this->clock_enabled) {
                this->sendRealTime((uint8_t)(midi::Stop)); //sendStop();
                this->sendRealTime((uint8_t)(midi::Start)); //sendStart();
                //this->sendNow();
                this->started = true;
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
            virtual LinkedList<MenuItem*> make_menu_items() override;
        #endif
};

#endif