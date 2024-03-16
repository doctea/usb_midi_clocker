#ifndef BEHAVIOUR_BASE_USB__INCLUDED
#define BEHAVIOUR_BASE_USB__INCLUDED

#include "behaviours/behaviour_base.h"

#include "parameters/Parameter.h"

#include <util/atomic.h>

class DeviceBehaviourUSBBase : virtual public DeviceBehaviourUltimateBase {
    public:
        //const uint32_t vid = 0x0000, pid = 0x0000;
        MIDIDeviceBase *device = nullptr;

        DeviceBehaviourUSBBase() = default;
        virtual ~DeviceBehaviourUSBBase() = default;

        virtual int getType() override {
            return BehaviourType::usb;
        }

        virtual uint32_t get_packed_id() { return 0xFFFFFFFF; }//= 0;///{ return (this->vid<<16 | this->pid); }

        // interface methods - static
        /*virtual bool matches_identifiers(uint16_t vid, uint16_t pid) {
            return vid==this->vid && pid==this->pid;
        }*/
        virtual bool matches_identifiers(uint32_t packed_id) {
            return (this->get_packed_id()==packed_id);
        }

        virtual bool is_connected() override {
            return this->device!=nullptr;
        }

        //FLASHMEM
        virtual void connect_device(MIDIDeviceBase *device) {
            //if (!is_connected()) return;

            //Serial.printf("DeviceBehaviourUSBBase#connected_device connecting %p\n", device);
            this->device = device;
            this->init();
            this->setup_callbacks();
        }

        // remove handlers that might already be set on this port -- new ones assigned thru setup_callbacks functions
        //FLASHMEM
        virtual void disconnect_device() {
            //if (this->device==nullptr) return;
            if (!is_connected()) return;
            
            this->device->setHandleNoteOn(nullptr);
            this->device->setHandleNoteOff(nullptr);
            this->device->setHandleControlChange(nullptr);
            this->device->setHandleClock(nullptr);
            this->device->setHandleStart(nullptr);
            this->device->setHandleStop(nullptr);
            this->device->setHandleSystemExclusive((void (*)(uint8_t *, unsigned int))nullptr);
            this->device->setHandlePitchChange(nullptr);
            this->device = nullptr;
        }

        virtual void read() override {
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                if (!is_connected()) return;

                if (this->device!=nullptr) while(this->device->read()); 
            }
        };

        virtual void actualSendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
            if (!is_connected()) return;
            if (debug) Serial.printf("USB#%s#actualSendNoteOn(%i, %i, %i)\n", this->get_label(), note, velocity, channel);
            this->device->sendNoteOn(note, velocity, channel);
        };
        virtual void actualSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
            if (!is_connected()) return;
            this->device->sendNoteOff(note, velocity, channel);
        };
        virtual void actualSendControlChange(uint8_t number, uint8_t value, uint8_t channel) override {
            if (!is_connected()) return;
            /*
            WEIRD FREEZE...

            - when sending MIDI CC from main loop, from update_mixers, to a MIDIDeviceBase
            - via actualSendControlChange in behaviour_base_usb.h
            - no crash so long as:-
            - sending serial text
            - and serial.flush
            - and serial monitor is connected
            - uClock disabled..
            - only if ticking..
            - only if CraftSynth is actually connected
            */
            //return; // <-- tryna figure out why getting crash ..
            //Serial.printf("%s\t#actualSendControlChange(%3i, %3i, %2i)\n", this->get_label(), number, value, channel);
            //Serial.flush();
            //delay(1);
            this->device->sendControlChange(number, value, channel);
            //this->device->send_now();
        };
        virtual void actualSendRealTime(uint8_t message) override {
            if (!is_connected()) return;
            this->device->sendRealTime(message);
        };
        virtual void actualSendPitchBend(int16_t bend, uint8_t channel = 0) {
            if (!is_connected() || this->device==nullptr) return;
            this->device->sendPitchBend(bend, channel);
        }
        
        #ifdef ENABLE_SCREEN
            //FLASHMEM
            /*virtual LinkedList<MenuItem*> *make_menu_items_device() override {
                // todo: show a 'connected/not connected' indicator
                return this->menuitems;
            }*/

            virtual LinkedList<MenuItem*> *make_menu_items_device();
        #endif
};


#endif