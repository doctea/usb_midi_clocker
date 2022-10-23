#include "Config.h"
#include <Arduino.h>
#include "USBHost_t36.h"
#include "behaviour_base_usbserial.h"

void handle_theremin_control_change(byte cc_number, byte value, byte channel);
void handle_theremin_note_on(byte cc_number, byte value, byte channel);
void handle_theremin_note_off(byte cc_number, byte value, byte channel);

class DeviceBehaviour_OpenTheremin : public DeviceBehaviourUSBSerialBase {
    public:

        uint16_t vid = 0x1c75, pid = 0x0288;
        virtual uint32_t get_packed_id () override { return (this->vid<<16 | this->pid); }

        void setup_callbacks() {
            this->input_interface->setHandleControlChange(handle_theremin_control_change);
            this->input_interface->setHandleNoteOn(handle_theremin_note_on);
            this->input_interface->setHandleNoteOn(handle_theremin_note_off);
        }
};

extern DeviceBehaviour_OpenTheremin *behaviour_opentheremin;