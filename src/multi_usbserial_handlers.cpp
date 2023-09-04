#include "Config.h"

#ifdef ENABLE_USBSERIAL

    #include "bpm.h"
    #include "midi/midi_outs.h"

    #include "multi_usbserial_handlers.h"
    #include "multi_usbserial_wrapper.h"

    #include "tft.h"

    #include "behaviours/behaviour_manager.h"

    extern USBHost Usb;

    USBSerialWrapper userial1(Usb);
    USBSerialWrapper userial2(Usb);
    USBSerialWrapper userial3(Usb);

    usbserial_midi_slot usb_serial_slots[NUM_USB_SERIAL_DEVICES] = {
        { 0x00, 0x00, 0x0000, &userial1, nullptr },
        { 0x00, 0x00, 0x0000, &userial2, nullptr },
        { 0x00, 0x00, 0x0000, &userial3, nullptr }
    };

    // assign device to port and set appropriate handlers
    void setup_usbserial_midi_device(uint8_t idx, uint32_t packed_id = 0x0000) {
        uint16_t vid, pid;
        if (packed_id==0) {
            vid = usb_serial_slots[idx].usbdevice->idVendor();
            pid = usb_serial_slots[idx].usbdevice->idProduct();
            packed_id = (usb_serial_slots[idx].usbdevice->idVendor()<<16) | (usb_serial_slots[idx].usbdevice->idProduct());
        } else {
            vid = packed_id >> 16;
            pid = 0x0000FFFF & packed_id;
        }
        if ((uint32_t)((usb_serial_slots[idx].usbdevice->idVendor()<<16) | (usb_serial_slots[idx].usbdevice->idProduct())) != packed_id) {
            Serial.printf(F("packed_id %08X and newly-generated packed_id %08X don't match already?!"), 
                (usb_serial_slots[idx].usbdevice->idVendor()<<16) | (usb_serial_slots[idx].usbdevice->idProduct()),
                packed_id 
            );
            return;
        }
        Serial.printf(F("USBSerial Port %d changed from %08X to %08X (now "), idx, usb_serial_slots[idx].packed_id, packed_id);
        if (usb_serial_slots[idx].usbdevice!=nullptr)
            Serial.printf(F("'%s' '%s')\n"), usb_serial_slots[idx].usbdevice->manufacturer(), usb_serial_slots[idx].usbdevice->product());
        else 
            Serial.println(F("disconnected)"));
        usb_serial_slots[idx].packed_id = packed_id;

        // remove handlers that might already be set on this port -- new ones assigned below thru xxx_init() functions
        if (usb_serial_slots[idx].behaviour!=nullptr) {
            Serial.printf(F("Disconnecting usbserial_slot %i behaviour\n"), idx);
            usb_serial_slots[idx].behaviour->disconnect_device();
        }

        if (packed_id==0) {
            usb_serial_slots[idx].packed_id = 0;
            Serial.printf(F("Disconnected usbserial device on port %i\n"), idx);
            return;
        }

        // attempt to connect this device to a registered behaviour type
        Serial.printf(F("About to attempt to connect port %d w/ %08x\n"), idx, packed_id);
        if (behaviour_manager->attempt_usbserial_device_connect(idx, packed_id))
            return;

        //usb_midi_connected[idx] = packed_id;
        usb_serial_slots[idx].packed_id = packed_id;

        Serial.printf(F("Detected unknown (or disabled) USB Serial device vid=%04x pid=%04x\n"), vid, pid);
    }


    void update_usbserial_device_connections() {
        for (int port = 0 ; port < NUM_USB_SERIAL_DEVICES ; port++) {
            #ifdef IRQ_PROTECT_USB_CHANGES
                bool irqs_enabled = __irq_enabled();
                __disable_irq();
            #endif
            uint32_t packed_id = (usb_serial_slots[port].usbdevice->idVendor()<<16) | (usb_serial_slots[port].usbdevice->idProduct());
            //Serial.printf("update_usbserial_device_connections(): packed %04X and %04X to %08X\n", usb_serial_slots[port].usbdevice->idVendor(),  usb_serial_slots[port].usbdevice->idProduct(), packed_id);
            if (usb_serial_slots[port].packed_id != packed_id) {
                // device at this port has changed since we last saw it -- ie, disconnection or connection
                // unassign the midi_xxx helper pointers if appropriate
                //usb_serial_slots[port].behaviour = nullptr;
                Serial.printf(F("update_usbserial_device_connections(): device at port %i is %08X which differs from current %08X!\n"), port, packed_id, usb_serial_slots[port].packed_id);
                // call setup_usb_midi_device() to assign device to port and set handlers
                setup_usbserial_midi_device(port, packed_id);
                Serial.println(F("-----"));
            }
            #ifdef IRQ_PROTECT_USB_CHANGES
                if (irqs_enabled) __enable_irq();
            #endif
        }
    }

    void setup_multi_usbserial() {
        // nothing to be done...
        /*Serial.print(F("Arduino initialising usb/midi...")); Serial_flush();

        Usb.begin();
        Serial.println(F("Usb.begin() returned")); Serial_flush();
        for (unsigned int i = 0 ; i < 5 ; i++) {
        Serial.printf(F("%i/5: Waiting 500ms for USB to settle down.."), i+1); Serial_flush();
        tft_print((char*)".");
        delay(500);
        }
        tft_print((char*)"done.\n");
        Serial.println(F("setup_multi_usb() finishing.")); Serial_flush();*/
    }
#endif