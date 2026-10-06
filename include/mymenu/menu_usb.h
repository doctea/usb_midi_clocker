
#ifdef ENABLE_USB
    #include "usb/multi_usb_handlers.h"
    #ifdef ENABLE_USBSERIAL
        #include "usb/multi_usbserial_handlers.h"
    #endif
    #if defined(ENABLE_TYPING_KEYBOARD) or defined(ENABLE_CONTROLLER_KEYBOARD)
        //#include "input_keyboard.h"
        #include "USBHost_t36.h"
        extern KeyboardController keyboard1;
    #endif
    class USBDevicesPanel : public MenuItem {
        public:
            USBDevicesPanel() : MenuItem("USB Devices") {}

            virtual int display(Coord pos, bool selected, bool opened) override {
                tft->setCursor(pos.x,pos.y);
                header("MIDI-USB devices:", pos, selected, opened);
                colours(opened);
                tft->setTextSize(1);
                int connected = 0;
                for (unsigned int i = 0 ; i < NUM_USB_MIDI_DEVICES ; i++) {
                    if (usb_midi_slots[i].packed_id && usb_midi_slots[i].device && usb_midi_slots[i].device->idVendor()>0) {
                        connected++;
                        char buf[100];
                        //snprintf(buf, 100, "%i %19s %04X:%04X (%s)\n", i, usb_midi_slots[i].device->product(), usb_midi_slots[i].device->idVendor(), usb_midi_slots[i].device->idProduct(), usb_midi_slots[i].device->serialNumber());
                        snprintf(buf, 100, "%i %19s %04X:%04X\n", i, usb_midi_slots[i].device->product(), usb_midi_slots[i].device->idVendor(), usb_midi_slots[i].device->idProduct());
                        tft->printf(buf);
                    }            
                }
                #ifdef ENABLE_USBSERIAL
                    pos.y = tft->getCursorY();
                    header("Serial-USB devices:", pos, selected, opened);
                    for (unsigned int i = 0 ; i < NUM_USB_SERIAL_DEVICES ; i++) {
                        if (usb_serial_slots[i].packed_id && usb_serial_slots[i].usbdevice && usb_serial_slots[i].usbdevice->idVendor()>0) {
                            connected++;
                            char buf[100];
                            snprintf(buf, 100, "%i %19s\n", i, usb_serial_slots[i].usbdevice->product());
                            tft->printf(buf);
                        }
                    }
                #endif
                #if defined(ENABLE_TYPING_KEYBOARD) or defined(ENABLE_CONTROLLER_KEYBOARD)
                    if (keyboard1)
                        tft->printf("K %19s\n", (char*)keyboard1.product());
                    else
                        tft->printf("K (Typing disconnected)\n");
                #endif
                tft->println();

                return tft->getCursorY();
            }
    };

    class USBHostDebugPanel : public MenuItem {
        public:
            USBHostDebugPanel() : MenuItem("USB host") {
                this->flags.selectable = false;
            }

            virtual int display(Coord pos, bool selected, bool opened) override {
                tft->setCursor(pos.x, pos.y);
                header("USB host:", pos, selected, opened);
                tft->setTextSize(1);
                colours(false);
                uint32_t devices, pipes, transfers, strings;
                USBHost::countFree(devices, pipes, transfers, strings);
                tft->printf("EHCI: %s\n", USBHost::hadSystemError() ? "HALTED" : "no system error");
                if (devices == UINT32_MAX || pipes == UINT32_MAX || transfers == UINT32_MAX || strings == UINT32_MAX) {
                    tft->println("Free pools: CORRUPT");
                } else {
                    tft->printf("Free devices:%lu pipes:%lu\n", (unsigned long)devices, (unsigned long)pipes);
                    tft->printf("Free transfers:%lu strings:%lu\n", (unsigned long)transfers, (unsigned long)strings);
                }
                uint32_t pending = 0, realtime = 0, dropped = 0, realtime_dropped = 0;
                unsigned int missing_queues = 0;
                for (unsigned int port = 0; port < NUM_USB_MIDI_DEVICES; ++port) {
                    const auto *device = usb_midi_slots[port].device;
                    if (device == nullptr) continue;
                    pending += device->getTransmitPendingCount();
                    realtime += device->getRealtimeTransmitPendingCount();
                    dropped += device->getTransmitDropCount();
                    realtime_dropped += device->getRealtimeTransmitDropCount();
                    if (!device->hasTransmitQueueStorage()) ++missing_queues;
                }
                tft->printf("TX queued:%lu RT:%lu\n", (unsigned long)pending, (unsigned long)realtime);
                tft->printf("TX drops:%lu RT:%lu\n", (unsigned long)dropped, (unsigned long)realtime_dropped);
                tft->printf("Missing TX queues:%u\n", missing_queues);
                const auto info = USBHost::getDiagnosticInfo();
                tft->printf("Error records:%lu (%s)\n", (unsigned long)info.count,
                            info.previous_boot ? "last boot" : "this boot");
                tft->println(USBHost::diagnosticErrorName(info.error));
                if (info.count > 0) {
                    tft->printf("At:%lu ms state:%u\n", (unsigned long)info.at_ms, info.state);
                    tft->printf("%04X:%04X address:%u\n", info.vid, info.pid, info.address);
                    tft->printf("Hub:%u port:%u\n", info.hub, info.port);
                    tft->printf("Detail:%08lX\n", (unsigned long)info.detail);
                }
                const auto progress = USBHost::getPreviousDiagnosticProgress();
                tft->printf("Prev: %s\n", USBHost::diagnosticPhaseName(progress.phase));
                tft->printf("At:%lu ms\n", (unsigned long)progress.at_ms);
                return tft->getCursorY();
            }
    };
#endif
