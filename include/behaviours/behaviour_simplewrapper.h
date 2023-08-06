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

};

class Behaviour_USBSimpleDividedClockedWrapper : public DeviceBehaviourUSBBase, public DividedClockedBehaviour {
    using DividedClockedBehaviour::DeviceBehaviourUltimateBase::parameters;
    using DividedClockedBehaviour::on_tick;
    using DividedClockedBehaviour::on_phrase;
    using DividedClockedBehaviour::on_bar;
    //using ClockedBehaviour::on_restart;
    //using ClockedBehaviour::send_clock;

    public:
    uint16_t vid, pid;
    char label[32] = "Generic";

    const char *get_label() override {
        return label;
    }

    /*bool has_output() override {
        return true;
    }*/

    Behaviour_USBSimpleDividedClockedWrapper(const char *label, uint16_t vid, uint16_t pid) : DividedClockedBehaviour() {
        this->pid = pid;
        this->vid = vid;
        strncpy(this->label, label, 32);
    }

    virtual uint32_t get_packed_id() override  { return (this->vid<<16 | this->pid); }

};


#endif