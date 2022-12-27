#ifndef KEYBOARD__INCLUDED
#define KEYBOARD__INCLUDED

#include "Config.h"

#include "storage.h"

extern DisplayTranslator_Configured steensy;

#include "screenshot.h"

#include "mymenu.h"

#include "midi/midi_mapper_matrix_manager.h"

#include "arrangement/arrangement.h"
#include "project.h"

void toggle_autoadvance(bool on = false);
void toggle_recall(bool on = false);

bool debug_stress_sequencer_load = false;

#ifdef ENABLE_TYPING_KEYBOARD
    #include "USBHost_t36.h"

    #include "menu.h"

    #define KEYREPEAT      200   // ms to repeat when a key is held

    // todo: convert to using raw keycodes, since these fuck up when modifiers are held
    #define KEYD_TAB       9
    #define KEYD_ENTER     10
    #define KEYD_ESC       27
    #define KEYD_HASH      92
    #define KEYD_BACKSPACE 127

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

    #include "queue.h"

    struct keypress_t {
        int key = 0;
        byte modifiers = 0;
    };

    Queue<keypress_t, 10> *keyboard_queue = new Queue<keypress_t, 10> ();

    // for avoiding re-entrant interrupts
    volatile bool already_pressing = false;

    // for handling keyrepeat
    keypress_t currently_held;
    volatile bool held = false;
    uint32_t last_retriggered = 0;  

    void OnPress(int key) {
        if (already_pressing) return;
        already_pressing = true;
        bool irqs_enabled = __irq_enabled();
        __disable_irq();
        
        // there is some stuff that we want to do (reboot, enable debug mode, reset serial monitor) even if
        // the main loop has crashed
        switch(key) {
            /*case KEY_ESC             :  // ESCAPE
                while(menu->is_opened())
                    menu->button_back();
                break;*/
            case 'D'    :
                debug = true;
                break;
            case 'd'    :
                debug = false;
                break;
            // debug
            case 'Z'    :
                Serial.clear();
                Serial.clearWriteError();
                Serial.end();
                Serial.begin(115200);
                Serial.setTimeout(0);
                Serial.println(F("---restarted serial---"));
                break;
            case KEYD_DELETE    :   // ctrl+alt+delete to reset Teensy
                {
                    int modifiers = keyboard1.getModifiers();
                    if (modifiers==(MOD_LCTRL+MOD_LALT+MOD_LSHIFT)) {
                        Serial.println("running a loop() manually because ctrl+alt+lshift+delete");
                        loop();
                        break;
                    }                    
                    if (modifiers==(MOD_LCTRL+MOD_LALT) || modifiers==(MOD_RCTRL+MOD_RALT)) {
                        reset_teensy();  
                        break;
                    }
                }
            default:
                //Serial.println("received key?");
                keyboard_queue->push({key, keyboard1.getModifiers()});

                currently_held.key = key;
                currently_held.modifiers = keyboard1.getModifiers();
                held = true;
                last_retriggered = millis();
                break;
        }

        already_pressing = false;
        if (irqs_enabled) 
            __enable_irq();
    }

    /*void OnRawPress(uint8_t keycode) {
        Serial.printf("OnRawPress with keycode %i (%c), modifiers %i\n", keycode, keycode, keyboard1.getModifiers());
    }*/

    void OnRelease(int key) {
        held = false;
    }

    void process_key(int key, int modifiers) {
        //int modifiers = keyboard1.getModifiers();
        //bool irqs_enabled = __irq_enabled();
        //__disable_irq();
        switch(key) {
            /*case KEY_ESC             :  // ESCAPE
                while(menu->is_opened())
                    menu->button_back();
                break;*/
            case KEYD_UP        : Serial.println(F("UP"));             menu->knob_left(); break;
            case KEYD_DOWN      : Serial.println(F("DN"));             menu->knob_right(); break;
            case KEYD_ESC        :
            case KEYD_LEFT      : 
            case KEYD_BACKSPACE : Serial.println(F("LEFT"));           menu->button_back(); break;
            case KEYD_RIGHT     : Serial.println(F("RIGHT")); 
            case KEYD_ENTER     : Serial.println(F("selecting"));      menu->button_select(); menu->button_select_released(); break;
            case KEYD_HASH      : Serial.println(F("right-button"));   menu->button_right(); break;
            case KEYD_TAB       :
                // switch menu page
                if (modifiers & MOD_LSHIFT || modifiers & MOD_RSHIFT) {
                    // go backwards
                    menu->select_previous_page();
                } else {
                    // go forwards
                    menu->select_next_page();
                }
                break;
            case 'T':
                debug_stress_sequencer_load = true;
                break;
            case 't':
                debug_stress_sequencer_load = false;
                break;
            case '-':
                Serial.println(F("------------------------")); break;
            case 'p'            : case 'P':
                Serial.println(F("MIDI (p)ANIC AT THE DISCO"));
                midi_matrix_manager->stop_all_notes();
                break;
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
            // take screenshot
            case ' '            :
                Serial.println(F("Taking screenshot!"));
                save_screenshot(&steensy.actual);
                break;
            // load/save/move selected sequence
            case 'L'            : 
                Serial.println(F("(L)oad selected sequence"));
                project->load_selected_sequence();
                Serial.println(F("Finished loading selected sequence"));
                break;
            case 'S'            :
                Serial.println(F("(S)ave sequencer!"));
                project->save_selected_sequence();
                break;
            case 'J'            :
                Serial.println(F("==== Loading previous sequence.."));
                //input_keyboard.queue_load_previous_sequence();
                project->load_previous_sequence();
                Serial.println(F("==== Loaded previous sequence!"));
                break;
            case ':'            :
                Serial.println(F("==== Loading next sequence")); Serial_flush();
                //input_keyboard.queue_load_next_sequence();
                project->load_next_sequence();
                Serial.println(F("==== Loaded next sequence!")); Serial_flush();
                break;
            case 'j'            :
                //Serial.println(F("Select previous sequence"));
                project->select_previous_sequence();
                break;
            case ';'            :
                //Serial.println(F("Select next sequence"));
                project->select_next_sequence();
                break;
            case 'E'    :
                arrangement->debug_arrangement();
                break;
            // change project number
            case 49 ... 57      :
                {
                    int adjust = 49;
                    if (modifiers & 4) {
                        adjust = 49 - 9;
                        modifiers &= ~MOD_LALT;
                    }
                    if (modifiers==0) {
                        Serial.printf(F("%i pressed -- loading project %i!\n"), key, key - adjust);
                        //input_keyboard.queue_setProjectNumber(key - adjust);
                        project->setProjectNumber(key - adjust);
                    } else {
                        Serial.printf(F("Ignoring %i with modifiers %i\n"), key, modifiers);
                    }
                }
                break;
            default:
                Serial.printf(F("received unhandled OnPress(%i/%c) with modifier %i!\n"), key, key, modifiers);
                break;
        }
        //if (irqs_enabled) __enable_irq();
    }

    void process_key_buffer() {
        bool irqs_enabled = __irq_enabled();
        __disable_irq();

        if (keyboard_queue->isReady()) {
            keypress_t *current = keyboard_queue->pop();
            process_key(current->key, current->modifiers);       
        }
        if (held && millis() - last_retriggered>KEYREPEAT) {
            keyboard_queue->push(currently_held);
            last_retriggered = millis();
        }
        if (irqs_enabled) __enable_irq();
    }

    #ifndef GDB_DEBUG
    FLASHMEM 
    #endif
    void setup_typing_keyboard() {
        keyboard1.attachPress(OnPress);
        //keyboard1.attachRawPress(OnRawPress);
        keyboard1.attachRelease(OnRelease);
    }
#endif

#endif
