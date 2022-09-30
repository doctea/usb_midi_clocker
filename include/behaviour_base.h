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

    virtual const char *get_label() {
        return (char*)"UltimateBase";
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

    /*
    virtual void save_sequence_add_lines(LinkedList<String> *lines) {
        // todo: save parameter mappings...
    }
    virtual bool parse_sequence_key_value(String key, String value) {
        // todo: reload parameter mappings...
        return false;
    }
    */

    virtual void reset_all_mappings() {
        for (int i = 0 ; i < this->parameters->size() ; i++) {
            this->parameters->get(i)->reset_mappings();
        }
    }

    virtual void save_project_add_lines(LinkedList<String> *lines) {

    }
    virtual bool parse_project_key_value(String key, String value) {
        return false;
    }

    virtual void save_sequence_add_lines(LinkedList<String> *lines) {
        // save all the parameter mapping settings 
        static String prefix = "parameter_" + this->get_label();

        LinkedList<DoubleParameter*> *parameters = this->get_parameters();
        for (int i = 0 ; i < parameters->size () ; i++) {
            DoubleParameter *parameter = parameters->get(i);
            char line[100];
            // todo: move handling of this into Parameter, or into a third class that can handle saving to different formats..?
            // todo: make these mappings part of an extra type of thing rather than associated with sequence?
            // todo: move these to be saved with the project instead?
            for (int slot = 0 ; slot < 3 ; slot++) { // TODO: MAX_CONNECTION_SLOTS...?
                if (parameter->connections[slot].parameter_input==nullptr) continue;      // skip if no parameter_input configured in this slot
                if (parameter->connections[slot].amount==0.00) continue;                     // skip if no amount configured for this slot
                /*sprintf(
                    line, 
                    "%s_%s_%i=%c|%3.3f", 
                    this->get_label(),
                    parameter->label,
                    slot,
                    parameter->get_connection_slot_name(slot),
                    parameter->connections[slot].amount
                );*/
                sprintf(
                    line, 
                    "%s_%s_%i=%3.3f", 
                    prefix,
                    parameter->label,
                    slot,
                    //parameter->get_connection_slot_name(slot),
                    parameter->connections[slot].amount
                );
                lines->add(String(line));
            }
        }
    }
    virtual bool parse_sequence_key_value(String key, String value) {
        // todo: reload parameter mappings...
        Serial.printf("parse_sequence_key_value passed '%s' => '%s'\n", key.c_str(), value.c_str());
        static String prefix = "parameter_" + this->get_label();
        if (key.startsWith(prefix)) {
            key = key.replace(String(this->get_label()) + "_", "");

            String parameter_name = key.substring(0, key.indexOf('_'));
            int slot_number = key.substring(key.indexOf('_')+1).toInt();
            String input_name = value.substring(0, key.indexOf('|'));
            double amount = value.substring(key.indexOf('|')+1).toFloat();

            //this->getParameterForLabel((char*)parameter_name.c_str())->set_slot_input(slot_number, get_input_for_parameter_name(parameter_name)));parameter_name.c_str()[0]);
            DoubleParameter *p = this->getParameterForLabel((char*)parameter_name.c_str());
            if (p!=nullptr)
                p->set_slot_amount(slot_number, amount);
            return true;
        }
        return false;
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

#include "bpm.h"
#include "MIDI.h"

class DividedClockedBehaviour : public ClockedBehaviour {
    public:
        unsigned long clock_delay_ticks = 0; //DEFAULT_DELAY_TICKS;
        int clock_divisor = 1; //DEFAULT_DIVISOR;

        virtual void set_delay_ticks(int delay_ticks) {
            this->clock_delay_ticks = delay_ticks;
        }
        virtual int get_delay_ticks() {
            return this->clock_delay_ticks;
        }
        virtual void set_divisor (int divisor) {
            this->clock_divisor = divisor;
        }
        virtual int get_divisor() {
            return this->clock_divisor;
        }

        int32_t real_ticks = 0;
        virtual void send_clock(unsigned long ticks) override {
            this->real_ticks = ticks;
            if (ticks<clock_delay_ticks) return;

            /*if (is_bpm_on_phrase(real_ticks - clock_delay_ticks)) {
                DeviceBehaviourUSBBase::on_phrase(BPM_CURRENT_PHRASE);
            }*/
            if (is_bpm_on_bar(real_ticks - clock_delay_ticks)) {
                ClockedBehaviour::on_bar(BPM_CURRENT_BAR_OF_PHRASE);
            }

            if (/*ticks==0 || */ticks % clock_divisor == 0)
                ClockedBehaviour::send_clock(ticks - clock_delay_ticks);
        }
        
        virtual void on_bar(int bar_number) override {
            // don't do anything - handle the delayed clocks in send_clock
            //if (is_bpm_on_bar(real_ticks - clock_delay_ticks))
        }
        virtual void on_phrase(uint32_t phrase_number) override {
            // don't do anything - handle the delayed clocks in send_clock
        }

        virtual void on_restart() override {
            Serial.println("\ton_restart() in DividedClockedBehaviour");
            if (this->is_connected() && this->clock_enabled) {
                this->sendRealTime((uint8_t)(midi::Stop)); //sendStop();
                this->sendRealTime((uint8_t)(midi::Start)); //sendStart();
                //this->sendNow();
                this->started = true;
            } else {
                Serial.println("\tin DividedClockedBehaviour on_restart, no device!");
            }
        }

        virtual bool parse_sequence_key_value(String key, String value) override {
            if (ClockedBehaviour::parse_project_key_value(key, value))
                return true;
            if (key.equals("divisor")) {
                this->set_divisor((int) value.toInt());
                returm true;
            } else if (key.equals("delay_ticks")) {
                this->set_delay_ticks(value.toInt());
                return true;
            }
            return false;
        }

        virtual void save_sequence_add_lines(LinkedList<String> *lines) override {
            ClockedBehaviour::save_project_add_lines(lines);
            lines->add("divisor=" + this->clock_divisor);
            lines->add("delay_ticks=" + this->clock_delay_ticks);
        }

};

#endif