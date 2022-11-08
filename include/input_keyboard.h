#ifndef KEYBOARD__INCLUDED
#define KEYBOARD__INCLUDED

#include "Config.h"

#include "storage.h"

extern DisplayTranslator_Configured steensy;

#include "screenshot.h"

#include "mymenu.h"

#include "midi/midi_mapper_matrix_manager.h"

void toggle_autoadvance(bool on = false);
void toggle_recall(bool on = false);

bool debug_insane_sequencer_load = false;

#ifdef ENABLE_TYPING_KEYBOARD
    #include "USBHost_t36.h"

    #include "menu.h"

    #define KEYD_BACKSPACE 127
    #define KEYD_ENTER     10
    #define KEYD_HASH      92
    #define MOD_LCTRL      1
    #define MOD_LSHIFT     2
    #define MOD_LALT       4
    #define MOD_WINDOWS    8
    #define MOD_RCTRL      16
    #define MOD_RSHIFT     32
    #define MOD_RALT       64

    extern USBHost Usb;
    extern bool debug;
      
    KeyboardController keyboard1(Usb);

    void OnPress(int key) {
        int modifiers = keyboard1.getModifiers();
        switch(key) {
            /*case KEY_ESC             :  // ESCAPE
                while(menu->is_opened())
                    menu->button_back();
                break;*/
            case KEYD_DELETE    :   // ctrl+alt+delete to reset Teensy
                if (modifiers==(MOD_LCTRL+MOD_LALT) || modifiers==(MOD_RCTRL+MOD_RALT))
                    reset_teensy();  
                break; /* ctrl+alt+delete to soft reboot */
            case KEYD_UP        : Serial.println(F("UP"));             menu->knob_left(); break;
            case KEYD_DOWN      : Serial.println(F("DN"));             menu->knob_right(); break;
            case KEY_ESC        :
            case KEYD_LEFT      : 
            case KEYD_BACKSPACE : Serial.println(F("LEFT"));           menu->button_back(); break;
            case KEYD_RIGHT     : Serial.println(F("RIGHT")); 
            case KEYD_ENTER     : Serial.println(F("selecting"));      menu->button_select(); menu->button_select_released(); break;
            case KEYD_HASH      : Serial.println(F("right-button"));   menu->button_right(); break;
            case 'T':
                debug_insane_sequencer_load = true;
                break;
            case 't':
                debug_insane_sequencer_load = false;
                break;
            case 'D'    :
                debug = true;
                break;
            case 'd'    :
                debug = false;
                break;
            case '-':
                Serial.println(F("------------------------")); break;
            case 'A': case 'a':
                Serial.println(F("Toggling (a)uto-advances"));
                toggle_autoadvance(key=='A');
                break;
            case 'Q': case 'q':
                Serial.println(F("Toggling Re(q)all"));
                toggle_recall(key=='Q');
                break;
            case 'r'            : 
                Serial.println(F("Setting (r)estart_on_next_bar"));
                restart_on_next_bar = true; 
                break;
            case 'L'            : 
                Serial.println(F("(L)oad selected sequence"));
                project.load_selected_sequence(); 
                Serial.println(F("Finished loading selected sequence"));
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
                Serial.println(F("==== Loading previous sequence.."));
                project.load_previous_sequence();
                Serial.println(F("==== Loaded previous sequence!"));
                break;
            case 'j'            :
                Serial.println(F("Select previous sequence"));
                project.select_previous_sequence();
                break;
            case ':'            :
                Serial.println(F("==== Loading next sequence")); Serial.flush();
                project.load_next_sequence();
                Serial.println(F("==== Loaded next sequence!")); Serial.flush();
                break;
            case ';'            :
                Serial.println(F("Select next sequence"));
                project.select_next_sequence();
                break;
            case 'Z'    :
                Serial.clear();
                Serial.clearWriteError();
                Serial.end();
                Serial.begin(115200);
                Serial.setTimeout(0);
                Serial.println("---restarted serial---");
                break;
            case 49 ... 57      :
                {
                    int adjust = 49;
                    if (modifiers & 4) {
                        adjust = 49 - 9;
                        modifiers &= ~4;
                    }
                    if (modifiers==0) {
                        Serial.printf(F("%i pressed -- loading project %i!\n"), key, key - adjust);
                        project.setProjectNumber(key - adjust);
                    } else {
                        Serial.printf(F("Ignoring %i with modifiers %i\n"), key, modifiers);
                    }
                }
                break;
            default:
                Serial.printf(F("received unhandled OnPress(%i/%c) with modifier %i!\n"), key, key, modifiers);
                break;
        }
    }

    #ifndef GDB_DEBUG
    FLASHMEM 
    #endif
    void setup_typing_keyboard() {
        keyboard1.attachPress(OnPress);
    }
#endif

#endif