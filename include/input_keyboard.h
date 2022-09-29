
#include "Config.h"

#include "storage.h"

extern DisplayTranslator_Configured steensy;

#include "screenshot.h"

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
            case KEYD_UP        : menu->knob_left(); Serial.println("UP"); break;
            case KEYD_DOWN      : menu->knob_right(); Serial.println("DN"); break;
            case KEYD_LEFT      : 
            case KEYD_BACKSPACE :
                Serial.println("LEFT");
                menu->button_back();
                break;
            case KEYD_RIGHT     : 
                Serial.println("RIGHT"); 
            case KEYD_ENTER     :
                menu->button_select();
                Serial.println("selecting"); 
                break;
            case KEYD_HASH      :
                menu->button_right();
                break;
            case 'r'            : restart_on_next_bar = true; break;
            case 'l'            : project.load_selected_sequence(); break;
            case ' '            :
                save_screenshot(&steensy.actual);
                break;
            default:
                Serial.printf("receiving OnPress(%i)\n", key);
        }
    }

    void setup_typing_keyboard() {
        keyboard1.attachPress(OnPress);
    }
#endif