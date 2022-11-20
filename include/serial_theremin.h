#ifdef SERIAL_THEREMIN_HACK
/*
#include "USBHost_t36.h"
#include "multi_usb_handlers.h"

#define USBBAUD 115200
uint32_t baud = USBBAUD;
uint32_t format = USBHOST_SERIAL_8N1;

//extern USBHost Usb;

class USBSerialWrapper : public USBSerial {
    public:
        USBSerialWrapper(USBHost &host) : USBSerial(host) {}

        bool beginTransmission(MidiType status) {
            
        }
        void endTransmission() {

        }
};

USBSerialWrapper userial(Usb);

midi::MidiInterface<USBSerialWrapper> midi_in_theremin(userial);

// todo: move all this into a DeviceBehaviour!

void handle_theremin_cc(byte cc_number, byte value, byte channel) {
    Serial.printf("handle_theremin_cc(%i, %i, %i)!\n", cc_number, value, channel);
}

void setup_serials() {
    midi_in_theremin.setHandleControlChange(handle_theremin_cc);
}

void poll_serials() {
    static bool serial_already_initialised = false;
    if (userial && serial_already_initialised==false) {
        Serial.println("userial detected, needs setting up!"); Serial_flush();
        userial.begin(baud, format);
        serial_already_initialised = true;
        Serial.printf("pid=%4x, vid=%4x (", userial.idProduct(), userial.idVendor());
        Serial.printf("%s, %s)\n", userial.manufacturer(), userial.product());
        Serial_flush();
    } else if (!userial) {
        if (serial_already_initialised) {
            Serial.print("userial disconnected!\n"); Serial_flush();
            //userial.end();    // crashes if do this...!
            serial_already_initialised = false;
        }
        Serial.println("userial does not exist!"); Serial_flush();
        return;
    } 
    while(midi_in_theremin.read()) {
        Serial.printf("received a message?\n");
    }
}*/

#endif