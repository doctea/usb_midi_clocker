#ifdef ENABLE_GATES_MCP23S17

#include "interfaces.h"
#include "MCP23S17.h"   //RobTillaart/MCP23S17
#include "SPI.h"

#define MCP23S17_SPI_CS1_PIN    38
#define MCP23S17_SPI_CS2_PIN    37
#define MCP23S17_SPI_MISO       39
#define MCP23S17_SPI_MOSI       26
#define MCP23S17_SPI_SCK        27

// todo: accept config (SPI and CS number) in constructor
class MCP23S17BankInterface : public BankInterface {
    public:
        MCP23S17 *mcp = nullptr;

        bool combine_writes = true;
        uint_least8_t num_gates = 16;

        bool *current_states = nullptr;

        bool dirty = false;

        MCP23S17BankInterface(const char *label, int cs_pin = MCP23S17_SPI_CS1_PIN, int address = 0, SPIClass *spi = &SPI1, int num_gates = 16) {
            Serial.println("MCP23S17BankInterface() constructor");
            this->label = label;
            //SPI1.setCS(MCP23S17_SPI_CS1_PIN);
            this->num_gates = num_gates;

            this->current_states = (bool*)calloc(num_gates, sizeof(bool));

            mcp = new MCP23S17(cs_pin, address, spi);
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


class MCP23S17SharedInputBankInterface : public BankInterface {
    public:
        MCP23S17 *mcp = nullptr;
        int *remap_pins = nullptr;
        int start_gate = 0;

        MCP23S17SharedInputBankInterface(const char *label, MCP23S17BankInterface *parent, int *remap_pins, int start_gate, int num_gates) {
            this->label = label;
            Serial.println("MCP23S17SharedInputBankInterface() constructor");

            this->remap_pins = (int*)CALLOC_FUNC(num_gates, sizeof(int));
            for (uint_least8_t i = 0 ; i < num_gates ; i++) {
                this->remap_pins[i] = remap_pins[i];
            }

            mcp = parent->mcp;
            this->start_gate = start_gate;
            this->num_gates = num_gates;
            
            for(int i = 0 ; i < num_gates ; i++) {
                Serial.printf("\tSetting up pin %i!..\n", i);
                if (parent->mcp->pinMode1(start_gate+this->remap_pins[i], INPUT)) {
                    parent->mcp->setPullup(start_gate+this->remap_pins[i], true);
                } else {
                    Serial.printf("MCP23s17BankInterface: Error setting pinMode %i\n", i);
                }
                Serial.flush();
            }
            Serial.flush();
            Serial.println("finished constructor");
        }

        virtual void set_gate(int gate_number, bool state) override {
            return; // input only
        }
        virtual bool check_gate(int gate_number) override {
            //return this->current_states[gate_number];
            bool v = mcp->read1(start_gate + this->remap_pins[gate_number]);
            if (v) {
                Serial.printf("MCP23S17SharedInputBankInterface::check_gate(%i) = %i\n", gate_number, v);
            } else {
                //Serial.printf("MCP23S17SharedInputBankInterface::check_gate(%i) = %i\n", gate_number, v);
            }
            if (v != last_state[gate_number]) {
                Serial.printf("MCP23S17SharedInputBankInterface::check_gate(%i) state changed to %i\n", gate_number, v);
                state_changed[gate_number] = true;
            } 
            last_state[gate_number] = v;
            return v;
        }
        bool ticked_flag = false;
        
        bool last_state[8] = { false };
        bool state_changed[8] = { false };
        
        virtual void update() override {
            //Serial.printf("MCP23S17SharedInputBankInterface::update() called!\n");
            for (int i = 0 ; i < num_gates ; i++) {
                check_gate(i);
            }
            if (last_state[7] && state_changed[7]) {
                Serial.println("last_state[7] is true and state_changed[7] is true - setting ticked flag!");
                // do a uClock external clock trigger
                this->ticked_flag = true;
            }
            /*if (this->ticked_flag) {
                Serial.println("MCP23S17SharedInputBankInterface::update() ticked flag is set!");
            } else {
                Serial.println("MCP23S17SharedInputBankInterface::update() ticked flag is NOT set!");
            }*/
            for (int i = 0 ; i < num_gates ; i++) {
                if (state_changed[i]) {
                    //Serial.printf("MCP23S17SharedInputBankInterface::update() gate %i changed to %i\n", i, last_state[i]);
                    state_changed[i] = false;
                }
            }
        }
        virtual bool has_ticked() {
            //Serial.print("CHECKING HAS_TICKED!: ");
            if (this->ticked_flag) {
                Serial.println("HAS TICKED FLAG SET! returning true and setting unticked!");
                this->ticked_flag = false;
                return true;
            }
            //Serial.println("HAS TICKED FLAG NOT SET! returning false!");
            return false;
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

        MCP23S17InputBankInterface(const char *label, int cs_pin = MCP23S17_SPI_CS1_PIN, int address = 0, SPIClass *spi = &SPI1) {
            this->label = label;
            Serial.println("MCP23S17InputBankInterface() constructor");
            /*SPI1.setCS(38);
            SPI1.setMISO(39);
            SPI1.setMOSI(26);
            SPI1.setSCK(27);
            //SPI1.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
            SPI1.setClockDivider(SPI_CLOCK_DIV4);
            SPI1.begin();*/

            this->current_states = (bool*)calloc(num_gates, sizeof(bool));

            mcp = new MCP23S17(cs_pin, address, spi);
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