#ifndef BEHAVIOUR_MANAGER__INCLUDED
#define BEHAVIOUR_MANAGER__INCLUDED

#include "behaviour_base.h"
#include "behaviour_base_usb.h"
#include "behaviour_base_serial.h"

#include "multi_usb_handlers.h"

#include <LinkedList.h>

class DeviceBehaviourManager {
    public:
        bool debug = false;

        static DeviceBehaviourManager* getInstance();

        // all of the registered behaviours
        LinkedList<DeviceBehaviourUltimateBase *> behaviours = LinkedList<DeviceBehaviourUltimateBase *>();

        // registered behaviours separated by type, so that we can treat them differently for connection and listing purposes
        LinkedList<DeviceBehaviourUSBBase *> behaviours_usb = LinkedList<DeviceBehaviourUSBBase *>();
        LinkedList<DeviceBehaviourSerialBase *> behaviours_serial = LinkedList<DeviceBehaviourSerialBase *>();

        void registerBehaviour(DeviceBehaviourUSBBase *behaviour) {
            if (behaviour==nullptr) {
                Serial.println("registerBehaviour<DeviceSerialUSB> passed a nullptr!"); Serial.flush();
                return;
            }
            this->behaviours_usb.add(behaviour);
            this->behaviours.add(behaviour);
        }

        void registerBehaviour(DeviceBehaviourSerialBase *behaviour) {
            if (behaviour==nullptr) {
                Serial.println("registerBehaviour<DeviceSerialBase> passed a nullptr!"); Serial.flush();
                return;
            }
            Serial.printf("registerBehaviour<DeviceBehaviourSerialBase> for %ith item passed %p\n", behaviours.size(), behaviour);
            this->behaviours_serial.add(behaviour);
            this->behaviours.add(behaviour);
        }

        bool attempt_usb_device_connect(uint8_t idx, uint32_t packed_id) {
            // loop over the registered behaviours and if the correct one is found, set it up
            const int size = behaviours_usb.size();
            for (int i = 0 ; i < size ; i++) {
                DeviceBehaviourUSBBase *behaviour = behaviours_usb.get(i);
                //if (behaviour->getType()==BehaviourType::usb) {
                    //DeviceBehaviourUSBBase *usb_behaviour = behaviour;
                    //if (this->debug) 
                    Serial.printf("DeviceBehaviourManager#attempt_usb_device_connect(): checking behaviour %i -- does it match %08X?\n", i, packed_id);
                    usb_midi_slots[idx].packed_id = packed_id;
                    if (behaviour->matches_identifiers(packed_id)) {
                        Serial.printf("\tDetected!  Behaviour %i on usb midi idx %i\n", i, idx); //-- does it match %u?\n", i, packed_id);
                        behaviour->connect_device(usb_midi_slots[idx].device);
                        return true;
                    } else {
                        Serial.printf("Didn't find a behaviour for %u, %08X!", idx, packed_id);
                    }
                //}
            }
            return false;
        }

        void do_reads() {
            for (int i = 0 ; i < behaviours.size() ; i++) {
                //Serial.printf("\tdo_reads on index %i (@%p) about to call read..\n", i, behaviours.get(i)); Serial.flush();
                behaviours.get(i)->read();
                //Serial.printf("\tdo_reads on index %i (@%p) called read..\n", i, behaviours.get(i)); Serial.flush();
            }
            /*for (int i = 0 ; i < NUM_USB_DEVICES ; i++) {
                while(usb_midi_slots[i].device!=nullptr && usb_midi_slots[i].device->read()); //device->read());
            }
            for (int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
                while(midi_out_serial[i]->read());
            }*/
        }
        /*#define SINGLE_FRAME_READ
        void read_midi_serial_devices() {
            #ifdef SINGLE_FRAME_READ
                //int i = 0;
                for (int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
                    while (midi_out_serial[i]->read());
                }
            #else
                static int counter = 0;
                if (counter>=NUM_MIDI_OUTS) 
                    counter = 0;
                while(midi_out_serial[counter]->read());
                counter++;
            #endif
        }*/
        //#define SINGLE_FRAME_READ_ONCE
        #define SINGLE_FRAME_READ_ALL

        /*void read_midi_usb_devices() {
        #ifdef SINGLE_FRAME_READ_ALL
            for (int i = 0 ; i < NUM_USB_DEVICES ; i++) {
            while(usb_midi_slots[i].device!=nullptr && usb_midi_slots[i].device->read()); //device->read());
            }
        #else
            #ifdef SINGLE_FRAME_READ_ONCE
            //static int counter;
            for (int i = 0 ; i < NUM_USB_DEVICES ; i++) {
                //while(usb_midi_device[i]->read());
                if (usb_midi_slots[i].device!=nullptr && usb_midi_slots[i].device->read()) {
                //usb_midi_device[counter%NUM_USB_DEVICES]->sendNoteOn(random(0,127),random(0,127),random(1,16));
                //Serial.printf("%i: read data from %04x:%04x\n", counter, usb_midi_device[i]->idVendor(), usb_midi_device[i]->idProduct());
                }
                //counter++;
            }
            #else
            static int counter;
            // only all messages from one device per loop
            if (counter>=NUM_USB_DEVICES)
                counter = 0;
            while(usb_midi_slots[counter].read());
            counter++;
            #endif
        #endif
        }*/

        void send_clocks() {    // replaces behaviours_send_clock
            int size = behaviours.size();
            for (int i = 0 ; i < size ; i++) {
                //Serial.printf("behaviours#send_clocks calling send_clock on behaviour %i\n", i); Serial.flush();
                behaviours.get(i)->send_clock(ticks);
                //Serial.printf("behaviours#send_clocks called send_clock on behaviour %i\n", i); Serial.flush();
            }  
            /*size = behaviours_serial.size();
            for (int i = 0 ; i < size ; i++) {
                behaviours_serial.get(i)->send_clock(ticks);
            }*/
        }

        void do_phrase(int phrase) {
            int size = behaviours.size();
            for (int i = 0 ; i < size ; i++) {
                behaviours.get(i)->on_phrase(phrase);
            }
            /*size = behaviours_serial.size();
            for (int i = 0 ; i < size ; i++) {
                behaviours_serial.get(i)->on_phrase(phrase);
            }*/
        }

        void do_bar(int bar) {
            int size = behaviours.size();
            for (int i = 0 ; i < size ; i++) {
                behaviours.get(i)->on_bar(bar);
            }
        }
        void do_end_bar(int bar) {
            int size = behaviours.size();
            for (int i = 0 ; i < size ; i++) {
                behaviours.get(i)->on_end_bar(bar);
            }
        }

        void do_loops() {       // replaces behaviours_loop
            unsigned long temp_tick;
            //noInterrupts();
            temp_tick = ticks;
            int size = behaviours.size();
            for (int i = 0 ; i < size ; i++) {
                DeviceBehaviourUltimateBase *behaviour = behaviours.get(i);
                if (behaviour!=nullptr) {
                    //Serial.printf("behaviours#do_loops calling loop on behaviour %i\n", i); Serial.flush();
                    behaviours.get(i)->loop(temp_tick);
                    //Serial.printf("behaviours#do_loops called loop on behaviour %i\n", i); Serial.flush();
                }
            }
        }

        void do_pre_clock(unsigned long in_ticks) {
            int size = behaviours.size();
            for (int i = 0 ; i < size ; i++) {
                behaviours.get(i)->on_pre_clock(in_ticks);
            }
        }

        void do_ticks(unsigned long in_ticks) { // replaces behaviours_do_tick
            int size = behaviours.size();
            for (int i = 0 ; i < size ; i++) {
                //Serial.printf("behaviours#do_ticks calling on_tick on behaviour %i\n", i); Serial.flush();
                behaviours.get(i)->on_tick(in_ticks);
                //Serial.printf("behaviours#do_ticks called on_tick on behaviour %i\n", i); Serial.flush();
            }
        }

        void on_restart() {
            int size = behaviours.size();
            for(int i = 0 ; i < size ; i++) {
                //Serial.printf("behaviours#on_restart calling on_restart on behaviour %i\n", i); Serial.flush();
                behaviours.get(i)->on_restart();
                //Serial.printf("behaviours#on_restart called on_restart on behaviour %i\n", i); Serial.flush();
            }
        }

        DeviceBehaviourUltimateBase *find_behaviour_for_label(String label) {
            for (int i = 0 ; i < this->behaviours.size() ; i++) {
                if (label.equals(this->behaviours.get(i)->get_label()))
                    return this->behaviours.get(i);
            }
            return nullptr;
        }

        bool load_parse_line(String line) {
            line = line.replace('\n',"");
            String key = line.substring(0, line.indexOf('='));
            String value = line.substring(line.indexOf('=')+1);
            return this->load_parse_key_value(key, value);
        }

        bool load_parse_key_value(String key, String value) {
            static DeviceBehaviourUltimateBase *current_behaviour = nullptr;
            if (key.equals("behaviour_start")) {
                Serial.printf("found behaviour_start for '%s'\n", value.c_str());
                current_behaviour = this->find_behaviour_for_label(value);
                return true;
            } else if (key.equals("behaviour_end")) {
                Serial.printf("found behaviour_end for '%s'\n", value.c_str());
                current_behaviour = nullptr;
                return true;
            } else if (current_behaviour!=nullptr && current_behaviour->load_parse_key_value(key, value)) {
                Serial.printf("loaded key %s for value '%s'\n", key.c_str(), value.c_str());
                return true;
            }
            Serial.printf("behaviour_manager processing %s => %s, but not a behaviour (or unhandled key)\n", key.c_str(), value.c_str());
            return false;
        }

        /*void load_project_parse_key_value(String key, String value) {
            Serial.printf("BehaviourManager#load_project_parse_key_value('%s', '%s')\n", key.c_str(), value.c_str());
            //String initial_key = key.substring(0, key.indexOf('_'));
            for (int i = 0 ; i < behaviours.size() ; i++) {
                //if (behaviours.get(i)->matches(initial_key))
                    if (behaviours.get(i)->parse_project_key_value(key, value))
                        return;
            }
        }

        void save_project_add_lines(LinkedList<String> *lines) { 
            for (int i = 0 ; i < behaviours.size() ; i++) {
                behaviours.get(i)->save_project_add_lines(lines);
            }
        }

        // ask behaviours if a sequence save key-value pair applies to them; early return if one is found
        void load_sequence_parse_key_value(String key, String value) {
            Serial.printf("BehaviourManager#load_sequence_parse_key_value('%s', '%s')\n", key.c_str(), value.c_str());
            for (int i = 0 ; i < behaviours.size() ; i++) {
                if (behaviours.get(i)->parse_sequence_key_value(key, value)) {
                    return;
                }
            }
        }*/

        // ask each behaviour to add option lines to save project file
        void save_project_add_lines(LinkedList<String> *lines) {
            for (int i = 0 ; i < behaviours.size() ; i++) {
                lines->add("behaviour_start=" + String(behaviours.get(i)->get_label()));
                behaviours.get(i)->save_project_add_lines(lines);
                lines->add("behaviour_end=" + String(behaviours.get(i)->get_label()));
            }
        }

        // ask each behaviour to add option lines to save sequence file
        void save_sequence_add_lines(LinkedList<String> *lines) {
            //LinkedList<String> lines = LinkedList<String>();
            for (int i = 0 ; i < behaviours.size() ; i++) {
                lines->add("behaviour_start=" + String(behaviours.get(i)->get_label()));
                behaviours.get(i)->save_sequence_add_lines(lines);
                lines->add("behaviour_end=" + String(behaviours.get(i)->get_label()));
            }
        }

        void reset_all_mappings() {
            for (int i = 0 ; i < behaviours.size() ; i++) {
                behaviours.get(i)->reset_all_mappings();
            }
        }

    private:
        static DeviceBehaviourManager* inst_;
        DeviceBehaviourManager() {}
        DeviceBehaviourManager(const DeviceBehaviourManager&);
        DeviceBehaviourManager& operator=(const DeviceBehaviourManager&);
};

extern DeviceBehaviourManager *behaviour_manager;

void setup_behaviour_manager();

#endif