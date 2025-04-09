// Gate output interfaces

#pragma once

#include <Arduino.h>

#include <LinkedList.h>

#include "debug.h"

#include "midi_helpers.h"

enum GATEBANKS {
    BANK_CLOCK,
    BANK_SEQ,
    #if defined(ENABLE_GATES_BANK_EXTRA)
        BANK_EXTRA_1,
        BANK_EXTRA_2,
    #endif
    NUM_GATE_BANKS
};

class BankInterface {
    public:

    uint_least8_t num_gates = 8;
    bool dirty = true;
    const char *label = "gates";

    virtual const char *get_label() {
        return label;
    }

    virtual void set_gate(int gate, bool state) = 0;
    virtual void update() = 0;
    virtual bool check_gate(int gate_number) = 0;
    virtual void stop_all_gates() {
        for (uint_least8_t i = 0 ; i < num_gates ; i++) {
            this->set_gate(i, false);
        }
    }
};

class VirtualRemapBankInterface : public BankInterface {
    public:
        BankInterface *underlying = nullptr;
        int *remap_pins = nullptr;
        int pin_count = 0;

        VirtualRemapBankInterface(const char *label, BankInterface *iface, int *remap_pins, int pin_count) {
            this->label = label;
            this->pin_count = pin_count;
            this->remap_pins = (int*)CALLOC_FUNC(pin_count, sizeof(int));
            for (uint_least8_t i = 0 ; i < pin_count ; i++) {
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
        virtual void update() override {
            underlying->update();
        }
};

// use one underlying BankInterface partitioned into two (or more)
class VirtualBankInterface : public BankInterface {
    public:
        BankInterface *underlying = nullptr;
        int_least8_t gate_offset = 0;
        bool reverse = false;   // reverse the order of the gate-output mapping

        VirtualBankInterface(const char *label, BankInterface *iface, int gate_offset, int num_gates, bool reverse = false) {
            this->label = label;
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
        virtual void update() override {
            underlying->update();
        }
};

// standard Arduino GPIO pinMode/digitalWrite
// note INPUT mode is completely untested!
class DigitalPinBankInterface : public BankInterface {
    public:
        uint_least8_t num_gates = 8;
        bool *current_states = nullptr;
        byte mode = OUTPUT;

        uint8_t *pin_numbers = nullptr;
        DigitalPinBankInterface(const char *label, const byte *pin_numbers, int num_pins, byte mode = OUTPUT) {
            this->label = label;
            this->pin_numbers = (uint8_t*)CALLOC_FUNC(num_pins, sizeof(uint8_t));
            this->mode = mode;
            num_gates = num_pins;
            //memcpy(this->pin_numbers, pin_numbers, sizeof(uint8_t)*num_pins);
            for (uint_least8_t i = 0 ; i < num_pins ; i++) {
                this->pin_numbers[i] = pin_numbers[i];
                pinMode(pin_numbers[i], mode);
            }
            this->current_states = (bool*)CALLOC_FUNC(num_gates, sizeof(bool));
        }

        virtual void set_gate(int gate_number, bool state) override {
            //this->dirty = true;
            if (gate_number < 0 || gate_number >= num_gates) {
                //messages_log_add(String("Attempted to send to invalid gate %i:%i") + String(bank) + String(": ") + String(gate));
                return;            
            }
            if (mode!=OUTPUT)
                return;
            digitalWrite(pin_numbers[gate_number%num_gates], state);
            this->current_states[gate_number] = state;
        }
        virtual bool check_gate(int gate_number) override {
            if (gate_number < 0 || gate_number >= num_gates)
                return false;

            if (mode==OUTPUT)
                return this->current_states[gate_number%num_gates];
            else
                return digitalRead(gate_number);
        }

        virtual void update() override {
            //dirty = false;
        };
};

#ifdef ENABLE_SCREEN
    class Menu;
    class MenuItem;
#endif

class GateManager : virtual public IGateTarget {
    public:
    uint_least8_t num_banks = 0;

    // TODO: handle more than 2 banks... currently expects two, BANK_CLOCK and BANK_SEQ
    BankInterface *banks[NUM_GATE_BANKS] = { nullptr, nullptr };

    GateManager() {
    }

    void add_bank_interface(int bank, BankInterface *iface) {
        //banks[bank].add(iface);
        if (num_banks>=NUM_GATE_BANKS) {
            Serial.printf("ERROR: attempted to add too many banks (%i of %i)!", bank+1, NUM_GATE_BANKS);
            messages_log_add(String("ERROR: attempted to add too many banks!"));
        }

        banks[bank] = iface;
        iface->stop_all_gates();
        num_banks++;    // TODO: handle more than 2 banks...!!
    }

    /*void send_gate_on(int8_t bank, int8_t gate) override {
        this->send_gate(bank, gate, true);
    }
    void send_gate_off(int8_t bank, int8_t gate) override {
        this->send_gate(bank, gate, false);
    }*/
    virtual void send_gate(int8_t bank, int8_t gate, bool state) {
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
        for (uint_fast8_t bank = 0 ; bank < num_banks ; bank++) {
            this->banks[bank]->update();
        }
    }

    // panic turn off all gates
    void stop_all_gates() {
        //Serial.println("stop all gates!");
        for (uint_fast8_t b = 0 ; b < num_banks ; b++) {
            this->banks[b]->stop_all_gates();
        }
    }

    #ifdef ENABLE_SCREEN
        FLASHMEM
        void create_controls(Menu *menu);
    #endif
};

extern GateManager *gate_manager;

void setup_gate_manager();
#ifdef ENABLE_SCREEN
    void setup_gate_manager_menus();
#endif


// convenience function for setting clock gate states
void set_clock_gate(int gate_number, bool state);

// convenience function for setting sequencer gate states
void set_sequence_gate(int gate_number, bool state);
