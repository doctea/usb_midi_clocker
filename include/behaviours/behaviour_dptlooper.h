#include "behaviour_base_serial.h"

#include "Config.h"

#ifdef ENABLE_DPT_LOOPER

class DeviceBehaviour_DPTLooper : public DeviceBehaviourSerialBase {
    enum loop_mode_t {
        ON_BAR,
        ON_2BAR,
        ON_PHRASE,
        ON_2PHRASE
    };

    public:
        int loop_type = loop_mode_t::ON_BAR;

        virtual const char *get_label() override {
            return (const char*)"DPT Looper";
        }

        virtual void on_bar(int bar_number) override {
            if (loop_type==loop_mode_t::ON_BAR)
                this->sendRealTime(midi::Start);
            else if (loop_type==loop_mode_t::ON_2BAR && bar_number % 2 ==0) {
                this->sendRealTime(midi::Start);
            }
        }
        virtual void on_phrase(uint32_t phrase_number) override {
            if (loop_type==loop_mode_t::ON_PHRASE)
                this->sendRealTime(midi::Start);
            else if (loop_type==loop_mode_t::ON_2PHRASE && phrase_number % 2 ==0) {
                this->sendRealTime(midi::Start);
            }
        }
        /*virtual void on_tick (uint32_t tick) override {
            send_clock(tick);
        }*/

        virtual void send_clock(uint32_t ticks) override {
            if (!is_connected()) return;
            this->sendRealTime(midi::Clock);
            this->sendNow();
        }
        
        void setLoopType(int loop_type) {
            this->loop_type = loop_type;
        }
        int getLoopType() {
            return this->loop_type;
        }

        virtual void on_restart() override {
            this->sendRealTime(midi::Start);
        }

        void start_dubbing() {
            this->sendRealTime(midi::Continue);
        }
        void stop_dubbing() {
            this->sendRealTime(midi::Stop);
        }

        #ifdef ENABLE_SCREEN
            LinkedList<MenuItem*> *make_menu_items() override;
        #endif

};

extern DeviceBehaviour_DPTLooper *behaviour_dptlooper;

#endif