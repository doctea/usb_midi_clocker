#ifdef ENABLE_MCP23S17

#include "interfaces.h"
#include "MCP23S17.h"   //RobTillaart/MCP23S17
#include "SPI.h"

#define MCP23S17_SPI_CS1_PIN    38
#define MCP23S17_SPI_CS2_PIN    37
/*#define MCP23S17_SPI_MISO       39
#define MCP23S17_SPI_MOSI       26
#define MCP23S17_SPI_SCK        27*/

class MCP23S17BankInterface : public BankInterface {
    public:
        int num_gates = 16;
        MCP23S17 *mcp = nullptr;

        MCP23S17BankInterface() {
            mcp = new MCP23S17(MCP23S17_SPI_CS1_PIN, &SPI1);
            mcp->begin();
            for(int i = 0 ; i < num_gates ; i++) {
                if (!mcp->pinMode(i, OUTPUT)) {
                    Serial.printf("MCP23s17BankInterface: Error setting pinMode %i\n", i);
                }
            }
        }

        virtual void set_gate(int gate_number, bool state) override {
            //this->dirty = true;
            if (gate_number >= num_gates) {
                //messages_log_add(String("Attempted to send to invalid gate %i:%i") + String(bank) + String(": ") + String(gate));
                return;            
            }
            mcp->digitalWrite(gate_number, state);
        }
        virtual void update() override {

        }
};

#endif