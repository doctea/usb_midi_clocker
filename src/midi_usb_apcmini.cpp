#include "midi_usb_apcmini.h"

#define ETL_NO_STL
#include "etl/delegate.h"
///using namespace etl;

using ptr_cc        = etl::delegate<void(uint8_t,uint8_t,uint8_t)>;
using ptr_note_on   = etl::delegate<void(uint8_t,uint8_t,uint8_t)>;
using ptr_note_off  = etl::delegate<void(uint8_t,uint8_t,uint8_t)>;

void USBDevice_APCMini::setup_callbacks() {
            ptr_cc dcc = ptr_cc::create<USBDevice_APCMini, this, USBDevice_APCMini::control_change>();
            //ptr_cc dcc = ptr_cc::create<USBDevice_APCMini>();

            //ptr_cc dcc      = ptr_cc::create<USBDevice_APCMini, this, &USBDevice_APCMini::control_change>();
            //ptr_cc dnon     = ptr_cc::create<USBDevice_APCMini, this, &USBDevice_APCMini::note_on>();
            //ptr_cc dnoff    = ptr_cc::create<USBDevice_APCMini, this, &USBDevice_APCMini::note_off>();

            //etl::delegate<void(uint8_t,uint8_t,uint8_t)> dcc = 
            //    etl::delegate<void(uint8_t,uint8_t,uint8_t)>::create<USBDevice_APCMini, &this, &USBDevice_APCMini::control_change>();

            this->device->setHandleControlChange(&dcc);

}
