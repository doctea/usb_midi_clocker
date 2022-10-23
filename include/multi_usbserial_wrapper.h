#ifndef USBSERIALWRAPPER__INCLUDED
#define USBSERIALWRAPPER__INCLUDED

#include "USBHost_t36.h"

// wrapper so that we can use a USBSerial connection with the MIDI library
class USBSerialWrapper : public USBSerial {
    public:
        USBSerialWrapper(USBHost &host) : USBSerial(host) {}

        bool beginTransmission(MidiType status) {
            return true;
        }
        void endTransmission() {
        }
        
};

#endif