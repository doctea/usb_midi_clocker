
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
                Serial.println(F("setting restart_on_next_bar"));
                restart_on_next_bar = true; 
                break;
            case 'l'            : 
                Serial.println(F("loading selected sequence"));
                project.load_selected_sequence(); 
                break;
            case 'p'            :
                Serial.println(F("MIDI PANIC AT THE DISCO"));
                midi_matrix_manager->stop_all_notes();
                break;
            case ' '            :
                Serial.println(F("Saving screenshot!"));
                save_screenshot(&steensy.actual);
                break;
            case 'j'            :
                /*int seq_number = project.get_loaded_sequence_number()-1;
                if (seq_number<0) seq_number = NUM_SEQUENCE_SLOTS_PER_PROJECT-1;
                project.load_sequence(seq_number);*/
                project.previous_sequence();
                break;
            case ';'            :
                project.next_sequence();
                break;
            case 49 ... 57      :
                int adjust = 49;
                if (modifiers & 4)
                    adjust = 49 + 9;
                Serial.printf("%i pressed, loading project %i!\n", key - adjust);
                project.load_project_settings(key - adjust);
                break;
            default:
                Serial.printf(F("received unhandled OnPress(%i/%c) with modifier %i!\n"), key, key, keyboard1.getModifiers());
        }
    }

    FLASHMEM void setup_typing_keyboard() {
        keyboard1.attachPress(OnPress);
    }
#endif