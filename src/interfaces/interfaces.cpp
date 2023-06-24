#include "interfaces/interfaces.h"

#include "cv_outs.h"
#include "sequencer.h"

GateManager *gate_manager = new GateManager();

#ifndef ENABLE_MCP23S17
    void setup_gate_manager() {
        gate_manager->add_bank_interface(BANK_CLOCK,    new DigitalPinBankInterface(cv_out_clock_pin,   NUM_CLOCKS));
        gate_manager->add_bank_interface(BANK_SEQ,      new DigitalPinBankInterface(cv_out_sequence_pin,     4));
    }
#else
    #include "interfaces/mcp23s17_interface.h"

    void setup_gate_manager() {
        MCP23S17BankInterface *mcp_interface = new MCP23S17BankInterface();
        gate_manager->add_bank_interface(BANK_CLOCK, new VirtualBankInterface(mcp_interface, 0, 8));
        gate_manager->add_bank_interface(BANK_SEQ,   new VirtualBankInterface(mcp_interface, 8, 8));
    }
#endif


void set_clock_gate(int gate_number, bool state) {
    gate_manager->send_gate(BANK_CLOCK, gate_number, state);
}
void set_sequence_gate(int gate_number, bool state) {
    gate_manager->send_gate(BANK_SEQ, gate_number, state);
}
