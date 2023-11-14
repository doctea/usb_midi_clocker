#ifndef BEHAVIOUR_SIMPLEWRAPPER__INCLUDED
#define BEHAVIOUR_SIMPLEWRAPPER__INCLUDED

#include "behaviour_clocked.h"
#include "behaviour_base_usb.h"

template<class BaseClass = DividedClockedBehaviour, class DeviceClass = DeviceBehaviourUSBBase>
class Behaviour_SimpleWrapper : public DeviceClass, public BaseClass {
    public:
    char label[32] = "Generic";
    bool can_transmit_midi_notes = false, can_receive_midi_notes = false;

    Behaviour_SimpleWrapper(const char *label, bool can_receive_midi_notes = false, bool can_transmit_midi_notes = false) : BaseClass() {
        strncpy(this->label, label, 32);
        this->can_transmit_midi_notes = can_transmit_midi_notes;
        this->can_receive_midi_notes = can_receive_midi_notes;
    }

    virtual LinkedList<MenuItem*> *make_menu_items() override {
        DeviceClass::make_menu_items();
        return BaseClass::make_menu_items();
    }

    const char *get_label() override {
        return label;
    }

    virtual bool transmits_midi_notes() override {
        return this->can_transmit_midi_notes;
    }
    virtual bool receives_midi_notes() override {
        return this->can_receive_midi_notes;
    }
   
};

template<class BaseClass>
class Behaviour_SimpleWrapperUSB : public Behaviour_SimpleWrapper<BaseClass, DeviceBehaviourUSBBase> {
    public:

    uint16_t vid, pid;

    Behaviour_SimpleWrapperUSB(const char *label, uint16_t vid, uint16_t pid, bool receives_midi_notes = false, bool transmits_midi_notes = false) 
    : Behaviour_SimpleWrapper<BaseClass, DeviceBehaviourUSBBase>(label, receives_midi_notes, transmits_midi_notes) 
    {
        this->pid = pid;
        this->vid = vid;
    }

    virtual uint32_t get_packed_id() override  { return (this->vid<<16 | this->pid); }

};

template<class BaseClass>
class Behaviour_SimpleWrapperSerial : public Behaviour_SimpleWrapper<BaseClass, DeviceBehaviourSerialBase> {
    public:

    /*Behaviour_SimpleWrapperSerial(
        const char *label,
        midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *input_device = nullptr, 
        midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *output_device = nullptr
    ) : Behaviour_SimpleWrapper<BaseClass, DeviceBehaviourSerialBase>(label, input_device!=nullptr, output_device!=nullptr) {
        this->connect_device_input(input_device);
        this->connect_device_output(output_device);
    }    */

    Behaviour_SimpleWrapperSerial(const char *label, bool receives_midi_notes = false, bool transmits_midi_notes = false) 
    : Behaviour_SimpleWrapper<BaseClass, DeviceBehaviourSerialBase>(label, receives_midi_notes, transmits_midi_notes) 
    {}
};

/*
class Behaviour_SimpleDividedClockedWrapper : public BaseClass, public DividedClockedBehaviour {}


template<class BaseClass = DeviceBehaviourUSBBase>
class Behaviour_USBSimpleClockedWrapper : public BaseClass, public ClockedBehaviour {
    using ClockedBehaviour::DeviceBehaviourUltimateBase::parameters;
    using ClockedBehaviour::on_tick;
    using ClockedBehaviour::on_phrase;
    using ClockedBehaviour::on_bar;
    //using ClockedBehaviour::on_restart;
    //using ClockedBehaviour::send_clock;

    bool can_transmit_midi_notes = false, can_receive_midi_notes = false;

    public:
    uint16_t vid, pid;
    char label[32] = "Generic";

    const char *get_label() override {
        return label;
    }

    bool transmits_midi_notes() override {
        return this->can_transmit_midi_notes;
    }

    Behaviour_USBSimpleClockedWrapper(const char *label, uint16_t vid, uint16_t pid, bool receives_midi_notes = false, bool transmits_midi_notes = false) : ClockedBehaviour() {
        this->pid = pid;
        this->vid = vid;
        strncpy(this->label, label, 32);
        this->can_transmit_midi_notes = transmits_midi_notes;
        this->can_receive_midi_notes = receives_midi_notes;
    }

    virtual uint32_t get_packed_id() override  { return (this->vid<<16 | this->pid); }

};

template<class BaseClass = DeviceBehaviourUSBBase>
class Behaviour_USBSimpleDividedClockedWrapper : public BaseClass, public DividedClockedBehaviour {
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
    Behaviour_USBSimpleDividedClockedWrapper(const char *label, uint16_t vid, uint16_t pid) : Behaviour_USBSimpleDividedClockedWrapper(label) {
        this->pid = pid;
        this->vid = vid;
    }
    Behaviour_USBSimpleDividedClockedWrapper(const char *label) : DividedClockedBehaviour() {
        strncpy(this->label, label, 32);
    }

    virtual uint32_t get_packed_id() override  { return (this->vid<<16 | this->pid); }
};
*/

#endif