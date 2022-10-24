#include "Config.h"

#ifdef ENABLE_OPENTHEREMIN

#include <Arduino.h>
#include "USBHost_t36.h"
#include "behaviour_base_usbserial.h"

void handle_theremin_control_change(byte channel, byte cc_number, byte value);
void handle_theremin_note_on(byte channel, byte cc_number, byte value);
void handle_theremin_note_off(byte channel, byte cc_number, byte value);

class DeviceBehaviour_OpenTheremin : public DeviceBehaviourUSBSerialMIDIBase {
    public:
        uint16_t vid = 0x1a86, pid = 0x7523;            // 1a867523
        virtual uint32_t get_packed_id () override { return (this->vid<<16 | this->pid); }
        
        virtual const char *get_label() {
            return (char*)"OpenTheremin";
        }

        void setup_callbacks() override {
            this->midi_interface->setHandleControlChange(handle_theremin_control_change);
            this->midi_interface->setHandleNoteOn(handle_theremin_note_on);
            this->midi_interface->setHandleNoteOff(handle_theremin_note_off);
        }
};

extern DeviceBehaviour_OpenTheremin *behaviour_opentheremin;

#endif