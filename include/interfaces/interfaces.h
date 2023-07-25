#ifndef GATE_MANAGER__INCLUDED
#define GATE_MANAGER__INCLUDED

#include <Arduino.h>

#include <LinkedList.h>

#include "debug.h"

class BankInterface {
    public:

    int num_gates = 8;
    bool dirty = true;

    virtual void set_gate(int gate, bool state) = 0;
    virtual void update() = 0;
    virtual bool check_gate(int gate_number) = 0;
    virtual void stop_all_gates() {
        for (int i = 0 ; i < num_gates ; i++) {
            this->set_gate(i, false);
        }
    }
};

class VirtualRemapBankInterface : public BankInterface {
    public:
        BankInterface *underlying = nullptr;
        int *remap_pins = nullptr;
        int pin_count = 0;

        VirtualRemapBankInterface(BankInterface *iface, int *remap_pins, int pin_count) {
            this->pin_count = pin_count;
            this->remap_pins = (int*)calloc(pin_count, sizeof(int));
            for (int i = 0 ; i < pin_count ; i++) {
                this->remap_pins[i] = remap_pins[i];
            }
            this->underlying = iface;
        }

        virtual void set_gate(int gate_number, bool state) override {
            this->underlying->set_gate(this->remap_pins[gate_number], state);
        }
        virtual bool check_gate(int gate_number) override {
            return this->underlying->check_gate(this->remap_pins[gate_number]);
        }
        virtual void update() override {}
};

// use one underlying BankInterface partitioned into two (or more)
class VirtualBankInterface : public BankInterface {
    public:
        BankInterface *underlying = nullptr;
        int gate_offset = 0;
        bool reverse = false;   // reverse the order of the gate-output mapping

        VirtualBankInterface(BankInterface *iface, int gate_offset, int num_gates, bool reverse = false) {
            this->underlying = iface;
            this->gate_offset = gate_offset;
            this->num_gates = num_gates;
            this->reverse = reverse;
        }

        // todo: untested
        virtual void set_gate(int gate_number, bool state) override {
            //this->dirty = true;
            if (gate_number >= num_gates) {
                //messages_log_add(String("Attempted to send to invalid gate %i:%i") + String(bank) + String(": ") + String(gate));
                return;            
            }
            if (reverse)
                gate_number = (num_gates-1) - gate_number;
            //digitalWrite(pin_numbers[gate_number], state);
            underlying->set_gate(gate_number+gate_offset, state);
        }
        virtual bool check_gate(int gate_number) override {
            if (gate_number >= num_gates) {
                //messages_log_add(String("Attempted to send to invalid gate %i:%i") + String(bank) + String(": ") + String(gate));
                return false;
            }   
            if (reverse)
                gate_number = (num_gates-1) - gate_number;
            //digitalWrite(pin_numbers[gate_number], state);
            return underlying->check_gate(gate_number+gate_offset);
        }
        virtual void update() override {}
};

// standard Arduino pinMode/digitalWrite
class DigitalPinBankInterface : public BankInterface {
    public:
        int num_gates = 8;

        bool *current_states = nullptr;

        //uint8_t raw_pin_numbers[num_gates];
        //int raw_pin_number = -1;
        /*DigitalPinBankInterface(int raw_pin_number) {
            this->raw_pin_number = raw_pin_number;
            pinMode(raw_pin_number, OUTPUT);
        }*/
        uint8_t *pin_numbers = nullptr;
        DigitalPinBankInterface(const byte *pin_numbers, int num_pins) {
            this->pin_numbers = (uint8_t*)calloc(num_pins, sizeof(uint8_t));
            num_gates = num_pins;
            //memcpy(this->pin_numbers, pin_numbers, sizeof(uint8_t)*num_pins);
            for (int i = 0 ; i < num_pins ; i++) {
                this->pin_numbers[i] = pin_numbers[i];
                pinMode(pin_numbers[i], OUTPUT);
            }
            this->current_states = (bool*)calloc(num_gates, sizeof(bool));
        }

        virtual void set_gate(int gate_number, bool state) override {
            //this->dirty = true;
            if (gate_number >= num_gates) {
                //messages_log_add(String("Attempted to send to invalid gate %i:%i") + String(bank) + String(": ") + String(gate));
                return;            
            }   
            digitalWrite(pin_numbers[gate_number%num_gates], state);
            this->current_states[gate_number] = state;
        }
        virtual bool check_gate(int gate_number) override {
            return this->current_states[gate_number%num_gates];
        }

        virtual void update() override {
            //dirty = false;
        };
};

class Menu;
class MenuItem;

class GateManager {
    public:
    int num_banks = 0;

    // TODO: handle more than 2 banks... currently expects two, BANK_CLOCK and BANK_SEQ
    BankInterface *banks[2] = { nullptr, nullptr };

    GateManager() {
    }

    void add_bank_interface(int bank, BankInterface *iface) {
        //banks[bank].add(iface);
        if (num_banks>=2)
            messages_log_add(String("ERROR: attempted to add too many banks!"));

        banks[bank] = iface;
        iface->stop_all_gates();
        num_banks++;    // TODO: handle more than 2 banks...!!
    }

    void send_gate_on(int bank, int gate) {
        this->send_gate(bank, gate, true);
    }
    void send_gate_off(int bank, int gate) {
        this->send_gate(bank, gate, false);
    }
    void send_gate(int bank, int gate, bool state) {
        if (bank>=num_banks) {
            messages_log_add(String("Attempted to send to invalid bank ") + String(bank) + String(" : ") + String(gate));            
            return;            
        }
        /*if (gate>=this->banks[bank].size()) {
            messages_log_add(String("Attempted to send to invalid gate %i:%i") + String(bank) + String(": ") + String(gate));
            return;            
        }*/

        this->banks[bank]->set_gate(gate, state);
    }

    bool check_gate(int bank, int gate) {
        return this->banks[bank]->check_gate(gate);
    }

    void update() {
        for (int bank = 0 ; bank < num_banks ; bank++) {
            this->banks[bank]->update();
        }
    }

    // panic turn off all gates
    void stop_all_gates() {
        Serial.println("stop all gates!");
        for (int b = 0 ; b < num_banks ; b++) {
            this->banks[b]->stop_all_gates();
        }
    }

    #ifdef ENABLE_SCREEN
        void *create_controls(Menu *menu);
    #endif
};

extern GateManager *gate_manager;

void setup_gate_manager();
#ifdef ENABLE_SCREEN
    void setup_gate_manager_menus();
#endif

#define BANK_CLOCK  0
#define BANK_SEQ    1

// convenience function for setting clock gate states
void set_clock_gate(int gate_number, bool state);

// convenience function for setting sequencer gate states
void set_sequence_gate(int gate_number, bool state);

#endif