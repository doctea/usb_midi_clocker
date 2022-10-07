
#include "Config.h"

#include "storage.h"

extern DisplayTranslator_Configured steensy;

#include "screenshot.h"

void reset_teensy() {
    // https://forum.pjrc.com/threads/57810-Soft-reboot-on-Teensy4-0
    #define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
    #define CPU_RESTART_VAL 0x5FA0004
    #define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);
    Serial.println("Restarting!\n"); Serial.flush();
    CPU_RESTART;
    Serial.println("Restarted?!"); Serial.flush();
}


#ifdef ENABLE_TYPING_KEYBOARD
    #include "USBHost_t36.h"

    #include "menu.h"

    #define KEYD_BACKSPACE 127
    #define KEYD_ENTER     10
    #define KEYD_HASH      92

    extern USBHost Usb;

    KeyboardController keyboard1(Usb);

    void OnPress(int key) {
        switch(key) {
            case KEYD_DELETE    : 
                if (keyboard1.getModifiers()==5)  
                    reset_teensy();  
                break; /* ctrl+alt+delete to soft reboot */
            case KEYD_UP        : Serial.println("UP");     menu->knob_left(); break;
            case KEYD_DOWN      : Serial.println("DN");     menu->knob_right(); break;
            case KEYD_LEFT      : 
            case KEYD_BACKSPACE : Serial.println("LEFT");   menu->button_back(); break;
            case KEYD_RIGHT     : Serial.println("RIGHT"); 
            case KEYD_ENTER     : Serial.println("selecting");      menu->button_select(); break;
            case KEYD_HASH      : Serial.println("right-button");   menu->button_right(); break;
            case 'r'            : 
                Serial.println("setting restart_on_next_bar");
                restart_on_next_bar = true; 
                break;
            case 'l'            : 
                Serial.println("loading selected sequence");
                project.load_selected_sequence(); 
                break;
            case ' '            :
                Serial.println("Saving screenshot!");
                save_screenshot(&steensy.actual);
                break;
            default:
                Serial.printf("received unhandled OnPress(%i) with modifier %i!\n", key, keyboard1.getModifiers());
        }
    }

    void setup_typing_keyboard() {
        keyboard1.attachPress(OnPress);
    }
#endif