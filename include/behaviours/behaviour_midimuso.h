#include "behaviour_base_serial.h"

#include "Config.h"

#ifdef ENABLE_DPT_LOOPER

class DeviceBehaviour_MIDIMuso : public DeviceBehaviourSerialBase {
    public:
        virtual const char *get_label() override {
            return (const char*)"MIDIMuso";
        }

        void send_mode_change(byte mode_number) {
            //this->output_device->sendProgramChange(99, 1);
            //this->output_device->sendProgramChange(mode_number, 1);
            //this->output_device->getTrsend(0xc1, mode_number, 0, 0);
            this->output_device->getTransport()->write(0xc0);
            this->output_device->getTransport()->write(99);

            this->output_device->getTransport()->write(0xc0);
            this->output_device->getTransport()->write(mode_number);

            this->output_device->getTransport()->write(0xc1);
            this->output_device->getTransport()->write(99);

            this->output_device->getTransport()->write(0xc1);
            this->output_device->getTransport()->write(mode_number);

            char message[40];
            snprintf(message, 40, "Send mode change %i", mode_number);
            messages_log_add(message);
        }

        void set_mode_0b() {
            this->send_mode_change(8);
        }
        void set_mode_2b() {
            this->send_mode_change(3);
        }

        void on_tick(uint32_t tick) override {
            static int count = 0;

            int beat = tick / 6;

            if (beat % 6 != 0) 
                return;

            this->sendNoteOn(60, 127, 16);
            this->sendNoteOn(35 + (count % 40), 127, 16);
            count++;
        }

        #ifdef ENABLE_SCREEN
            LinkedList<MenuItem*> *make_menu_items();
        #endif

};

extern DeviceBehaviour_MIDIMuso *behaviour_midimuso;

#endif