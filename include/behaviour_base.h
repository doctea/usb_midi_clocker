#ifndef BEHAVIOUR_BASE__INCLUDED
#define BEHAVIOUR_BASE__INCLUDED

#include <Arduino.h>

//#include <MIDI.hpp>
#include <USBHost_t36.h>

#include <LinkedList.h>

class DeviceBehaviourBase {
    public:
        bool debug = false;

        const uint32_t vid = 0x0000, pid = 0x0000;
        MIDIDeviceBase *device = nullptr;

        DeviceBehaviourBase() = default;
        virtual ~DeviceBehaviourBase() = default;

        virtual uint32_t get_packed_id() { return (this->vid<<16 | this->pid); }

        // interface methods - static
        virtual bool matches_identifiers(uint16_t vid, uint16_t pid) {
            return vid==this->vid && pid==this->pid;
        }
        virtual bool matches_identifiers(uint32_t packed_id) {
            return (this->get_packed_id()==packed_id);
        }

        virtual void setup_callbacks() {};
        virtual void connect_device(MIDIDeviceBase *device) {
            if (device==nullptr) return;

            Serial.printf("DeviceBehaviourBase#connected_device connecting %p\n", device);
            this->device = device;
            this->setup_callbacks();
            this->init();
        }
        // remove handlers that might already be set on this port -- new ones assigned below thru setup_callbacks functions
        virtual void disconnect_device() {
            if (this->device==nullptr) return;
            
            this->device->setHandleNoteOn(nullptr);
            this->device->setHandleNoteOff(nullptr);
            this->device->setHandleControlChange(nullptr);
            this->device->setHandleClock(nullptr);
            this->device->setHandleStart(nullptr);
            this->device->setHandleStop(nullptr);
            this->device->setHandleSystemExclusive((void (*)(uint8_t *, unsigned int))nullptr);
            this->device = nullptr;
        }

        /*// interface methods for instantiated devices
        virtual bool init(MIDIDevice *device) { 
            this->device = device;
            // add hooks to device
        };
        virtual bool deinit() {
            this->device = nullptr;
            // remove all hooks from device
        }*/
        virtual void read() {
            if (this->device==nullptr) return;

            while(this->device->read()); 
        };
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
        // called when a note_on message is received from the device
        virtual void note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {};
        // called when a note_off message is received from the device
        virtual void note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {};
        // called when a control_change message is received from the device
        virtual void control_change (uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {};
        virtual void init() {};

};

class ClockedBehaviour : public DeviceBehaviourBase {
    public:
        bool restart_on_bar = true;
        bool started = false;
        bool clock_enabled = true;

        virtual void send_clock(uint32_t ticks) override {
            if (this->device == nullptr) return;
            //Serial.println("send_clock() in ClockedBehaviour");
            if (this->clock_enabled) {
                this->device->sendRealTime(MIDIDevice::Clock);
                this->device->send_now();
            } else {
                //Serial.println("in clocked behaviour send_clock, no device!");
            }
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
            if (this->device == nullptr) return;
            //Serial.println("\ton_restart() in ClockedBehaviour");
            if (this->clock_enabled) {
                this->device->sendRealTime(MIDIDevice::Stop); //sendStop();
                this->device->sendRealTime(MIDIDevice::Start); //sendStart();
                this->device->send_now();
                this->started = true;
            } else {
                //Serial.println("\tin clocked behaviour on_restart, no device!");
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