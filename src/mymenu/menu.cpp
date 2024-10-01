#include "Config.h"
#include "storage.h"

#ifdef ENABLE_SCREEN

#include "mymenu.h"

#include "submenuitem.h"

#include "mymenu/menu_looper.h"
#include "mymenu/menu_sequencer.h"
#include "mymenu/menu_bpm.h"
#include "mymenu/menu_clock_source.h"
#include "mymenu/menu_midi_matrix.h"
#include "mymenu/menuitems_scale.h"

#include "menuitems_pinned.h"

#include "menuitems_object_multitoggle.h"

#include "mymenu/menu_usb.h"
#include "mymenu/menu_behaviours.h"

#include "submenuitem_bar.h"

#include "behaviours/behaviour_beatstep.h"
#include "behaviours/behaviour_keystep.h"
#include "behaviours/behaviour_mpk49.h"
#include "behaviours/behaviour_subclocker.h"
#include "behaviours/behaviour_craftsynth.h"
#include "behaviours/behaviour_neutron.h"

#include "midi/midi_out_wrapper.h"
#include "midi/midi_outs.h"
#include "midi/midi_pc_usb.h"

#include "behaviours/behaviour_subclocker.h"

#include "clock.h"

#include "profiler.h"

#include "menu_io.h"


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

LoopMarkerPanel top_loop_marker_panel = LoopMarkerPanel(LOOP_LENGTH_TICKS, PPQN, BEATS_PER_BAR, BARS_PER_PHRASE);

BPMPositionIndicator posbar = BPMPositionIndicator();
ClockSourceSelectorControl clock_source_selector = ClockSourceSelectorControl("Clock source", clock_mode);

//uint16_t *framebuffers[2];
//bool buffer_number = 0;
/*
void swap_framebuffer() {
    //buffer_number = !buffer_number;
    //tft->tft->setFrameBuffer(framebuffers[buffer_number]);
    tft->framebuffer_ready = true;
    //tft->ready_for_frame = 
}
*/

// make these global so that we can toggle it from input_keyboard
ObjectMultiToggleControl *project_multi_recall_options = nullptr;
ObjectMultiToggleControl *project_multi_autoadvance = nullptr;

#ifdef ENABLE_SEQUENCER
    #include "mymenu/menu_gatedisplay.h"
    #include "mymenu/menu_sequencer_display.h"
    SequencerStatus sequencer_status = SequencerStatus("Pattern");
    TriggerSequencerDisplay trigger_sequencer_display = TriggerSequencerDisplay("Trigger Sequencer");
    ClockSequencerDisplay   clock_sequencer_display   = ClockSequencerDisplay("Clock Sequencer");
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

MidiMatrixSelectorControl midi_matrix_selector = MidiMatrixSelectorControl("MIDI Matrix");

#ifdef ENABLE_SD
    #include "menuitems_fileviewer.h"
    extern FileViewerMenuItem *sequence_fileviewer;
    extern FileViewerMenuItem *project_fileviewer;
#endif

/*MenuItem test_item_1 = MenuItem("test 1");
MenuItem test_item_2 = MenuItem("test 2");
MenuItem test_item_3 = MenuItem("test 3");*/

//DisplayTranslator_STeensy display_translator = DisplayTranslator_STeensy();
//DisplayTranslator_STeensy_Big display_translator = DisplayTranslator_STeensy_Big();
DisplayTranslator_Configured display_translator = DisplayTranslator_Configured();

//#include "menuitems_numbers.h"
//int8_t shuffle_data = 0;

void setup_menu_transport() {
    //menu->add(new SeparatorMenuItem("Transport"));

    menu->add(&posbar);     // bpm and position indicator

    menu->add(&clock_source_selector);  // midi clock source (internal or from PC USB)

    // todo: support midi clock shuffle, when possible to do so..
    //menu->add(new DirectNumberControl<int8_t>("Shuffle", &shuffle_data, 0, (int8_t)0, (int8_t)255));
    /*menu->add(new LambdaToggleControl("Shuffle", 
        [=](bool v) -> void { uClock.setShuffle(v); },   [=]() -> bool { return uClock.isShuffled(); }
    ));
    */

    // add start/stop/continue bar
    SubMenuItemBar *project_startstop = new SubMenuItemBar("Transport", false);
    project_startstop->add(new ActionItem("Start",    clock_start));
    project_startstop->add(new ActionItem("Stop",     clock_stop));
    project_startstop->add(new ActionItem("Continue", clock_continue));
    project_startstop->add(new ActionFeedbackItem("Restart", set_restart_on_next_bar_on, is_restart_on_next_bar, "Restarting..", "Restart"));
    menu->add(project_startstop);
}

#if defined(ENABLE_CLOCKS) || defined(ENABLE_SEQUENCER)
    void setup_menu_project() {
        menu->add_page("Project");

        //menu->add(new SeparatorMenuItem("Project"));

        ActionConfirmItem *project_save = new ActionConfirmItem("Save settings", &save_project_settings);
        LambdaNumberControl<int> *project_selector = new LambdaNumberControl<int>(
            "Project number", 
            [=](int project_number) -> void { project->setProjectNumber(project_number); },
            [=]() -> int { return project->getProjectNumber(); },
            nullptr, 
            0, 
            100
        );

        menu->add(project_save);       // save project settings button
        menu->add(project_selector);   // save project selector button

        // project loading options (whether to load or hold matrix settings, clock, sequence, behaviour options)
        project_multi_recall_options = new ObjectMultiToggleControl("Recall options", true);
        MultiToggleItemClass<Project> *load_matrix = new MultiToggleItemClass<Project> (
            "MIDI Mappings",
            project,
            &Project::setLoadMatrixMappings,
            &Project::isLoadMatrixMappings
        );
        #ifdef ENABLE_CLOCKS
            MultiToggleItemClass<Project> *load_clock = new MultiToggleItemClass<Project> (
                "Clock Settings",
                project,
                &Project::setLoadClockSettings,
                &Project::isLoadClockSettings    
            );
        #endif
        #ifdef ENABLE_SEQUENCER
            MultiToggleItemClass<Project> *load_pattern = new MultiToggleItemClass<Project> (
                "Sequence Settings",
                project,
                &Project::setLoadSequencerSettings,
                &Project::isLoadSequencerSettings    
            );
        #endif
        MultiToggleItemClass<Project> *load_behaviour_settings = new MultiToggleItemClass<Project> {
            "Behaviour Options",
            project,
            &Project::setLoadBehaviourOptions,
            &Project::isLoadBehaviourOptions
        };
        project_multi_recall_options->addItem(load_matrix);
        #ifdef ENABLE_CLOCKS
            project_multi_recall_options->addItem(load_clock);
        #endif
        #ifdef ENABLE_SEQUENCER
            project_multi_recall_options->addItem(load_pattern);
        #endif
        project_multi_recall_options->addItem(load_behaviour_settings);
        //menu->add(&project_load_matrix_mappings);
        menu->add(project_multi_recall_options);

        // options for whether to auto-advance looper/sequencer/beatstep
        project_multi_autoadvance = new ObjectMultiToggleControl("Auto-advance", true);
        #ifdef ENABLE_SEQUENCER
            MultiToggleItemClass<Project> *auto_advance_pattern = new MultiToggleItemClass<Project> (
                "Sequence",
                project,
                &Project::set_auto_advance_pattern,
                &Project::is_auto_advance_pattern
            );
            project_multi_autoadvance->addItem(auto_advance_pattern);
        #endif
        #ifdef ENABLE_LOOPER
            MultiToggleItemClass<Project> *auto_advance_looper = new MultiToggleItemClass<Project> (
                "Looper",
                project,
                &Project::set_auto_advance_looper,
                &Project::is_auto_advance_looper
            );
            project_multi_autoadvance->addItem(auto_advance_looper);
        #endif
        #if defined(ENABLE_BEATSTEP) && defined(ENABLE_BEATSTEP_SYSEX)
            project_multi_autoadvance->addItem(new MultiToggleItemClass<DeviceBehaviour_Beatstep> (
                #ifdef ENABLE_BEATSTEP_2
                    "Beatstep 1",
                #else
                    "Beatstep",
                #endif
                behaviour_beatstep,
                &DeviceBehaviour_Beatstep::set_auto_advance_pattern,
                &DeviceBehaviour_Beatstep::is_auto_advance_pattern
            ));
            #ifdef ENABLE_BEATSTEP_2
                project_multi_autoadvance->addItem(new MultiToggleItemClass<DeviceBehaviour_Beatstep> (
                    "Beatstep 2",
                    behaviour_beatstep_2,
                    &DeviceBehaviour_Beatstep::set_auto_advance_pattern,
                    &DeviceBehaviour_Beatstep::is_auto_advance_pattern
                ));
            #endif
        #endif
        menu->add(project_multi_autoadvance);

        #ifdef ENABLE_SD
            project_fileviewer = new FileViewerMenuItem("Project");
            menu->add(project_fileviewer);
        #endif
    }
#endif

void setup_menu_midi() {
    menu->add_page("MIDI");
    menu->add(new SeparatorMenuItem("MIDI"));
    menu->add(new LambdaActionItem("{PANIC}", [=]() -> void { midi_matrix_manager->stop_all_notes(); } )); 
    menu->add(new LambdaActionConfirmItem("{HARD PANIC}", [=]() -> void { midi_matrix_manager->stop_all_notes_force(); } ));
    menu->add(&midi_matrix_selector);
    LambdaScaleMenuItemBar *global_quantise_bar = new LambdaScaleMenuItemBar(
        "Global Scale", 
        [=](SCALE scale) -> void { midi_matrix_manager->set_global_scale_type(scale); }, 
        [=]() -> SCALE { return midi_matrix_manager->get_global_scale_type(); },
        [=](int8_t scale_root) -> void { midi_matrix_manager->set_global_scale_root(scale_root); },
        [=]() -> int8_t { return midi_matrix_manager->get_global_scale_root(); }
    );
    global_quantise_bar->add(new LambdaToggleControl("Quantise",
        [=](bool v) -> void { midi_matrix_manager->set_global_quantise_on(v); },
        [=]() -> bool { return midi_matrix_manager->is_global_quantise_on(); }
    ));
    menu->add(global_quantise_bar);
    menu->add(new ToggleControl<bool>("Debug", &midi_matrix_manager->debug));
}

#ifdef ENABLE_SEQUENCER
    void setup_menu_sequencer() {
        // sequencer
        menu->add_page("Sequencer");
        //menu->add(&project_auto_advance_sequencer);
        menu->add(new SeparatorMenuItem("Sequencer"));
        menu->add(&sequencer_status);
        menu->add(&clock_sequencer_display);

        menu->add(new ObjectActionConfirmItem<VirtualBehaviour_SequencerGates>("Clear sequencer pattern", behaviour_sequencer_gates, &VirtualBehaviour_SequencerGates::sequencer_clear_pattern));
        menu->add(&trigger_sequencer_display);
        
        menu->add(new ActionItem("[debug] Reset cache", apcdisplay_initialise_last_sent, false));
        //menu->add(new ActionItem("[debug] Clear display", apcmini_clear_display, false));

        #ifdef ENABLE_SD
            sequence_fileviewer = new FileViewerMenuItem("Sequence");
            menu->add(sequence_fileviewer);
        #endif
    }
#endif

#ifdef ENABLE_LOOPER
    void setup_menu_looper() {
        // looper stuff
        menu->add_page("Looper");
        menu->add(midi_loop_track.make_menu_items());
        #ifdef ENABLE_DRUM_LOOPER
            menu->add(&drum_looper_status);
            menu->add(&drum_loop_quantizer_setting);
        #endif
    }
#endif

#ifndef GDB_DEBUG
//FLASHMEM // causes a section type conflict with 'void Menu::add(LinkedList<MenuItem*>*, uint16_t)'
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
    menu = new Menu(tft, false);
    Serial.println(F("Created Menu object"));
    Serial_flush();

    menu->set_messages_log(messages_log);

    menu->add_pinned(&top_loop_marker_panel);  // pinned position indicator

    setup_menu_transport();
    setup_menu_project();
    setup_menu_midi();
    #if defined(ENABLE_CLOCKS) || defined(ENABLE_SEQUENCER)
        setup_menu_sequencer();
    #endif
    #ifdef ENABLE_LOOPER
        setup_menu_looper();
    #endif
   
    /*Serial.println(F("...starting behaviour_manager#make_menu_items..."));
    behaviour_manager->create_all_behaviour_menu_items(menu);
    Serial.println(F("...finished behaviour_manager#make_menu_items..."));*/

    // todo: probably don't need this stuff anymore {
    #if defined(ENABLE_CRAFTSYNTH_USB) && defined(ENABLE_CRAFTSYNTH_CLOCKTOGGLE)
        menu->add(&craftsynth_clock_toggle);
    #endif

    #ifdef ENABLE_PROFILER
        //DirectNumberControl(const char* label, DataType *target_variable, DataType start_value, DataType min_value, DataType max_value, void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr) 
        DirectNumberControl<uint32_t> *average = new DirectNumberControl<uint32_t>(
            "averages micros per loop", 
            &average_loop_micros, 
            average_loop_micros,
            (uint32_t)0, 
            (uint32_t)(2^64)
        );
        average->readOnly = true;
        menu->add(average);
    #endif
    // }

    Serial.println(F("Exiting setup_menu"));
    Serial_flush();

    /*menu->add(&test_item_1);
    menu->add(&test_item_2);
    menu->add(&test_item_3);*/
}

#endif