#ifndef BEHAVIOUR_BASE__INCLUDED
#define BEHAVIOUR_BASE__INCLUDED

#include <Arduino.h>

#include <MIDI.h>
#include <USBHost_t36.h>

#include <LinkedList.h>

#include "midi/midi_mapper_matrix_types.h"

#include "parameters/Parameter.h"

class MenuItem;

using namespace midi;

enum BehaviourType {
    undefined,
    usb,        // a USB MIDI device that identifies as a USB MIDI device
    serial,     // a MIDI device connected over a hardware serial port
    usbserial,       // a USB device that connects over serial, but doesn't support MIDI
    usbserialmidi   // a USB MIDI device that identifies as a SERIAL device (ie OpenTheremin, Arduino device á la Hairless MIDI)
};

class DeviceBehaviourUltimateBase {
    public:

    bool debug = false;

    source_id_t source_id = -1;
    target_id_t target_id = -1;

    //MIDIOutputWrapper *wrapper = nullptr;

    DeviceBehaviourUltimateBase() = default;
    virtual ~DeviceBehaviourUltimateBase() = default;

    virtual const char *get_label() {
        return (char*)"UltimateBase";
    }

    virtual bool has_input() { return false; }
    virtual bool has_output() { return false; }
    // input/output indicator
    virtual const char *get_indicator() {
        static bool done = false;
        static char indicator_text[5];
        if (!done) {
            sprintf(indicator_text,"%c%c", this->has_input()?'I':' ', this->has_output()?'O':' ');
        }
        return indicator_text;
    }

    virtual int getType() {
        return BehaviourType::undefined;
    }

    virtual void setup_callbacks() {};

    virtual bool is_connected() {
        return false;
    }

    virtual void read() {};
    // called every loop
    virtual void loop(uint32_t ticks) {};
    // on_pre_clock called before the clocks are sent
    virtual void on_pre_clock(uint32_t ticks) {};
    // called during tick when behaviour_manager send_clocks is called
    virtual void send_clock(uint32_t ticks) {};
    // called every tick, after clocks sent
    virtual void on_tick(uint32_t ticks) {};
    // called when new bar starts
    virtual void on_bar(int bar_number) {};
    virtual void on_end_bar(int bar_number) {};
    // called when the clock is restarted
    virtual void on_restart() {};
    // called when we change phrase
    virtual void on_phrase(uint32_t phrase) {};
    virtual void receive_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
    // called when a note_off message is received from the device
    virtual void receive_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
    virtual void receive_control_change (uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
    virtual void init() {};

    // tell the device to play a note on
    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
        //Serial.println("DeviceBehaviourUltimateBase#sendNoteOn");
        this->actualSendNoteOn(note, velocity, channel);
    };
    // tell the device to play a note off
    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
        //Serial.println("DeviceBehaviourUltimateBase#sendNoteOff");
        this->actualSendNoteOff(note, velocity, channel);
    };
    // tell the device to send a control change
    virtual void sendControlChange(uint8_t number, uint8_t value, uint8_t channel) {
        //Serial.println("DeviceBehaviourUltimateBase#sendControlChange");
        this->actualSendControlChange(number, value, channel);
    };
    // tell the device to send a realtime message
    virtual void sendRealTime(uint8_t message) {
        //Serial.println("DeviceBehaviourUltimateBase#sendRealTime");
        this->actualSendRealTime(message);
    };    
    virtual void sendNow() {
    };

    // use the underlying object to actually send a value
    virtual void actualSendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
    }
    virtual void actualSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
    }
    virtual void actualSendControlChange(uint8_t number, uint8_t value, uint8_t channel) {
    }
    virtual void actualSendRealTime(uint8_t message) {
    }

    // parameter handling shit
    LinkedList<DoubleParameter*> *parameters = new LinkedList<DoubleParameter*>();
    virtual LinkedList<DoubleParameter*> *get_parameters () {
        if (parameters->size()==0)
            this->initialise_parameters();
        return parameters;
    }
    virtual LinkedList<DoubleParameter*> *initialise_parameters() {
        /*if (parameters==nullptr)
            parameters = new LinkedList<DoubleParameter*>();*/
        return parameters;
    }
    virtual bool has_parameters() {
        return this->get_parameters()->size()>0;
    }
    virtual DoubleParameter* getParameterForLabel(char *label) {
        //Serial.printf(F("getParameterForLabel(%s) in behaviour %s..\n"), label, this->get_label());

        for (int i = 0 ; i < parameters->size() ; i++) {
            //Serial.printf(F("Comparing '%s' to '%s'\n"), parameters->get(i)->label, label);
            if (strcmp(parameters->get(i)->label, label)==0) 
                return parameters->get(i);
        }
        Serial.printf(F("WARNING/ERROR in behaviour %s: didn't find a Parameter labelled %s\n"), this->get_label(), label);
        return nullptr;
    }

    /*
    virtual void save_sequence_add_lines(LinkedList<String> *lines) {
        // todo: save parameter mappings...
    }
    virtual bool parse_sequence_key_value(String key, String value) {
        // todo: reload parameter mappings...
        return false;
    }
    */

    virtual void reset_all_mappings() {
        for (int i = 0 ; i < this->parameters->size() ; i++) {
            this->parameters->get(i)->reset_mappings();
        }
    }

    virtual void save_project_add_lines(LinkedList<String> *lines) {

    }
    virtual bool parse_project_key_value(String key, String value) {
        return false;
    }

    // save all the parameter mapping settings; override in subclasses, which should call back up the chain
    virtual void save_sequence_add_lines(LinkedList<String> *lines) {   
        LinkedList<DoubleParameter*> *parameters = this->get_parameters();
        for (int i = 0 ; i < parameters->size () ; i++) {
            DoubleParameter *parameter = parameters->get(i);
            char line[100];
            // todo: move handling of this into Parameter, or into a third class that can handle saving to different formats..?
            // todo: make these mappings part of an extra type of thing rather than associated with sequence?
            // todo: move these to be saved with the project instead?
            for (int slot = 0 ; slot < 3 ; slot++) { // TODO: MAX_CONNECTION_SLOTS...?
                if (parameter->connections[slot].parameter_input==nullptr) continue;      // skip if no parameter_input configured in this slot
                if (parameter->connections[slot].amount==0.00) continue;                     // skip if no amount configured for this slot

                char input_name = parameter->get_input_name_for_slot(slot);

                sprintf(line, "parameter_%s_%i=%c|%3.3f", 
                    parameter->label, 
                    slot, 
                    input_name,
                    //'A'+slot, //TODO: implement proper saving of mapping! /*parameter->get_connection_slot_name(slot), */
                    //parameter->connections[slot].parameter_input->name,
                    parameter->connections[slot].amount
                );
                lines->add(String(line));
            }
        }
    }

    // ask behaviour to process the key/value pair
    virtual bool load_parse_key_value(String key, String value) {
        // todo: reload parameter mappings...
        //Serial.printf(F("parse_sequence_key_value passed '%s' => '%s'...\n"), key.c_str(), value.c_str());
        //static String prefix = String("parameter_" + this->get_label());
        const char *prefix = "parameter_";
        if (this->has_parameters() && key.startsWith(prefix)) {
            // sequence save line looks like: `parameter_Filter Cutoff_0=A|1.000`
            //                                 ^^head ^^_^^param name^_slot=ParameterInputName|Amount
            key = key.replace(prefix, "");

            String parameter_name = key.substring(0, key.indexOf('_'));
            int slot_number = key.substring(key.indexOf('_')+1).toInt();
            String input_name = value.substring(0, key.indexOf('|'));
            double amount = value.substring(key.indexOf('|')+1).toFloat();

            //this->getParameterForLabel((char*)parameter_name.c_str())->set_slot_input(slot_number, get_input_for_parameter_name(parameter_name)));parameter_name.c_str()[0]);
            DoubleParameter *p = this->getParameterForLabel((char*)parameter_name.c_str());
            if (p!=nullptr) {
                //Serial.printf(F("\t%s: setting slot_number %i to %f\n"), p->label, slot_number, amount);
                //BaseParameterInput *input = parameter_manager->getInputForName(input_name.c_str()[0]);
                p->set_slot_input(slot_number, input_name.c_str()[0]);
                p->set_slot_amount(slot_number, amount);
                return true;
            } else {
                Serial.printf("WARNING: Couldn't find a Parameter for name %s\n", parameter_name.c_str());
                return false;
            }
            //Serial.printf(F("\t slot_number %i and amount %f but no parameter found for %s in %s\n"), slot_number, amount, parameter_name.c_str(), this->get_label());
            return true;
        }
        ///Serial.printf(F("...load_parse_key_value(%s, %s) isn't a parameter!\n"));
        return false;
    }

    #ifdef ENABLE_SCREEN
        LinkedList<MenuItem*> *menuitems = nullptr;
        FLASHMEM virtual LinkedList<MenuItem*> *make_menu_items();
    #endif
    
};

#endif