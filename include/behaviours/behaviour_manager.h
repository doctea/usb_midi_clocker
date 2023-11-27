#ifndef BEHAVIOUR_MANAGER__INCLUDED
#define BEHAVIOUR_MANAGER__INCLUDED

#include "behaviours/behaviour_base.h"
#include "behaviours/behaviour_base_usb.h"
#include "behaviours/behaviour_base_serial.h"
#include "behaviours/behaviour_base_usbserial.h"

#include "behaviours/behaviour_simplewrapper.h"

#include "usb/multi_usb_handlers.h"

#include "mymenu.h"

#include <LinkedList.h>

#ifdef IRQ_PROTECT_USB_CHANGES
    #include <util/atomic.h>
#endif

class DeviceBehaviourManager {
    public:
        bool debug = false;

        static DeviceBehaviourManager* getInstance();

        // all of the registered behaviours
        LinkedList<DeviceBehaviourUltimateBase *> *behaviours = nullptr;

        // registered behaviours separated by type, so that we can treat them differently for connection and listing purposes
        LinkedList<DeviceBehaviourUSBBase *> *behaviours_usb = nullptr;
        LinkedList<DeviceBehaviourSerialBase *> *behaviours_serial = nullptr;
        LinkedList<DeviceBehaviourUltimateBase *> *behaviours_virtual = nullptr;
        #ifdef ENABLE_USBSERIAL
            LinkedList<DeviceBehaviourUSBSerialBase *> *behaviours_usbserial = nullptr;
        #endif

        void setup_saveable_parameters() {
            for (unsigned int i = 0 ; i < behaviours->size() ; i++) {
                Serial.printf("setup_saveable_parameters for %i: %s\n", i, behaviours->get(i)->get_label());
                behaviours->get(i)->setup_saveable_parameters();
            }
        }

        void registerBehaviour(DeviceBehaviourUSBBase *behaviour) {
            if (behaviour==nullptr) {
                Debug_println(F("registerBehaviour<DeviceBehaviourUSBBase> passed a nullptr!")); Serial_flush();
                return;
            }
            this->behaviours_usb->add(behaviour);
            this->behaviours->add(behaviour);
        }
        void registerBehaviour(DeviceBehaviourSerialBase *behaviour) {
            if (behaviour==nullptr) {
                Debug_println(F("registerBehaviour<DeviceBehaviourSerialBase> passed a nullptr!")); Serial_flush();
                return;
            }
            Debug_printf(F("registerBehaviour<DeviceBehaviourSerialBase> for %ith item passed %p\n"), behaviours->size(), behaviour); Serial_flush();
            this->behaviours_serial->add(behaviour);
            Debug_println(F("Added item to behaviours_serial."));
            this->behaviours->add(behaviour);
            Debug_println(F("Added item to behaviours."));
        }
        #ifdef ENABLE_USBSERIAL
            void registerBehaviour(DeviceBehaviourUSBSerialBase *behaviour) {
                if (behaviour==nullptr) {
                    Debug_println(F("registerBehaviour<DeviceBehaviourUSBBase> passed a nullptr!")); Serial_flush();
                    return;
                }
                this->behaviours_usbserial->add(behaviour);
                this->behaviours->add(behaviour);
            }
        #endif
        void registerBehaviour(DeviceBehaviourUltimateBase *behaviour) {
            if (behaviour==nullptr) {
                Debug_println(F("registerBehaviour<DeviceBehaviourUltimateBase> passed a nullptr!")); Serial_flush();
                return;
            }
            Debug_printf(F("registerBehaviour<DeviceBehaviourUltimateBase> for %ith item passed %p\n"), behaviours->size(), behaviour); Serial_flush();
            this->behaviours_virtual->add(behaviour);
            this->behaviours->add(behaviour);
        }

        #ifdef ENABLE_USB
            bool attempt_usb_device_connect(uint8_t idx, uint32_t packed_id) {
                #ifdef IRQ_PROTECT_USB_CHANGES
                    //bool irqs_enabled = __irq_enabled();
                    //__disable_irq();
                    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                #endif

                // loop over the registered behaviours and if the correct one is found, set it up
                const unsigned int size = behaviours_usb->size();
                for (unsigned int i = 0 ; i < size ; i++) {
                    DeviceBehaviourUSBBase *behaviour = behaviours_usb->get(i);
                    Debug_printf(F("DeviceBehaviourManager#attempt_usb_device_connect(): checking behaviour %i -- does it match %08X?\n"), i, packed_id);
                    usb_midi_slots[idx].packed_id = packed_id;
                    if (behaviour->matches_identifiers(packed_id)) {
                        Debug_printf(F("\tDetected!  Behaviour %i on usb midi idx %i\n"), i, idx); //-- does it match %u?\n", i, packed_id);
                        behaviour->connect_device(usb_midi_slots[idx].device);
                        usb_midi_slots[idx].behaviour = behaviour;
                        //if (irqs_enabled) __enable_irq();
                        return true;
                    }
                }
                Debug_printf(F("Didn't find a behaviour for device #%u with %08X!\n"), idx, packed_id);
                #ifdef IRQ_PROTECT_USB_CHANGES
                    }
                    //if (irqs_enabled) __enable_irq();
                #endif

                return false;
            }
        #endif

        #ifdef ENABLE_USBSERIAL
            bool attempt_usbserial_device_connect(uint8_t idx, uint32_t packed_id) {
                #ifdef IRQ_PROTECT_USB_CHANGES
                    //bool irqs_enabled = __irq_enabled();
                    //__disable_irq();
                    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                #endif
                Serial.printf(F("attempt_usbserial_device_connect(idx=%i, packed_id=%08x)...\n"), idx, packed_id); Serial_flush();
                // loop over the registered behaviours and if the correct one is found, set it up
                const unsigned int size = behaviours_usbserial->size();
                for (unsigned int i = 0 ; i < size ; i++) {
                    if (!usb_serial_slots[idx].usbdevice || usb_serial_slots[idx].packed_id!=packed_id) {
                        Serial.printf(F("WARNING: usb serial device at %i went away!\n"), idx);
                        //if (irqs_enabled) __enable_irq();
                        return false;
                    }
                    DeviceBehaviourUSBSerialBase *behaviour = behaviours_usbserial->get(i);
                    Debug_printf(F("DeviceBehaviourManager#attempt_usbserial_device_connect(): checking behaviour %i -- does it match %08X?\n"), i, packed_id);
                    usb_serial_slots[idx].packed_id = packed_id;
                    if (behaviour->matches_identifiers(packed_id)) {
                        Debug_printf(F("\tDetected!  Behaviour %i on usb serial idx %i\n"), i, idx); //-- does it match %u?\n", i, packed_id);
                        Serial.printf(F("\t\tbehaviour name '%s' w/ id %08X, device product name '%s'?\n"), 
                            behaviour->get_label(), 
                            behaviour->get_packed_id(), 
                            usb_serial_slots[idx].usbdevice->product()
                        );
                        behaviour->connect_device(usb_serial_slots[idx].usbdevice);
                        usb_serial_slots[idx].behaviour = behaviour;
                        //if (irqs_enabled) __enable_irq();
                        return true;
                    }
                }
                #ifdef IRQ_PROTECT_USB_CHANGES
                    }
                    //if (irqs_enabled) __enable_irq();
                #endif
                Serial.printf(F("Didn't find a behaviour for device #%u with %08X (%s)!\n"), idx, packed_id, usb_serial_slots[idx].usbdevice->product()); Serial_flush();
                return false;
            }
        #endif

        void do_reads() {
            const unsigned int size = behaviours->size();
            for (unsigned int i = 0 ; i < size ; i++) {
                //Serial.printf("\tdo_reads on index %i (@%p) about to call read..\n", i, behaviours->get(i)); Serial_flush();
                //Serial.printf("\t\t%s\n", behaviours->get(i)->get_label());
                behaviours->get(i)->read();
                //Serial.printf("\tdo_reads on index %i (@%p) called read..\n", i, behaviours->get(i)); Serial_flush();
            }
        }
        /*#define SINGLE_FRAME_READ
        void read_midi_serial_devices() {
            #ifdef SINGLE_FRAME_READ
                //int i = 0;
                for (unsigned int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
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
            for (unsigned int i = 0 ; i < NUM_USB_MIDI_DEVICES ; i++) {
            while(usb_midi_slots[i].device!=nullptr && usb_midi_slots[i].device->read()); //device->read());
            }
        #else
            #ifdef SINGLE_FRAME_READ_ONCE
            //static int counter;
            for (unsigned int i = 0 ; i < NUM_USB_MIDI_DEVICES ; i++) {
                //while(usb_midi_device[i]->read());
                if (usb_midi_slots[i].device!=nullptr && usb_midi_slots[i].device->read()) {
                //usb_midi_device[counter%NUM_USB_MIDI_DEVICES]->sendNoteOn(random(0,127),random(0,127),random(1,16));
                //Serial.printf("%i: read data from %04x:%04x\n", counter, usb_midi_device[i]->idVendor(), usb_midi_device[i]->idProduct());
                }
                //counter++;
            }
            #else
            static int counter;
            // only all messages from one device per loop
            if (counter>=NUM_USB_MIDI_DEVICES)
                counter = 0;
            while(usb_midi_slots[counter].read());
            counter++;
            #endif
        #endif
        }*/

        void send_clocks() {    // replaces behaviours_send_clock
            const unsigned int size = behaviours->size();
            for (unsigned int i = 0 ; i < size ; i++) {
                //Serial.printf("behaviours#send_clocks calling send_clock on behaviour %i\n", i); Serial_flush();
                behaviours->get(i)->send_clock(ticks);
                //Serial.printf("behaviours#send_clocks called send_clock on behaviour %i\n", i); Serial_flush();
            }  
        }

        void do_phrase(int phrase) {
            const unsigned int size = behaviours->size();
            for (unsigned int i = 0 ; i < size ; i++) {
                behaviours->get(i)->on_phrase(phrase);
            }
        }
        void do_end_phrase(int phrase) {
            const unsigned int size = behaviours->size();
            for (unsigned int i = 0 ; i < size ; i++) {
                behaviours->get(i)->on_end_phrase(phrase);
            }
        }

        void do_end_phrase_pre_clock(int phrase) {
            const unsigned int size = behaviours->size();
            for (unsigned int i = 0 ; i < size ; i++) {
                behaviours->get(i)->on_end_phrase_pre_clock(phrase);
            }
        }

        void do_bar(int bar) {
            const unsigned int size = behaviours->size();
            for (unsigned int i = 0 ; i < size ; i++) {
                behaviours->get(i)->on_bar(bar);
            }
        }
        void do_end_bar(int bar) {
            const unsigned int size = behaviours->size();
            for (unsigned int i = 0 ; i < size ; i++) {
                behaviours->get(i)->on_end_bar(bar);
            }
        }

        void do_loops() {       // replaces behaviours_loop
            unsigned long temp_tick;
            //noInterrupts();
            temp_tick = ticks;
            const unsigned int size = behaviours->size();
            for (unsigned int i = 0 ; i < size ; i++) {
                DeviceBehaviourUltimateBase *behaviour = behaviours->get(i);
                if (behaviour!=nullptr) {
                    //Serial.printf("behaviours#do_loops calling loop on behaviour %i\n", i); Serial_flush();
                    behaviour->loop(temp_tick);
                    //Serial.printf("behaviours#do_loops called loop on behaviour %i\n", i); Serial_flush();
                }
            }
        }

        void do_pre_clock(unsigned long in_ticks) {
            const unsigned int size = behaviours->size();
            for (unsigned int i = 0 ; i < size ; i++) {
                Debug_printf("About to on_pre_clock() for behaviour #%i at %p...\n", i, behaviours->get(i));
                if (behaviours->get(i)!=nullptr) {
                    Debug_printf("\t\t(named %s)\n", behaviours->get(i)->get_label()); 
                    Serial_flush();
                    behaviours->get(i)->on_pre_clock(in_ticks);
                    Debug_printf("finished on_pre_clock() for behaviour #%i...\n", i); Serial_flush();
                }
            }
        }

        void do_ticks(unsigned long in_ticks) { // replaces behaviours_do_tick
            const unsigned int size = behaviours->size();
            for (unsigned int i = 0 ; i < size ; i++) {
                //Serial.printf("behaviours#do_ticks calling on_tick on behaviour %i\n", i); Serial_flush();
                if (behaviours->get(i)!=nullptr) {
                    behaviours->get(i)->on_tick(in_ticks);
                }
                //Serial.printf("behaviours#do_ticks called on_tick on behaviour %i\n", i); Serial_flush();
            }
        }

        void on_restart() {
            const unsigned int size = behaviours->size();
            for (unsigned int i = 0 ; i < size ; i++) {
                //Serial.printf("behaviours#on_restart calling on_restart on behaviour %i\n", i); Serial_flush();
                if (behaviours->get(i)!=nullptr) {
                    behaviours->get(i)->on_restart();
                }
                //Serial.printf("behaviours#on_restart called on_restart on behaviour %i\n", i); Serial_flush();
            }
        }

        #ifdef ENABLE_SCREEN
            //FLASHMEM 
            void create_all_behaviour_menu_items(Menu *menu);
            //FLASHMEM
            void create_single_behaviour_menu_items(Menu *menu, DeviceBehaviourUltimateBase *device);
        #endif

        DeviceBehaviourUltimateBase *find_behaviour_for_label(String label) {
            const unsigned int size = this->behaviours->size();
            for (unsigned int i = 0 ; i < size ; i++) {
                DeviceBehaviourUltimateBase *device = this->behaviours->get(i);
                //Serial.printf("find_behaviour_for_label('%s') looping over '%s'\n", label.c_str(), device->get_label());
                if (device!=nullptr && label.equals(device->get_label()))
                    return device;
            }
            //Serial.printf("behaviour_start failed to find a behaviour with label '%s'\n", label.c_str());
            return nullptr;
        }

        bool load_parse_line(String line) {
            line = line.replace('\n',"");
            line = line.replace('\r',"");
            //Serial.printf("\t\tbehaviour_manager#load_parse_line() passed line \"%s\"\n", line.c_str()); Serial_flush();
            String key = line.substring(0, line.indexOf('='));
            String value = line.substring(line.indexOf('=')+1);
            return this->load_parse_key_value(key, value);
        }

        bool load_parse_key_value(String key, String value) {
            static DeviceBehaviourUltimateBase *current_behaviour = nullptr;
            if (key.equals(F("behaviour_start"))) {
                //Serial.printf(F("found behaviour_start for '%s'\n"), value.c_str());
                current_behaviour = this->find_behaviour_for_label(value);
                return true;
            } else if (key.equals(F("behaviour_end"))) {
                //Serial.printf(F("found behaviour_end for '%s'\n"), value.c_str());
                current_behaviour = nullptr;
                return true;
            } else if (current_behaviour!=nullptr && current_behaviour->load_parse_key_value(key, value)) {
                //Serial.printf(F("%s: Succeeded in loading key %s for value '%s'\n"), current_behaviour->get_label(), key.c_str(), value.c_str());
                return true;
            }
            /*Serial.printf(F("behaviour_manager tried processing '%s' => '%s' but: not handled; "), key.c_str(), value.c_str());
            if (current_behaviour==nullptr) Serial.printf("and not a behaviour ");
            Serial.println();*/
            return false;
        }

        // ask each behaviour to add option lines to save project file
        void save_project_add_lines(LinkedList<String> *lines) {
            const unsigned int size = behaviours->size();
            for (unsigned int i = 0 ; i < size ; i++) {
                DeviceBehaviourUltimateBase *device = behaviours->get(i);
                unsigned int lines_before = lines->size();
                device->save_project_add_lines(lines);
                if (lines_before!=lines->size()) {
                    lines->add(lines_before, F("behaviour_start=") + String(device->get_label()));
                    lines->add(F("behaviour_end=") + String(device->get_label()));
                }
            }
        }

        // ask each behaviour to add option lines to save sequence file
        void save_sequence_add_lines(LinkedList<String> *lines) {
            //LinkedList<String> lines = LinkedList<String>();
            const unsigned int size = behaviours->size();
            for (unsigned int i = 0 ; i < size ; i++) {
                //Serial.printf(">>> behaviour_manager#save_sequence_add_lines for behaviour %i aka %s\n", i, behaviours->get(i)->get_label());
                DeviceBehaviourUltimateBase *device = behaviours->get(i);
                unsigned int lines_before = lines->size();
                //Serial.printf("about to save_sequence_add_lines on behaviour.."); Serial_flush();
                device->save_sequence_add_lines(lines);
                //Serial.printf("just did save_sequence_add_lines and got %i items\n", lines->size()); Serial_flush();
                if (lines_before!=lines->size()) {
                    lines->add(lines_before, F("behaviour_start=") + String(device->get_label()));
                    //Serial.printf("\tbehaviour_manager#save_sequence_add_lines calling on behaviour...\n");
                    lines->add(F("behaviour_end=") + String(device->get_label()));
                }
                //Serial.printf("<<< behaviour_manager#save_sequence_add_lines completed behaviour %i aka %s\n", i, behaviours->get(i)->get_label());
            }
        }

        void reset_all_mappings() {
            const unsigned int size = behaviours->size();
            for (unsigned int i = 0 ; i < size ; i++) {
                behaviours->get(i)->reset_all_mappings();
            }
        }

        void kill_all_current_notes() {
            const unsigned int size = behaviours->size();
            for (unsigned int i = 0 ; i < size ; i++) {
                // todo: only kill if the behaviour is quantising
                behaviours->get(i)->killCurrentNote();
            }
        }

    private:
        static DeviceBehaviourManager* inst_;
        DeviceBehaviourManager() {
            #ifdef ENABLE_USB
                this->behaviours_usb = new LinkedList<DeviceBehaviourUSBBase*>();
            #endif
            this->behaviours_serial = new LinkedList<DeviceBehaviourSerialBase*>();
            this->behaviours_virtual = new LinkedList<DeviceBehaviourUltimateBase*>();
            #ifdef ENABLE_USBSERIAL
                this->behaviours_usbserial = new LinkedList<DeviceBehaviourUSBSerialBase*>();
            #endif
            this->behaviours = new LinkedList<DeviceBehaviourUltimateBase*>();
        }
        DeviceBehaviourManager(const DeviceBehaviourManager&);
        DeviceBehaviourManager& operator=(const DeviceBehaviourManager&);
};

extern DeviceBehaviourManager *behaviour_manager;

void setup_behaviour_manager();

#endif