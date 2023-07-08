#include "interfaces/interfaces.h"

#include "cv_outs.h"
#include "sequencer.h"

#include "SPI.h"

GateManager *gate_manager = new GateManager();

#ifdef ENABLE_GATES_GPIO
    void setup_gate_manager() {
        gate_manager->add_bank_interface(BANK_CLOCK,    new DigitalPinBankInterface(cv_out_clock_pin,       NUM_CLOCKS));
        gate_manager->add_bank_interface(BANK_SEQ,      new DigitalPinBankInterface(cv_out_sequence_pin,    4));
    }
#elif defined(ENABLE_GATES_MCP23S17)
    #include "interfaces/mcp23s17_interface.h"

    void setup_gate_manager() {
        Serial.println("setup_gate_manager..");
        MCP23S17BankInterface *mcp_interface = new MCP23S17BankInterface();
        Serial.println("\tdid mcp_interface");
        gate_manager->add_bank_interface(BANK_CLOCK, new VirtualBankInterface(mcp_interface, 0, 8));
        Serial.println("\tadded virtual bank interface 1");
        gate_manager->add_bank_interface(BANK_SEQ,   new VirtualBankInterface(mcp_interface, 8, 8));
        Serial.println("\tadded virtual bank interface 2");
        Serial.println("returning from setup_gate_manager().");
    }
#else
    void setup_gate_manager() {
        Serial.println("setup_gate_manager: No gates enabled (no ENABLE_GATES_GPIO or ENABLE_GATES_MCP23S17)");
    }
#endif


void set_clock_gate(int gate_number, bool state) {
    gate_manager->send_gate(BANK_CLOCK, gate_number, state);
}
void set_sequence_gate(int gate_number, bool state) {
    gate_manager->send_gate(BANK_SEQ, gate_number, state);
}
