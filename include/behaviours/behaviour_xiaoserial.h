#include "Config.h"

#if defined(ENABLE_MICROLIDIAN) && defined(ENABLE_XIAOSERIAL) && defined(ENABLE_USBSERIAL)

#include <Arduino.h>
#include "USBHost_t36.h"
#include "behaviour_base_usbserial.h"

/*void handle_theremin_control_change(byte channel, byte cc_number, byte value);
void handle_theremin_note_on(byte channel, byte cc_number, byte value);
void handle_theremin_note_off(byte channel, byte cc_number, byte value);
void handle_theremin_pitch_bend(byte channel, int bend);*/

class DeviceBehaviour_XiaoSerial : virtual public DeviceBehaviourUSBSerialBase {
    public:
        uint16_t vid = 0x2E8A, pid = 0x000A;
        uint16_t vid2 = 0x1337, pid2 = 0xBEEF;
        virtual uint32_t get_packed_id() override  { return (this->vid<<16 | this->pid); }
        virtual uint32_t get_packed_id2() override { return (this->vid2<<16 | this->pid2); }
        virtual bool matches_identifiers(uint32_t packed_id) {
            return packed_id==get_packed_id() || packed_id==get_packed_id2();
        }

        //virtual bool receives_midi_notes() override   { return true; }
        //virtual bool transmits_midi_notes() override  { return false; }
        
        virtual const char *get_label() {
            return "XiaoSerial";
        }

        FLASHMEM 
        virtual void setup_callbacks() override {
            /*this->midi_interface->setHandleControlChange(handle_theremin_control_change);
            this->midi_interface->setHandleNoteOn(handle_theremin_note_on);
            this->midi_interface->setHandleNoteOff(handle_theremin_note_off);
            this->midi_interface->setHandlePitchBend(handle_theremin_pitch_bend);*/
        }
};

extern DeviceBehaviour_XiaoSerial *behaviour_xiaoserial;

#endif