#ifndef BEHAVIOUR_CLOCKED__INCLUDED
#define BEHAVIOUR_CLOCKED__INCLUDED

#include "Config.h"
#include "behaviour_base.h"

#include "midi/midi_mapper_matrix_manager.h"

//#include "clips/behaviour_clips.h"

class ClockedBehaviour : virtual public DeviceBehaviourUltimateBase {
    public:
        bool restart_on_bar = true;
        bool started = false;
        bool clock_enabled = true;

        bool stop_notes_when_paused = false;

        virtual bool should_show_restart_option() {
            return false;
        }

        virtual void send_clock(uint32_t ticks) override;

        virtual void on_bar(int bar_number) override;
        virtual bool is_set_restart_on_bar();
        virtual void set_restart_on_bar(bool v = true);
        virtual const char *get_restart_on_bar_status_label(bool value);

        virtual void on_restart() override;

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

        FLASHMEM
        virtual ArrangementTrackBase *create_arrangement_track() override;
};

#include "bpm.h"
#include "MIDI.h"

class DividedClockedBehaviour : public ClockedBehaviour { //}, virtual public ArrangementTrackBehaviour {
    //using ArrangementTrackBehaviour::create_arrangement_track;
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
        virtual void send_clock(unsigned long ticks) override;
        
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
            Serial.println("DividedClockedBehaviour::setup_saveable_parameters");
            Serial.printf("saveable_parameters is @%p\n", this->saveable_parameters);
            this->saveable_parameters->add(new SaveableParameter<DividedClockedBehaviour,uint32_t>("divisor", "Clocked", this, &this->clock_divisor, nullptr, nullptr, &DividedClockedBehaviour::set_divisor, &DividedClockedBehaviour::get_divisor));
            this->saveable_parameters->add(new SaveableParameter<DividedClockedBehaviour,int32_t>("delay_ticks", "Clocked", this, &this->clock_delay_ticks, nullptr, nullptr, &DividedClockedBehaviour::set_delay_ticks, &DividedClockedBehaviour::get_delay_ticks));
            this->saveable_parameters->add(new SaveableParameter<DividedClockedBehaviour,bool>("auto_restart_on_change", "Clocked", this, &this->auto_restart_on_change, nullptr, nullptr, &DividedClockedBehaviour::set_auto_restart_on_change, &DividedClockedBehaviour::should_auto_restart_on_change));
            this->saveable_parameters->add(new SaveableParameter<DividedClockedBehaviour,int8_t>("pause_on", "Clocked", this, &this->pause_during_delay, nullptr, nullptr, &DividedClockedBehaviour::set_pause_during_delay, &DividedClockedBehaviour::get_pause_during_delay));
        }

        #ifdef ENABLE_SCREEN
            virtual LinkedList<MenuItem*> *make_menu_items() override;
        #endif
};

#endif