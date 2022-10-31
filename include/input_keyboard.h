
#include "Config.h"

#include "storage.h"

extern DisplayTranslator_Configured steensy;

#include "screenshot.h"

#include "midi/midi_mapper_matrix_manager.h"

#ifdef ENABLE_TYPING_KEYBOARD
    #include "USBHost_t36.h"

    #include "menu.h"

    #define KEYD_BACKSPACE 127
    #define KEYD_ENTER     10
    #define KEYD_HASH      92

    extern USBHost Usb;

    KeyboardController keyboard1(Usb);

    void OnPress(int key) {
        int modifiers = keyboard1.getModifiers();
        switch(key) {
            case KEYD_DELETE    :   // ctrl+alt+delete to reset Teensy
                if (modifiers==5)  // ctrl + alt
                    reset_teensy();  
                break; /* ctrl+alt+delete to soft reboot */
            case KEYD_UP        : Serial.println(F("UP"));     menu->knob_left(); break;
            case KEYD_DOWN      : Serial.println(F("DN"));     menu->knob_right(); break;
            case KEYD_LEFT      : 
            case KEYD_BACKSPACE : Serial.println(F("LEFT"));   menu->button_back(); break;
            case KEYD_RIGHT     : Serial.println(F("RIGHT")); 
            case KEYD_ENTER     : Serial.println(F("selecting"));      menu->button_select(); break;
            case KEYD_HASH      : Serial.println(F("right-button"));   menu->button_right(); break;
            case 'r'            : 
                Serial.println(F("Setting (r)estart_on_next_bar"));
                restart_on_next_bar = true; 
                break;
            case 'L'            : 
                Serial.println(F("(L)oad selected sequence"));
                project.load_selected_sequence(); 
                break;
            case 'S'            :
                Serial.println(F("(S)ave sequencer!"));
                project.save_selected_sequence();
                break;
            case 'p'            :
                Serial.println(F("MIDI (p)ANIC AT THE DISCO"));
                midi_matrix_manager->stop_all_notes();
                break;
            case ' '            :
                Serial.println(F("Taking screenshot!"));
                save_screenshot(&steensy.actual);
                break;
            case 'J'            :
                Serial.println(F("Load previous sequence"));
                project.load_previous_sequence();
                break;
            case 'j'            :
                Serial.println(F("Select previous sequence"));
                project.select_previous_sequence();
                break;
            case ':'            :
                Serial.println(F("Load next sequence"));
                project.load_next_sequence();
                break;
            case ';'            :
                Serial.println(F("Select next sequence"));
                project.select_next_sequence();
                break;
            case 49 ... 57      :
                {
                    int adjust = 49;
                    if (modifiers & 4) {
                        adjust = 49 - 9;
                        modifiers &= ~4;
                    }
                    if (modifiers==0) {
                        Serial.printf("%i pressed -- loading project %i!\n", key, key - adjust);
                        project.setProjectNumber(key - adjust);
                    } else {
                        Serial.printf("Ignoring %i with modifiers %i\n", key, modifiers);
                    }
                }
                break;
            default:
                Serial.printf(F("received unhandled OnPress(%i/%c) with modifier %i!\n"), key, key, modifiers);
                break;
        }
    }

    FLASHMEM void setup_typing_keyboard() {
        keyboard1.attachPress(OnPress);
    }
#endif
