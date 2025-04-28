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

        virtual bool transmits_midi_clock() override {
            return isClockEnabled();
        }

        virtual void send_clock(uint32_t ticks) override {
            if (!is_connected()) return;
            if (this->isClockEnabled()) {
                this->sendRealTime(midi::Clock);
                this->sendNow();
            }
        }

        virtual void on_bar(int bar_number) override {
            if (this->restart_on_bar) {
                //Serial.printf(F("%s:\tClockedBehaviour#on_bar and restart_on_bar set!\n"), this->get_label());
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
            //FLASHMEM 
            virtual LinkedList<MenuItem*> *make_menu_items() override;
        #endif
};

#include "bpm.h"
#include "MIDI.h"

class DividedClockedBehaviour : public ClockedBehaviour {
    public:
        int32_t clock_delay_ticks = 0; //DEFAULT_DELAY_TICKS;
        uint32_t clock_divisor = 1; //DEFAULT_DIVISOR;
        uint32_t queued_clock_divisor = 1;
        bool auto_restart_on_change = true;
        int8_t pause_during_delay = false;

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
        virtual void set_delay_ticks(int32_t delay_ticks) {
            if ((int32_t)delay_ticks != this->clock_delay_ticks && this->should_auto_restart_on_change())
                this->set_restart_on_bar(true);
            this->clock_delay_ticks = delay_ticks;
        }
        virtual int32_t get_delay_ticks() {
            return this->clock_delay_ticks;
        }

        // set how many real ticks count for one of our internal ticks -- for use in doing half-time, etc 
        virtual void set_divisor (uint32_t divisor) {
            //Serial.printf("%s#set_divisor(%i)\n", this->get_label(), divisor);
            if (divisor!=this->get_divisor() && this->should_auto_restart_on_change()) {
                this->set_restart_on_bar(true);
            }
            if (is_bpm_on_beat(ticks))
                this->clock_divisor = this->queued_clock_divisor = divisor;
            else {
                //Serial.printf("%s#set_divisor queueing %i\n", this->get_label(), divisor);
                this->queued_clock_divisor = divisor;
            }

        }
        virtual uint32_t get_divisor() {
            return this->clock_divisor;
        }

        // pause during delay
        enum DELAY_PAUSE {
            PAUSE_OFF, PAUSE_BAR, PAUSE_TWO_BAR, PAUSE_PHRASE, PAUSE_FINAL_PHRASE
        };
        virtual void set_pause_during_delay (int8_t value) {
            this->pause_during_delay = value;
        }
        virtual int8_t get_pause_during_delay() {
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
                case PAUSE_FINAL_PHRASE: {
                    //Serial.printf("should_pause_during_bar_number(%i) with PAUSE_FINAL_PHRASE and BARS_PER_PHRASE is %i!\n", bar_number, BARS_PER_PHRASE);
                    bool val = bar_number==0 || (bar_number%BARS_PER_PHRASE)==(BARS_PER_PHRASE-1);
                    //if (val) Serial.printf("matches final bar!\n");
                    return val;
                }
                default: return false;
            }
        }

        // get the longest time that looping should be possible
        int32_t get_period_length() {
            switch(this->pause_during_delay) {
                case PAUSE_BAR: return PPQN*BEATS_PER_BAR;
                case PAUSE_TWO_BAR: return PPQN*BEATS_PER_BAR*2;
                case PAUSE_PHRASE: 
                case PAUSE_OFF: 
                    return PPQN*BEATS_PER_BAR*BARS_PER_PHRASE;
                case PAUSE_FINAL_PHRASE:
                    return PPQN*BEATS_PER_BAR*(BPM_CURRENT_BAR_OF_PHRASE+1); //*BARS_PER_PHRASE; //*(BARS_PER_PHRASE+1);
            }
            return PPQN*BEATS_PER_BAR*BARS_PER_PHRASE;
        }

        int32_t real_ticks = 0;
        bool waiting = true;
        virtual void send_clock(unsigned long ticks) override {
            //if (this->debug) Serial.print("/");
            // if we've been waiting to be on-beat before changing divisor, do so now
            if (is_bpm_on_beat(ticks) && this->queued_clock_divisor!=this->clock_divisor) {
                this->set_divisor(this->queued_clock_divisor);
                this->clock_divisor = queued_clock_divisor;
            }
            //if (this->clock_delay_ticks==PPQN*4) this->debug = true; else this->debug = false;

            int32_t period_length = this->get_period_length();
            uint32_t tick_of_period = (ticks%period_length); //*BARS_PER_PHRASE));

            real_ticks = (int32_t)tick_of_period - clock_delay_ticks;
            if (this->waiting && ((int32_t)tick_of_period) < clock_delay_ticks) { //(real_ticks) < clock_delay_ticks) {
                //if (this->debug) Serial.printf(F("DividedClockBehaviour with global ticks %i, not sending because waiting %i && (tick_of_period %i < clock_delay_ticks of %i\n"), ticks, waiting, tick_of_period, clock_delay_ticks);
                //Serial.printf("%s#sendClock() not sending due to tick %% clock_divisor test failed", this->get_label());
                return;
            }
            if (waiting) {
                //if (this->debug) Serial.printf(F("%s: DividedClockBehaviour with real_ticks %i and clock_delay_ticks %i was waiting\n"), this->get_label(), real_ticks, clock_delay_ticks);
                this->started = true;
                this->sendRealTime((uint8_t)(midi::Stop)); //sendStart();
                this->sendRealTime((uint8_t)(midi::Start)); //sendStart();
                //if (this->debug) Serial.printf(F("%s:\tunsetting waiting!\n"), this->get_label());
                waiting = false;
            }

            if (is_bpm_on_bar(real_ticks)) { //}, clock_delay_ticks)) {
                //if (this->should_auto_restart_on_change())    // force resync restart on every bar, however, no good if target device (eg beatstep) pattern is longer than a bar
                //    this->set_restart_on_bar();
                //if (this->debug) Serial.printf(F("%s: DividedClockBehaviour with real_ticks %i and clock_delay_ticks %i confirmed yes for is_bpm_on_bar, called ClockedBehaviour::on_bar\n"), this->get_label(), real_ticks, clock_delay_ticks);
                ClockedBehaviour::on_bar(BPM_CURRENT_BAR_OF_PHRASE);
                // todo: should we handle calling on_phrase here?  maybe thats where we should force re-sync?
            }

            if (ticks % clock_divisor == 0) {
                //if (this->debug) Serial.print("\\");
                ClockedBehaviour::send_clock(ticks - clock_delay_ticks);
            } else {
                //if (this->debug) Serial.printf("%s#sendClock() not sending due to tick %% clock_divisor test failed", this->get_label());
            }
        }
        
        // actually, we do want to respond to on_bar, because that's when we actually need to start the delay from!
        virtual void on_bar(int bar_number) override {
            // don't do anything - handle the delayed clocks in send_clock
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
            //if (this->debug) Serial.printf(F("%s: on_restart() in DividedClockedBehaviour\n"), this->get_label());
            if (this->is_connected() && this->clock_enabled) {
                this->sendRealTime((uint8_t)(midi::Stop)); //sendStop();
                
                if (this->get_delay_ticks()>0) {
                    this->waiting = true;
                    this->started = false;
                } else {
                    this->sendRealTime((uint8_t)(midi::Start));
                    this->started = true;
                }
                //if (this->debug) Serial.printf(F("%s:\tsetting waiting!\n"), this->get_label());
            } else {
                //if (this->debug) Serial.println(F("\tin DividedClockedBehaviour on_restart, no device!"));
            }
        }

        virtual void setup_saveable_parameters() {
            if (this->saveable_parameters==nullptr)
                DeviceBehaviourUltimateBase::setup_saveable_parameters();
            Serial_println("DividedClockedBehaviour::setup_saveable_parameters");
            Serial_printf("saveable_parameters is @%p\n", this->saveable_parameters);
            this->saveable_parameters->add(new LSaveableParameter<uint32_t>("divisor", "Clocked", &this->clock_divisor, [=](uint32_t v) -> void { this->set_divisor(v); }, [=]() -> uint32_t { return this->get_divisor(); }));
            this->saveable_parameters->add(new LSaveableParameter<int32_t>("delay_ticks", "Clocked", &this->clock_delay_ticks, [=](int32_t v) -> void { this->set_delay_ticks(v); }, [=]() -> int32_t { return this->get_delay_ticks(); }));
            this->saveable_parameters->add(new LSaveableParameter<bool>("auto_restart_on_change","Clocked", &this->auto_restart_on_change, [=](bool v) -> void { this->set_auto_restart_on_change(v); }, [=]() -> bool { return this->should_auto_restart_on_change(); }));
            this->saveable_parameters->add(new LSaveableParameter<int8_t>("pause_on", "Clocked", &this->pause_during_delay, [=](int8_t v) -> void { this->set_pause_during_delay(v); }, [=]() -> int8_t { return this->get_pause_during_delay(); } ));
        }

        #ifdef ENABLE_SCREEN
            virtual LinkedList<MenuItem*> *make_menu_items() override;
        #endif
};

#endif