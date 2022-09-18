#ifndef BEHAVIOUR_BASE__INCLUDED
#define BEHAVIOUR_BASE__INCLUDED

#include <Arduino.h>

#include <MIDI.h>
#include <USBHost_t36.h>

#include <LinkedList.h>

#include "midi_mapper_matrix_types.h"

#include "parameters/Parameter.h"

using namespace midi;

enum BehaviourType {
    undefined,
    usb,
    serial
};

class DeviceBehaviourUltimateBase {
    public:

    bool debug = false;

    source_id_t source_id = -1;
    target_id_t target_id = -1;

    //MIDIOutputWrapper *wrapper = nullptr;

    DeviceBehaviourUltimateBase() = default;
    virtual ~DeviceBehaviourUltimateBase() = default;

    virtual char *get_label() {
        return "UltimateBase";
    }

    virtual int getType() {
        return BehaviourType::undefined;
    }

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
    virtual void on_bar(int bar_number) {};
    virtual void on_end_bar(int bar_number) {};
    // called when the clock is restarted
    virtual void on_restart() {};
    // called when we change phrase
    virtual void on_phrase(uint32_t phrase) {};
    virtual void receive_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
    // called when a note_off message is received from the device
    virtual void receive_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
    virtual void receive_control_change (uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
    virtual void init() {};

    // tell the device to play a note on
    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
        Serial.println("DeviceBehaviourUltimateBase#sendNoteOn");
    };
    // tell the device to play a note off
    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
        Serial.println("DeviceBehaviourUltimateBase#sendNoteOff");
    };
    // tell the device to send a control change
    virtual void sendControlChange(uint8_t number, uint8_t value, uint8_t channel) {
        Serial.println("DeviceBehaviourUltimateBase#sendControlChange");
    };
    // tell the device to send a realtime message
    virtual void sendRealTime(uint8_t message) {
        Serial.println("DeviceBehaviourUltimateBase#sendRealTime");
    };    
    virtual void sendNow() {

    };

    virtual void save_sequence_add_lines(LinkedList<String> *lines) {
        // todo: save parameter mappings...
    }
    virtual bool parse_sequence_key_value(String key, String value) {
        // todo: reload parameter mappings...
        return false;
    }

    // parameter handling shit
    LinkedList<DoubleParameter*> *parameters = new LinkedList<DoubleParameter*>();
    virtual LinkedList<DoubleParameter*> *get_parameters () {
        if (parameters->size()==0)
            this->initialise_parameters();
        return parameters;
    }
    virtual LinkedList<DoubleParameter*> *initialise_parameters() {
        /*if (parameters==nullptr)
            parameters = new LinkedList<DoubleParameter*>();*/
        return parameters;
    }
    virtual DoubleParameter* getParameterForLabel(char *label) {
        Serial.printf("getParameterForLabel(%s) in behaviour %s..\n", label, this->get_label());

        for (int i = 0 ; i < parameters->size() ; i++) {
            Serial.printf("Comparing '%s' to '%s'\n", parameters->get(i)->label, label);
            if (strcmp(parameters->get(i)->label, label)==0) 
                return parameters->get(i);
        }
        Serial.printf("WARNING/ERROR in behaviour %s: didn't find a Parameter labelled %s\n", this->get_label(), label);
        return nullptr;
    }
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
            //Serial.printf("ClockedBehaviour#on_bar in %p\n", this);
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