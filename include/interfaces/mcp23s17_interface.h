#ifdef ENABLE_GATES_MCP23S17

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
        bool *current_states = nullptr;

        MCP23S17BankInterface() {
            Serial.println("MCP23S17BankInterface() constructor");
            SPI1.setCS(38);
            SPI1.setMISO(39);
            SPI1.setMOSI(26);
            SPI1.setSCK(27);
            //SPI1.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
            SPI1.setClockDivider(SPI_CLOCK_DIV4);
            SPI1.begin();

            this->current_states = (bool*)calloc(num_gates, sizeof(bool));

            mcp = new MCP23S17(38, 0, &SPI1);
            Serial.println("\tconstructed!... calling begin()");
            if (!mcp->begin()) {
                Serial.println("\tbegin() return false!");
            }
            Serial.println("\tdid begin()!");
            for(int i = 0 ; i < num_gates ; i++) {
                Serial.printf("\tSetting up pin %i!..\n", i);
                if (!mcp->pinMode(i, OUTPUT)) {
                    Serial.printf("MCP23s17BankInterface: Error setting pinMode %i\n", i);
                }
                set_gate(i, false);
            }

            for (int x = 0 ; x < 5 ; x++) {
                for (int c = 0 ; c < num_gates ; c++) {
                    set_gate(c, x % 2);
                    delay(100);
                }
            }

            Serial.println("finished constructor");
            Serial.printf("!!! last error from mcp = %x\n", mcp->lastError());
        }

        virtual void set_gate(int gate_number, bool state) override {
            //this->dirty = true;
            if (gate_number >= num_gates) {
                //messages_log_add(String("Attempted to send to invalid gate %i:%i") + String(bank) + String(": ") + String(gate));
                return;            
            }
            mcp->digitalWrite(gate_number, state);
            this->current_states[gate_number] = state;

            // for debug, output inversed gates on shifted up gate numbers
            /*if (gate_number<4) {
                mcp->digitalWrite(gate_number+4, !state);
                this->current_states[gate_number+4] = !state;
            }*/

            /*for (int i = 0 ; i < 100 ; i++) {
                mcp->digitalWrite(gate_number, state);
                //if (gate_number<4) mcp->digitalWrite(gate_number+4, !state);
            }*/
            //Serial.printf("!!! last error from mcp for gate %2i: %x\n", gate_number, mcp->lastError());
        }
        virtual bool check_gate(int gate_number) override {
            return this->current_states[gate_number];
        }
        virtual void update() override {

        }
};

#endif