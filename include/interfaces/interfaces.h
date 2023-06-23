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
};

class VirtualBankInterface : public BankInterface {
    public:
        BankInterface *underlying = nullptr;
        int gate_offset = 0;

        VirtualBankInterface(BankInterface *iface, int gate_offset, int num_gates) {
            this->underlying = iface;
            this->gate_offset = gate_offset;
            this->num_gates = num_gates;
        }

        virtual void set_gate(int gate_number, bool state) override {
            //this->dirty = true;
            if (gate_number >= num_gates) {
                //messages_log_add(String("Attempted to send to invalid gate %i:%i") + String(bank) + String(": ") + String(gate));
                return;            
            }   
            //digitalWrite(pin_numbers[gate_number], state);
            underlying->set_gate(gate_number+gate_offset, state);
        }
        virtual void update() override {}
};

class DigitalPinBankInterface : public BankInterface {
    public:
        int num_gates = 8;
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
        }

        virtual void set_gate(int gate_number, bool state) override {
            //this->dirty = true;
            if (gate_number >= num_gates) {
                //messages_log_add(String("Attempted to send to invalid gate %i:%i") + String(bank) + String(": ") + String(gate));
                return;            
            }   
            digitalWrite(pin_numbers[gate_number], state);
        }

        virtual void update() override {
            //dirty = false;
        };
};

class GateManager {
    public:
    int num_banks = 0;

    // TODO: handle more than 2 banks...
    BankInterface *banks[2] = { nullptr, nullptr };

    GateManager() {
    }

    void add_bank_interface(int bank, BankInterface *iface) {
        //banks[bank].add(iface);
        if (num_banks>=2)
            messages_log_add(String("ERROR: attempted to add too many banks!"));

        banks[bank] = iface;
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

    void update() {
        for (int bank = 0 ; bank < num_banks ; bank++) {
            this->banks[bank]->update();
        }
    }
};

extern GateManager *gate_manager;

void setup_gate_manager();

#define BANK_CLOCK 0
#define BANK_SEQ 1

// convenience function for setting clock gate states
void set_clock_gate(int gate_number, bool state);

// convenience function for setting sequencer gate states
void set_sequence_gate(int gate_number, bool state);

#endif