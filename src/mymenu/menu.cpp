#include "Config.h"
#include "storage.h"

#ifdef ENABLE_SCREEN

#include "mymenu.h"

#include "submenuitem.h"

#include "mymenu/menu_conductor.h"
#include "mymenu/menu_looper.h"
#include "mymenu/menu_midi_matrix.h"
#include "mymenu/menu_project.h"
#include "mymenu/menu_sequencer.h"
#include "mymenu/menu_taptempo.h"

#include "mymenu/menu_taptempo.h"

#include "menuitems_pinned.h"

#include "menuitems_object_multitoggle.h"

#include "mymenu/menu_usb.h"
#include "mymenu/menu_behaviours.h"

#include "mymenu/menu_uclockdebug.h"

#include "submenuitem_bar.h"

#include "behaviours/behaviour_beatstep.h"
#include "behaviours/behaviour_keystep.h"
#include "behaviours/behaviour_mpk49.h"
#include "behaviours/behaviour_subclocker.h"
#include "behaviours/behaviour_craftsynth.h"
#include "behaviours/behaviour_neutron.h"
#include "behaviours/behaviour_progression.h"
#include "behaviours/behaviour_midilooper.h"

#include "midi/midi_out_wrapper.h"
#include "midi/midi_outs.h"
#include "midi/midi_pc_usb.h"

#include "behaviours/behaviour_subclocker.h"

#include "clock.h"

#include "profiler.h"

#include "menu_io.h"

#ifdef ENABLE_SHUFFLE
    #include "sequencer/shuffle.h"
#endif

//DisplayTranslator *tft;
#ifdef TFT_ST7789_T3_BIG
    DisplayTranslator_STeensy_Big *tft;
#elif defined(TFT_ILI9341_T3N)
    DisplayTranslator_ILI9341_T3N *tft;
#elif defined(TFT_BODMER)
    DisplayTranslator_Bodmer *tft;
#endif
Menu *menu; // = Menu();

#ifdef ENCODER_KNOB_L
    Encoder knob(ENCODER_KNOB_L, ENCODER_KNOB_R);
    //extern Encoder knob;
#endif
#ifdef PIN_BUTTON_A
    Button pushButtonA = Button(); // 10ms debounce
    //extern Bounce pushButtonA;
#endif
#ifdef PIN_BUTTON_B
    Button pushButtonB = Button(); // 10ms debounce
    //extern Bounce pushButtonB; 
#endif
#ifdef PIN_BUTTON_C
    Button pushButtonC = Button(); // 10ms debounce
    //extern Bounce pushButtonC;
#endif

#ifdef ENABLE_DRUM_LOOPER
    LooperStatus            drum_looper_status  =   LooperStatus("Drum looper", &drums_loop_track);
    LooperQuantizeControl   drum_loop_quantizer_setting = LooperQuantizeControl("Drum Loop quant",   &drums_loop_track);   // todo: make this part of the LooperStatus object
#endif

#if defined(ENABLE_CRAFTSYNTH_USB) && defined(ENABLE_CRAFTSYNTH_CLOCKTOGGLE)
    LambdaToggleControl craftsynth_clock_toggle = LambdaToggleControl (
        "CraftSynth clock enable",
        [=](bool v) -> void { behaviour_craftsynth->setClockEnabled(v); },
        [=]() -> bool { return behaviour_craftsynth->isClockEnabled(); },
        nullptr
    );
#endif

/*MenuItem test_item_1 = MenuItem("test 1");
MenuItem test_item_2 = MenuItem("test 2");
MenuItem test_item_3 = MenuItem("test 3");*/

//DisplayTranslator_STeensy display_translator = DisplayTranslator_STeensy();
//DisplayTranslator_STeensy_Big display_translator = DisplayTranslator_STeensy_Big();
DisplayTranslator_Configured display_translator = DisplayTranslator_Configured();

//#include "menuitems_numbers.h"
//int8_t shuffle_data = 0;

// add Conductor menus
void setup_menu_transport() {
    conductor->make_menu_items(menu, COMBINE_NONE);
        
    int current_page = menu->get_selected_page_index();

    menu->select_page(current_page);
}

#ifdef ENABLE_TAPTEMPO

    TapTempoControl *tapper_control = nullptr;
    extern TapTempoTracker *tapper;

    void setup_menu_taptempo() {
        tapper_control = new TapTempoControl("Tap tempo", tapper);
        // go back a page and add this selector at the end so that it's in the main page clock menu
        // todo: but should probably put this on its own page or something somewhere?
        int current_page = menu->get_selected_page_index();
        menu->select_page_by_name("Main");
        menu->add(tapper_control);   
        menu->select_page(current_page);
    }
#endif

#ifdef ENABLE_LOOPER
    void setup_menu_looper() {
        // looper stuff
        menu->add_page("Looper", C_WHITE, true, "Project");  // should this be somewhere else? isnt it already handled as a behaviour..?
        menu->add(behaviour_midilooper->make_menu_items());
        #ifdef ENABLE_DRUM_LOOPER
            menu->add(&drum_looper_status);
            menu->add(&drum_loop_quantizer_setting);
        #endif
    }
#endif

#ifndef GDB_DEBUG
//FLASHMEM // causes a section type conflict with 'void Menu::add(MenuItemList*, uint16_t)'
#endif
void setup_menu(bool button_high_state) {
    Serial.println(F("Starting setup_menu()..")); //Instantiating DisplayTranslator_STeensy.."));

    #ifdef PIN_BUTTON_A
        pushButtonA.setPressedState(button_high_state);
        pushButtonA.attach(PIN_BUTTON_A, INPUT_PULLUP);
        pushButtonA.interval(10);
    #endif
    #ifdef PIN_BUTTON_B
        pushButtonB.setPressedState(button_high_state);
        pushButtonB.attach(PIN_BUTTON_B, INPUT_PULLUP);
        pushButtonB.interval(10);
    #endif
    #ifdef PIN_BUTTON_C
        pushButtonC.setPressedState(button_high_state);
        pushButtonC.attach(PIN_BUTTON_C, INPUT_PULLUP);
        pushButtonC.interval(10);
    #endif

    tft = &display_translator; //DisplayTranslator_STeensy();
    #ifdef TFT_BODMER
        tft->init();
    #endif

    //delay(50);
    //Serial.println(F("Finished  constructor"));
    Serial_flush();
    Serial.println(F("Creating Menu object.."));
    Serial_flush();
    menu = new Menu(tft, button_high_state);
    Serial.println(F("Created Menu object"));
    Serial_flush();

    menu->set_messages_log(&message_log);

    menu->add_pinned(new LoopMarkerPanel(LOOP_LENGTH_TICKS, PPQN));  // pinned position indicator

    setup_menu_transport();

    #ifdef ENABLE_TAPTEMPO
        setup_menu_taptempo();
    #endif

    #if !SAFE_DISABLE_MATRIX_UI_BOOT
        setup_menu_midi();
    #endif
    
    setup_menu_project();
    
    #if defined(ENABLE_CLOCKS) || defined(ENABLE_SEQUENCER)
        setup_menu_sequencer();
    #endif

    #ifdef ENABLE_SHUFFLE
        setup_menu_shuffle();
    #endif

    #ifdef ENABLE_CV_GATE_OUTPUT
        setup_gate_manager_menus();
    #endif

    #ifdef ENABLE_LOOPER
        setup_menu_looper();
    #endif

    Serial.println(F("Exiting setup_menu"));
    Serial_flush();

    /*menu->add(&test_item_1);
    menu->add(&test_item_2);
    menu->add(&test_item_3);*/
}

#endif