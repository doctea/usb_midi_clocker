#include "midi_device_usb_handler.h"

#include "midi_apcmini.h"
#include "midi_bamble.h"
#include "midi_beatstep.h"
#include "midi_keystep.h"
#include "midi_mpk49.h"
#include "midi_subclocker.h"

USBMidiDeviceManager* usb_manager;

void setup_usb_device_handler() {
    usb_manager = USBMidiDeviceManager::getInstance();

    usb_manager->registerDevice(new USBDevice_APCMini());
    usb_manager->registerDevice(new USBDevice_Bamble());
    usb_manager->registerDevice(new USBDevice_Beatstep());
    usb_manager->registerDevice(new USBDevice_Keystep());
    usb_manager->registerDevice(new USBDevice_MPK49());
    usb_manager->registerDevice(new USBDevice_Subclocker());
}