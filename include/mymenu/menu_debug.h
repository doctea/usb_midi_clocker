#include "Config.h"

#include "menu.h"
#include "debug.h"
#include "mymenu/menu_usb.h"
#include "mymenu/menu_behaviours.h"
#include "menuitems_numbers.h"
#include "submenuitem_bar.h"
#include "menuitems_listviewer.h"
#include "menuitems_lambda.h"

#include "storage.h"

#ifdef ENABLE_GATES_MCP23S17
    #include "MCP23S17.h"
#endif
#ifdef ENABLE_CV_INPUT
    #include "ADS1X15.h"
#endif
#ifdef ENABLE_CV_OUTPUT
    #include "cv_output.h"
#endif

#ifdef USE_UCLOCK
    #include "uClock.h"
#endif

#include "__version.h"

#include "ram_stuff.h"

extern bool debug_flag, debug_stress_sequencer_load;

class DebugPanel : public MenuItem {
    public:
        DebugPanel() : MenuItem("Debug") {
            this->selectable = false;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            unsigned long time = millis()/1000;
            tft->setCursor(pos.x,pos.y);

            pos.y = header("Status:", pos, selected, opened);
            tft->printf("  Free RAM: %u bytes\n", freeRam());
            #ifdef ARDUINO_TEENSY41
                tft->printf("  Free extRAM: %u bytes\n", freeExtRam());
                tft->printf("  PSRAM clock: %i MHz\n", psram_clocked_at);
            #endif
            tft->printf("  Uptime: %02uh %02um %02us\n", time/60/60, (time/60)%60, (time)%60);
            tft->printf("  Tick:   %i\n", ticks);
            #ifdef USE_UCLOCK
                tft->printf("  uClock int overflow: %u\n", uClock.getIntOverflowCounter());
                tft->printf("  uClock ext overflow: %u\n", uClock.getExtOverflowCounter());
            #endif
            
            tft->print("\nSerial: ");
            tft->print(Serial?"connected\n":"not connected\n");

            tft->println("\nBuild info:");
            tft->println("  PIO Env: " ENV_NAME);
            tft->println("  Git info: " COMMIT_INFO);
            tft->println("  Built at " __TIME__ " on " __DATE__);

            #ifdef ENABLE_GATES_MCP23S17
                tft->printf("  MCP23S17 version: %s\n", (char*)MCP23S17_LIB_VERSION);
            #endif
            #ifdef ENABLE_CV_INPUT
                tft->printf("  ADS1X15  version: %s", (char*)ADS1X15_LIB_VERSION);
                tft->printf(" @ 0x%2x", ENABLE_CV_INPUT);
                #ifdef ENABLE_CV_INPUT_2
                    tft->printf(",0x%2x\n", ENABLE_CV_INPUT_2);
                #endif
                tft->println();
            #endif
            #ifdef ENABLE_CV_OUTPUT
                // loop over the cvoutput_configs array
                for (size_t i = 0 ; i < cvoutput_configs_size ; i++) {
                    if (i==0) {
                        tft->printf("  DAC8574  version: %s", (char*)DAC8574_LIB_VERSION);
                    } else {
                        tft->print ("\n                          ");
                    }
                    cvoutput_config_t config = cvoutput_configs[i];
                    tft->printf("@ 0x%2x bank %i", config.address, config.dac_extended_address);
                }
                tft->println();
            #endif

            tft->println("\nStats:");
            #ifdef ENABLE_PARAMETERS
                tft->printf("  Parameters: %i\n", parameter_manager->available_parameters->size());
            #endif

            return tft->getCursorY();
        }
};


#ifndef GDB_DEBUG
//FLASHMEM // void setup_debug_menu() causes a section type conflict with void Menu::start()
#endif
void setup_debug_menu() {
    menu->add_page("Behaviours/USB");

    #ifdef ENABLE_USB
        USBDevicesPanel *usbdevices_panel = new USBDevicesPanel();
        menu->add(usbdevices_panel);
    #endif

    BehavioursPanel *behaviours_panel = new BehavioursPanel();
    menu->add(behaviours_panel);

    ////

    menu->add_page("Debug");

    ActionConfirmItem *reset_control = new ActionConfirmItem("RESET TEENSY?", reset_teensy);
    menu->add(reset_control);

    SubMenuItemBar *bar = new SubMenuItemBar("Debug");

    LambdaToggleControl *debug_times_control = new LambdaToggleControl("Render times", [=](bool v) -> void { menu->setDebugTimes(v); }, [=]() -> bool { return menu->isDebugTimes(); }, nullptr);
    bar->add(debug_times_control);
    LambdaToggleControl *profiler_control = new LambdaToggleControl("Profiler", [=](bool v) -> void { menu->setProfileEnable(v); }, [=]() -> bool { return menu->isProfileEnable(); }, nullptr);
    bar->add(profiler_control);

    bar->add(new NumberControl<bool>("Extra", (bool*)&debug_flag, debug_flag, false, true));
    #if defined(ENABLE_CONTROLLER_KEYBOARD) || defined(ENABLE_TYPING_KEYBOARD)
        bar->add(new NumberControl<bool>("InSaNe", (bool*)&debug_stress_sequencer_load, debug_flag, false, true));
    #endif
    menu->add(bar);

    menu->add(new DebugPanel());

    SubMenuItemBar *crashreport_bar = new SubMenuItemBar("CrashReport");
    crashreport_bar->add(new ActionItem("Dump log", storage::dump_crashreport_log));
    crashreport_bar->add(new ActionConfirmItem("Clear log", storage::clear_crashreport_log));
    crashreport_bar->add(new ActionConfirmItem("ForceCrash", storage::force_crash));
    menu->add(crashreport_bar);

    setup_messages_menu();

}