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
        byte num_gates = 8;
        /*  // for some reason, these are all coming out funny on the pcb, so her eis the empirical mappings to use
            0 -> 0 -> 7
            1 -> 1 -> 6
            2 -> 7 -> 0
            3 -> 5 -> 2
            4 -> 3 -> 4
            5 -> 6 -> 1
            6 -> 4 -> 3
            7 -> 2 -> 5

            2 -> 7 -> 0
            5 -> 6 -> 1
            3 -> 5 -> 2
            6 -> 4 -> 3
            4 -> 3 -> 4
            7 -> 2 -> 5
            1 -> 1 -> 6
            0 -> 0 -> 7
        */
        int remap_clocks[num_gates] = { 2, 5, 3, 6, 4, 7, 1, 0 };
        gate_manager->add_bank_interface(BANK_SEQ, 
            new VirtualRemapBankInterface(                              // remap the output-gate mappings because
                new VirtualBankInterface(mcp_interface, 0, num_gates),  // use outputs 0-7 of underlying MCP object
                remap_clocks, 
                num_gates
            )
        );
        
        // use outputs 8-15 of the underlying MCP object, and reverse them
        gate_manager->add_bank_interface(BANK_CLOCK,   new VirtualBankInterface(mcp_interface, num_gates, num_gates, true));
        
        Serial.println("returning from setup_gate_manager().");
    }
#else
    void setup_gate_manager() {
        Serial.println("setup_gate_manager: No gates enabled (no ENABLE_GATES_GPIO or ENABLE_GATES_MCP23S17)");
    }
#endif


void set_clock_gate(int gate_number, bool state) {
    gate_manager->send_gate(BANK_CLOCK, gate_number, state);
    //set_sequence_gate(gate_number, state);
}
void set_sequence_gate(int gate_number, bool state) {
    gate_manager->send_gate(BANK_SEQ, gate_number, state);
}

#ifdef ENABLE_SCREEN
    #include "menu.h"

    void setup_gate_manager_menus() {
        gate_manager->create_controls(menu);
    }

    #include "menuitems_object_multitoggle.h"
    class GateMultiToggleItem : public MultiToggleItemBase {
        public:
            int bank_number, gate_number;
            GateMultiToggleItem (const char *label, int bank_number, int gate_number) :
                MultiToggleItemBase(label), 
                bank_number(bank_number), gate_number(gate_number) {
            }

            virtual bool do_getter() {
                return gate_manager->check_gate(bank_number, gate_number);
            }
            virtual void do_setter(bool value) {
                gate_manager->send_gate(bank_number, gate_number, value);
            }
    };

    const char *gate_labels[16] = {
        " 1", " 2", " 3", " 4", " 5", " 6", " 7", " 8", " 9", "10", "11", "12", "13", "14", "15", "16"
    };

    #include "mymenu/menu_gatedisplay.h"
    LinkedList<MenuItem*> *GateManager::create_controls(Menu *menu) {
        menu->select_page(2);

        menu->add(new GatesDisplay("Gates"));

        char label[MENU_C_MAX];
        for (int bank = 0 ; bank < this->num_banks ; bank++) {
            snprintf(label, MENU_C_MAX, "Bank %i", bank);
            ObjectMultiToggleControl *gate_toggles = new ObjectMultiToggleControl(label, true);

            for (int gate = 0 ; gate < this->banks[bank]->num_gates ; gate++) {
                //snprintf(label, MENU_C_MAX, "G%2i", gate);
                gate_toggles->addItem(new GateMultiToggleItem(gate_labels[(bank*num_banks)+gate], bank, gate));
            }
            menu->add(gate_toggles);
        }

    }
#endif