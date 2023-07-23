#ifndef BEHAVIOUR_SIMPLEWRAPPER__INCLUDED
#define BEHAVIOUR_SIMPLEWRAPPER__INCLUDED

#include "behaviour_clocked.h"
#include "behaviour_base_usb.h"

class Behaviour_USBSimpleClockedWrapper : public DeviceBehaviourUSBBase, public ClockedBehaviour {
    using ClockedBehaviour::DeviceBehaviourUltimateBase::parameters;
    using ClockedBehaviour::on_tick;
    using ClockedBehaviour::on_phrase;
    using ClockedBehaviour::on_bar;
    //using ClockedBehaviour::on_restart;
    //using ClockedBehaviour::send_clock;

    public:
    uint16_t vid, pid;
    char label[32] = "Generic";

    const char *get_label() override {
        return label;
    }

    bool has_output() override {
        return true;
    }

    Behaviour_USBSimpleClockedWrapper(const char *label, uint16_t vid, uint16_t pid) : ClockedBehaviour() {
        this->pid = pid;
        this->vid = vid;
        strncpy(this->label, label, 32);
    }

    virtual uint32_t get_packed_id() override  { return (this->vid<<16 | this->pid); }

    /*virtual void on_tick(uint32_t ticks) override {
        Serial.println("behringer edge tick!");
        ClockedBehaviour::on_tick(ticks);
    }*/
    /*virtual void send_clock(uint32_t ticks) override {
        Serial.println("behringer edge send_clock!");
        if (!is_connected())
            Serial.println("\tisn't connected?");
        ClockedBehaviour::send_clock(ticks);
    }*/

    virtual void on_restart() override {
        Serial.println("behringer edge on_restart!");

        /*if (is_connected()) {
            DeviceBehaviourUSBBase::device->sendSongPosition(0);
        } else {
            Serial.println("\tisn't connected?");
        }*/
        //virtual void on_restart() override {
            //if (!is_connected()) return;

        if (this->clock_enabled) {
            this->sendRealTime(midi::Stop);
            if (is_connected())
                DeviceBehaviourUSBBase::device->sendSongPosition(0);
            this->sendRealTime(midi::Start);
            //this->sendRealTime(midi::Continue);
            //this->sendNow();
            //this->started = true;
        }
        //}
    }
};

#endif