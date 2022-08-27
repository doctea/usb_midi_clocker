#ifndef BEHAVIOUR_BASE__INCLUDED
#define BEHAVIOUR_BASE__INCLUDED

#include <Arduino.h>

#include <MIDI.h>
#include <USBHost_t36.h>

#include <LinkedList.h>

#include "midi_mapper_matrix_types.h"

using namespace midi;

class DeviceBehaviourUltimateBase {
    public:

    bool debug = false;

    source_id_t source_id = -1;
    target_id_t target_id = -1;

    DeviceBehaviourUltimateBase() = default;
    virtual ~DeviceBehaviourUltimateBase() = default;

    virtual void setup_callbacks() {};

    virtual bool is_connected() {
        return false;
    }

    virtual void read() {};
    // called every loop
    virtual void loop(uint32_t ticks) {};
    // on_pre_clock called before the clocks are sent
    virtual void on_pre_clock(uint32_t ticks) {};
    // called during tick when behaviour_manager send_clocks is called
    virtual void send_clock(uint32_t ticks) {};
    // called every tick, after clocks sent
    virtual void on_tick(uint32_t ticks) {};
    // called when new bar starts
    virtual void on_bar(int bar_number) {}
    // called when the clock is restarted
    virtual void on_restart() {};
    // called when we change phrase
    virtual void on_phrase(uint32_t phrase) {};
    virtual void note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
    // called when a note_off message is received from the device
    virtual void note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
    virtual void control_change (uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
    virtual void init() {};

    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
        Serial.println("DeviceBehaviourUltimateBase#sendNoteOn");
    };
    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
        Serial.println("DeviceBehaviourUltimateBase#sendNoteOff");
    };
    virtual void sendControlChange(uint8_t number, uint8_t value, uint8_t channel) {
        Serial.println("DeviceBehaviourUltimateBase#sendControlChange");
    };
    virtual void sendRealTime(uint8_t message) {
        Serial.println("DeviceBehaviourUltimateBase#sendRealTime");
    };    
    virtual void sendNow() {

    };

};

class DeviceBehaviourUSBBase : virtual public DeviceBehaviourUltimateBase {
    public:

        const uint32_t vid = 0x0000, pid = 0x0000;
        MIDIDeviceBase *device = nullptr;

        DeviceBehaviourUSBBase() = default;
        virtual ~DeviceBehaviourUSBBase() = default;

        virtual uint32_t get_packed_id() { return (this->vid<<16 | this->pid); }

        // interface methods - static
        virtual bool matches_identifiers(uint16_t vid, uint16_t pid) {
            return vid==this->vid && pid==this->pid;
        }
        virtual bool matches_identifiers(uint32_t packed_id) {
            return (this->get_packed_id()==packed_id);
        }

        virtual bool is_connected() override {
            return this->device!=nullptr;
        }

        virtual void connect_device(MIDIDeviceBase *device) {
            //if (!is_connected()) return;

            Serial.printf("DeviceBehaviourUSBBase#connected_device connecting %p\n", device);
            this->device = device;
            this->setup_callbacks();
            this->init();
        }
        // remove handlers that might already be set on this port -- new ones assigned below thru setup_callbacks functions
        virtual void disconnect_device() {
            //if (this->device==nullptr) return;
            if (!is_connected()) return;
            
            this->device->setHandleNoteOn(nullptr);
            this->device->setHandleNoteOff(nullptr);
            this->device->setHandleControlChange(nullptr);
            this->device->setHandleClock(nullptr);
            this->device->setHandleStart(nullptr);
            this->device->setHandleStop(nullptr);
            this->device->setHandleSystemExclusive((void (*)(uint8_t *, unsigned int))nullptr);
            this->device = nullptr;
        }

        virtual void read() override {
            if (!is_connected()) return;

            while(this->device->read()); 
        };

        virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
            if (!is_connected()) return;
            this->device->sendNoteOn(note, velocity, channel);
        };
        virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
            if (!is_connected()) return;
            this->device->sendNoteOff(note, velocity, channel);
        };
        virtual void sendControlChange(uint8_t number, uint8_t value, uint8_t channel) override {
            if (!is_connected()) return;
            this->device->sendControlChange(number, value, channel);
        };
        virtual void sendRealTime(uint8_t message) override {
            if (!is_connected()) return;
            this->device->sendRealTime(message);
        };
};

class ClockedBehaviour : virtual public DeviceBehaviourUltimateBase {

    public:
        bool restart_on_bar = true;
        bool started = false;
        bool clock_enabled = true;

        virtual void send_clock(uint32_t ticks) override {
            if (!is_connected()) return;
            this->sendRealTime(midi::Clock);
            this->sendNow();
        }

        virtual void on_bar(int bar_number) override {
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
};

#endif