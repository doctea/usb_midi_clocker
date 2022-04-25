#include "Config.h"
#include "ConfigMidi.h"
#include "bpm.h"
#include "midi_outs.h"

/*
usb_midi_device[0] is 1C75:0288 aka Arturia:Arturia KeyStep 32
usb_midi_device[1] is 2886:800B aka The Tyrell Corporation:Bambleweeny57
usb_midi_device[2] is 1C75:0206 aka Arturia:Arturia BeatStep
usb_midi_device[3] is 09E8:0028 aka AKAI PROFESSIONAL,LP:APC MINI       
usb_midi_device[4] is 09E8:006B aka Akai:Akai MPK49
*/

#define NUM_USB_DEVICES 8

// assign device to port and set appropriate handlers
void setupmidi(uint8_t idx, uint32_t packed_id);
void update_usb_device_connections();
void read_midi_usb_devices();
void loop_midi_usb_devices();
void send_midi_usb_clocks();

void on_restart();
void setup_multi_usb();

#ifdef ENABLE_SCREEN
    #include "tft.h"
    void display_usb_device_list(ST7789_t3 *tft);
#endif
