#include "interfaces/interfaces.h"
#include "cv_gate_outs.h"
#include "SPI.h"

GateManager *gate_manager = new GateManager();

//#define TEST_MCP23s17_INPUT // for testing gate input mode; which seems to work on a code level, except the hardware seems to let us down

#ifdef ENABLE_GATES_GPIO
    void setup_gate_manager() {
        gate_manager->add_bank_interface(BANK_CLOCK,    new DigitalPinBankInterface(cv_out_clock_pin,       NUM_CLOCKS));
        gate_manager->add_bank_interface(BANK_SEQ,      new DigitalPinBankInterface(cv_out_sequence_pin,    NUM_SEQUENCES));
    }
#elif defined(ENABLE_GATES_MCP23S17)
    #include "interfaces/mcp23s17_interface.h"

    //#define DUPLICATE_CLOCKS

    void setup_gate_manager() {
        Serial.println("setup_gate_manager..");

        SPI1.setMISO(MCP23S17_SPI_MISO);
        SPI1.setMOSI(MCP23S17_SPI_MOSI);
        SPI1.setSCK(MCP23S17_SPI_SCK);
        //SPI1.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
        //SPI1.beginTransaction(SPISettings(3000000, LSBFIRST, SPI_MODE3));
        //SPI1.setClockDivider(SPI_CLOCK_DIV4);
        SPI1.begin();


        #ifdef TEST_MCP23s17_INPUT
            MCP23S17InputBankInterface *mcp_interface = new MCP23S17InputBankInterface();
        #else
            MCP23S17BankInterface *mcp_interface = new MCP23S17BankInterface("MCP1", MCP23S17_SPI_CS1_PIN, 0, &SPI1);
        #endif
        Serial.println("\tdid mcp_interface");
        byte num_gates = 8;
        /*  // for some reason, these are all coming out funny on the current pcb revision, so here is the empirical mappings to use
            0 -> 0 -> 7
            1 -> 1 -> 6
            2 -> 7 -> 0
            3 -> 5 -> 2
            4 -> 3 -> 4
            5 -> 6 -> 1
            6 -> 4 -> 3
            7 -> 2 -> 5
            // and now reordered so as to be useful for remapping
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
        gate_manager->add_bank_interface(
            BANK_SEQ, 
            new VirtualRemapBankInterface(
                "sequences",                            // remap the output-gate mappings because
                new VirtualBankInterface("sequences", mcp_interface, 0, num_gates, true),  // use outputs 0-7 of underlying MCP object
                remap_clocks, 
                num_gates
            )
        );
        
        // use outputs 8-15 of the underlying MCP object, and reverse them
        gate_manager->add_bank_interface(
            BANK_CLOCK,   
            new VirtualBankInterface("clocks", mcp_interface, num_gates, num_gates, false)
        );

        #ifdef ENABLE_GATES_BANK_EXTRA
            MCP23S17BankInterface *mcp_interface_2 = new MCP23S17BankInterface("MCP2", MCP23S17_SPI_CS2_PIN, 0, &SPI1);
            gate_manager->add_bank_interface(
                BANK_EXTRA_1, 
                new VirtualBankInterface("gates", mcp_interface_2, num_gates, num_gates, false)
            );
            //gate_manager->add_bank_interface(BANK_EXTRA_2, new VirtualBankInterface(mcp_interface_2, 0, num_gates, false));
            gate_manager->add_bank_interface(BANK_EXTRA_2, 
                //new VirtualRemapBankInterface("inputs",
                    new MCP23S17SharedInputBankInterface("inputs", mcp_interface_2, remap_clocks, 0, num_gates)
                    //remap_clocks, 
                    //num_gates  
                //)
            );
            //, 0, num_gates, false));
        #endif
        
        Serial.println("returning from setup_gate_manager().");
        Serial.flush();

    }
#else
    void setup_gate_manager() {
        Serial.println("setup_gate_manager: No gates enabled (no ENABLE_GATES_GPIO or ENABLE_GATES_MCP23S17)");
    }
#endif

#include "clock.h"
#include <uClock.h>
volatile bool cv_clock_ticked_flag;
bool has_gone_off = true;

bool has_cv_clock_ticked() {
    if (cv_clock_ticked_flag) {
        cv_clock_ticked_flag = false;
        return true;
    }
    return false;
}

void checkClock() {
    MCP23S17SharedInputBankInterface *mcp_interface = (MCP23S17SharedInputBankInterface*)gate_manager->banks[BANK_EXTRA_2];
    if (clock_mode==CLOCK_EXTERNAL_CV && mcp_interface->check_gate(7) && has_gone_off) {
        Serial.println("cv clock ticked!");
        cv_clock_ticked_flag = true;
        has_gone_off = false;
        uClock.clockMe();
    } else if (clock_mode==CLOCK_EXTERNAL_CV && !mcp_interface->check_gate(7) && !has_gone_off) {
        cv_clock_ticked_flag = false;
        has_gone_off = true;
    }
}


void set_clock_gate(int gate_number, bool state) {
    gate_manager->send_gate(BANK_CLOCK, gate_number, state);
    //set_sequence_gate(gate_number, state);
    #ifdef DUPLICATE_CLOCKS
        // duplicate clocks onto the (currently unused) sequencer outputs
        if (gate_number>=0 && gate_number<4) {
            gate_manager->send_gate(BANK_SEQ, gate_number+4, state);
        }
    #endif
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
    void GateManager::create_controls(Menu *menu) {
        menu->select_page_for_name("Sequencer");   // since we only call create_controls later in the setup process, we need to move back to the second page ('Sequencer') to add these items
        menu->add(new GatesDisplay("Gates"));

        byte gate_count = 0;

        Serial.println("GateManager::create_controls("); Serial_flush();
        debug_free_ram();
        
        char label[MENU_C_MAX];
        for (int bank = 0 ; bank < this->num_banks ; bank++) {
            snprintf(label, MENU_C_MAX, "Bank %i: %s", bank, bank==BANK_CLOCK ? "Clocks" : "Sequences");
            ObjectMultiToggleControl *gate_toggles = new ObjectMultiToggleControl(label, true);

            Serial.printf("Creating gate toggles for bank %i (%s)\n", bank, label); Serial_flush();
            for (int gate = 0 ; gate < this->banks[bank]->num_gates ; gate++) {
                //snprintf(label, MENU_C_MAX, "G%2i", gate);
                gate_toggles->addItem(new GateMultiToggleItem(gate_labels[gate_count++], bank, gate));
            }
            menu->add(gate_toggles);
        }
    }
#endif