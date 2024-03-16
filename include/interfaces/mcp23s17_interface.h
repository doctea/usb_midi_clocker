#ifdef ENABLE_GATES_MCP23S17

#include "interfaces.h"
#include "MCP23S17.h"   //RobTillaart/MCP23S17
#include "SPI.h"

#define MCP23S17_SPI_CS1_PIN    38
#define MCP23S17_SPI_CS2_PIN    37
/*#define MCP23S17_SPI_MISO       39
#define MCP23S17_SPI_MOSI       26
#define MCP23S17_SPI_SCK        27*/

// todo: accept config (SPI and CS number) in constructor
class MCP23S17BankInterface : public BankInterface {
    public:
        MCP23S17 *mcp = nullptr;

        bool combine_writes = true;
        uint_least8_t num_gates = 16;

        bool *current_states = nullptr;

        bool dirty = false;

        MCP23S17BankInterface() {
            Serial.println("MCP23S17BankInterface() constructor");
            //SPI1.setCS(MCP23S17_SPI_CS1_PIN);
            SPI1.setMISO(39);
            SPI1.setMOSI(26);
            SPI1.setSCK(27);
            //SPI1.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
            //SPI1.beginTransaction(SPISettings(3000000, LSBFIRST, SPI_MODE3));
            //SPI1.setClockDivider(SPI_CLOCK_DIV4);
            SPI1.begin();

            this->current_states = (bool*)calloc(num_gates, sizeof(bool));

            mcp = new MCP23S17(MCP23S17_SPI_CS1_PIN, 0, &SPI1);
            mcp->setSPIspeed(10000000);
            
            Serial.println("\tconstructed!... calling begin()");
            if (!mcp->begin()) {
                Serial.println("\tbegin() return false!");
            }
            Serial.println("\tdid begin()!");
            for(int i = 0 ; i < num_gates ; i++) {
                Serial.printf("\tSetting up pin %i!..\n", i);
                if (!mcp->pinMode1(i, OUTPUT)) {
                    Serial.printf("MCP23s17BankInterface: Error setting pinMode %i\n", i);
                }
                set_gate(i, false);
            }

            // do a debug loop around the outputs, several times, to be sure the results are taken
            for (int x = 0 ; x < 5 ; x++) {
                for (int c = 0 ; c < num_gates ; c++) {
                    set_gate(c, x % 2);
                    this->update();
                    delay(10);
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
            if (!combine_writes) {
                mcp->write1(gate_number, state);
            } else {
                dirty = true;
            }
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
            if (!combine_writes)
                return;

            if (!dirty) 
                return;

            uint_fast16_t v = 0;
            for (uint_fast8_t i = 0 ; i < num_gates ; i++) {
                if (current_states[(num_gates-1)-i])
                    v += (1 << (i));
            }

            mcp->write16(v);

            dirty = false;
        }
};


// experimental -- actually seems to be working on a code level, but the hardware behaves strangely..
// todo: make an interface that can support mixed ins/outs 
// todo: make this use batched reads
// todo: make this set up the SPI in the same way as the Output interface above
class MCP23S17InputBankInterface : public BankInterface {
    public:
        int num_gates = 16;
        MCP23S17 *mcp = nullptr;
        bool *current_states = nullptr;

        MCP23S17InputBankInterface() {
            Serial.println("MCP23S17InputBankInterface() constructor");
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
                if (mcp->pinMode1(i, OUTPUT))
                    mcp->write1(i, false);
                //mcp->setPullup(i, false);
                if (!mcp->pinMode1(i, INPUT)) {
                    Serial.printf("\tMCP23s17BankInterface: Error setting pinMode %i\n", i);
                }

                //set_gate(i, false);
            }

            // do a debug loop around the outputs
            /*for (int x = 0 ; x < 5 ; x++) {
                for (int c = 0 ; c < num_gates ; c++) {
                    set_gate(c, x % 2);
                    delay(10);
                }
            }*/

            Serial.println("finished constructor");
            Serial.printf("!!! last error from mcp = %x\n", mcp->lastError());
        }

        virtual void set_gate(int gate_number, bool state) override {
            return; // input only
        }
        virtual bool check_gate(int gate_number) override {
            //return this->current_states[gate_number];
            return mcp->read1(gate_number);
        }
        virtual void update() override {

        }
};


#endif