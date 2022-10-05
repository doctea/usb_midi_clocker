#ifndef BEHAVIOUR_CLOCKED__INCLUDED
#define BEHAVIOUR_CLOCKED__INCLUDED

#include "Config.h"
#include "behaviour_base.h"

#include "midi/midi_mapper_matrix_manager.h"

class ClockedBehaviour : virtual public DeviceBehaviourUltimateBase {
    public:
        bool restart_on_bar = true;
        bool started = false;
        bool clock_enabled = true;

        bool stop_notes_when_paused = false;

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
        uint32_t clock_delay_ticks = 0; //DEFAULT_DELAY_TICKS;
        uint32_t clock_divisor = 1; //DEFAULT_DIVISOR;
        bool auto_restart_on_change = true;
        byte pause_during_delay = false;

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
        virtual void set_delay_ticks(uint32_t delay_ticks) {
            if ((uint32_t)delay_ticks != this->clock_delay_ticks && this->should_auto_restart_on_change())
                this->set_restart_on_bar(true);
            this->clock_delay_ticks = delay_ticks;
        }
        virtual uint32_t get_delay_ticks() {
            return this->clock_delay_ticks;
        }

        // set how many real ticks count for one of our internal ticks -- for use in doing half-time, etc 
        virtual void set_divisor (uint32_t divisor) {
            if (divisor!=this->get_divisor() && this->should_auto_restart_on_change()) 
                this->set_restart_on_bar(true);
            this->clock_divisor = divisor;
        }
        virtual uint32_t get_divisor() {
            return this->clock_divisor;
        }

        // pause during delay
        enum DELAY_PAUSE {
            PAUSE_OFF, PAUSE_BAR, PAUSE_TWO_BAR, PAUSE_PHRASE
        };
        virtual void set_pause_during_delay (uint32_t value) {
            this->pause_during_delay = value;
        }
        virtual uint32_t get_pause_during_delay() {
            return this->pause_during_delay;
        }
        /*virtual bool should_pause_during_delay(uint32_t type) {
            bool should = this->pause_during_delay >= type;
        }
        virtual bool should_pause_during_delay_bar_number(uint32_t bar_number) {
            // todo: this needs fixing so that it returns true under correct circumstances!

            if (should_pause_during_delay(DELAY_PAUSE::PAUSE_TWO_BAR))
                return bar_number % 2 == 0;
            return false;
        }*/
        virtual bool should_pause_during_bar_number(uint32_t bar_number) {
            bar_number %= BARS_PER_PHRASE;
            switch(this->pause_during_delay) {
                case PAUSE_OFF: return false;
                case PAUSE_BAR: return true;
                case PAUSE_TWO_BAR: return (bar_number%2==0);
                case PAUSE_PHRASE: return bar_number==0;
                default: return false;
            }
        }

        int32_t real_ticks = 0;
        bool waiting = true;
        virtual void send_clock(unsigned long ticks) override {
            uint32_t tick_of_phrase = (ticks%(PPQN*BEATS_PER_BAR)); //*BARS_PER_PHRASE));
            //real_ticks = (ticks%(PPQN*BEATS_PER_BAR*BARS_PER_PHRASE)) - clock_delay_ticks;
            //real_ticks++;
            real_ticks = (int32_t)tick_of_phrase - clock_delay_ticks;
            //if (this->waiting && real_ticks<0) { //(real_ticks) < clock_delay_ticks) {
            if (this->waiting && tick_of_phrase < clock_delay_ticks) { //(real_ticks) < clock_delay_ticks) {
                if (this->debug) Serial.printf("DividedClockBehaviour with phrase tick %i, not sending because real_ticks %i haven't reached clock_delay_ticks of %i\n", ticks%(PPQN*BEATS_PER_BAR*BARS_PER_PHRASE), real_ticks, clock_delay_ticks);
                return;
            }
            if (waiting) {
                Serial.printf("%s: DividedClockBehaviour with real_ticks %i and clock_delay_ticks %i was waiting\n", this->get_label(), real_ticks, clock_delay_ticks);
                //this->on_restart(); = true;
                this->started = true;
                this->sendRealTime((uint8_t)(midi::Stop)); //sendStart();
                this->sendRealTime((uint8_t)(midi::Start)); //sendStart();
                if (this->debug) Serial.printf("%s:\tunsetting waitin!\n", this->get_label());
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
            if (is_bpm_on_bar(real_ticks)) { //}, clock_delay_ticks)) {
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
            //if (this->should_pause_during_delay_bar_number(bar_number) && this->get_delay_ticks()>0)
            if (this->get_delay_ticks()>0 && this->should_pause_during_bar_number(bar_number)) {
                this->sendRealTime((uint8_t)(midi::Stop));
                if (this->stop_notes_when_paused && this->target_id!=-1) {
                    MIDIOutputWrapper *tgt = midi_matrix_manager->get_target_for_id(this->target_id);
                    if (tgt!=nullptr) tgt->stop_all_notes();
                }
                this->waiting = true;
            }
        }
        virtual void on_phrase(uint32_t phrase_number) override {
            // don't do anything - handle the delayed clocks in send_clock
            //if (this->should_pause_during_delay(DELAY_PAUSE::PAUSE_PHRASE) && this->get_delay_ticks()>0)
            //    this->waiting = true;
        }

        virtual void on_restart() override {
            Serial.printf("%s: on_restart() in DividedClockedBehaviour\n", this->get_label());
            if (this->is_connected() && this->clock_enabled) {
                this->sendRealTime((uint8_t)(midi::Stop)); //sendStop();
                //this->sendRealTime((uint8_t)(midi::Start)); //sendStart();
                //this->sendNow();
                this->started = false;
                //this->real_ticks = this->clock_delay_ticks * -1;
                this->waiting = true;
                Serial.printf("%s:\tsetting waiting!\n", this->get_label());
            } else {
                Serial.println("\tin DividedClockedBehaviour on_restart, no device!");
            }
        }

        virtual void save_sequence_add_lines(LinkedList<String> *lines) override {
            ClockedBehaviour::save_project_add_lines(lines);
            lines->add(String("divisor=") + String(this->get_divisor()));
            lines->add(String("delay_ticks=") + String(this->get_delay_ticks()));
            lines->add(String("pause_on=") + String(this->get_pause_during_delay()));
        }

        // ask behaviour to process the key/value pair
        virtual bool load_parse_key_value(String key, String value) override {
            if (key.equals("divisor")) {
                this->set_divisor((int) value.toInt());
                return true;
            } else if (key.equals("delay_ticks")) {
                this->set_delay_ticks((int) value.toInt());
                return true;
            } else if (key.equals("pause_on")) {
                this->set_pause_during_delay((byte)value.toInt());
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