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

#include "menuitems_object_multitoggle.h"

#include "mymenu/menu_usb.h"
#include "mymenu/menu_behaviours.h"

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

//DisplayTranslator *tft;
DisplayTranslator_STeensy_Big *tft;
Menu *menu; // = Menu();

#ifdef ENCODER_KNOB_L
    Encoder knob(ENCODER_KNOB_L, ENCODER_KNOB_R);
    //extern Encoder knob;
#endif
#ifdef PIN_BUTTON_A
    Bounce pushButtonA = Bounce(PIN_BUTTON_A, 10); // 10ms debounce
    //extern Bounce pushButtonA;
#endif
#ifdef PIN_BUTTON_B
    Bounce pushButtonB = Bounce(PIN_BUTTON_B, 10); // 10ms debounce
    //extern Bounce pushButtonB; 
#endif
#ifdef PIN_BUTTON_C
    Bounce pushButtonC = Bounce(PIN_BUTTON_C, 10); // 10ms debounce
    //extern Bounce pushButtonC;
#endif

LoopMarkerPanel top_loop_marker_panel = LoopMarkerPanel(LOOP_LENGTH_TICKS, PPQN, BEATS_PER_BAR, BARS_PER_PHRASE);

BPMPositionIndicator posbar = BPMPositionIndicator();
ClockSourceSelectorControl clock_source_selector = ClockSourceSelectorControl("Clock source", clock_mode);

ActionConfirmItem project_save = ActionConfirmItem("Save settings", &save_project_settings);
ObjectNumberControl<Project,int> project_selector = ObjectNumberControl<Project,int>("Project number", &project, &Project::setProjectNumber, &Project::getProjectNumber, nullptr, 0, 100);

// make these global so that we can toggle it from input_keyboard
ObjectMultiToggleControl *project_multi_recall_options = nullptr;
ObjectMultiToggleControl *project_multi_autoadvance = nullptr;

#ifdef ENABLE_SEQUENCER
    SequencerStatus sequencer_status =      SequencerStatus("Pattern");
#endif

#ifdef ENABLE_DRUM_LOOPER
    LooperStatus            drum_looper_status  =   LooperStatus("Drum looper", &drums_loop_track);
    LooperQuantizeControl   drum_loop_quantizer_setting = LooperQuantizeControl("Drum Loop quant",   &drums_loop_track);   // todo: make this part of the LooperStatus object
#endif

#if defined(ENABLE_CRAFTSYNTH_USB) && defined(ENABLE_CRAFTSYNTH_CLOCKTOGGLE)
    ObjectToggleControl<ClockedBehaviour> craftsynth_clock_toggle = ObjectToggleControl<ClockedBehaviour> (
        "CraftSynth clock enable",
        behaviour_craftsynth,
        &ClockedBehaviour::setClockEnabled,
        &ClockedBehaviour::isClockEnabled,
        nullptr
    );
#endif

MidiMatrixSelectorControl midi_matrix_selector = MidiMatrixSelectorControl("MIDI Matrix");

/*MenuItem test_item_1 = MenuItem("test 1");
MenuItem test_item_2 = MenuItem("test 2");
MenuItem test_item_3 = MenuItem("test 3");*/

//DisplayTranslator_STeensy steensy = DisplayTranslator_STeensy();
//DisplayTranslator_STeensy_Big steensy = DisplayTranslator_STeensy_Big();
DisplayTranslator_Configured steensy = DisplayTranslator_Configured();

#ifndef GDB_DEBUG
FLASHMEM 
#endif
void setup_menu() {
    Serial.println(F("Starting setup_menu()..")); //Instantiating DisplayTranslator_STeensy.."));
    tft = &steensy; //DisplayTranslator_STeensy();
    //delay(50);
    //Serial.println(F("Finished  constructor"));
    Serial_flush();
    Serial.println(F("Creating Menu object.."));
    Serial_flush();
    menu = new Menu(tft);
    Serial.println(F("Created Menu object"));
    Serial_flush();

    menu->add_pinned(&top_loop_marker_panel);  // pinned position indicator
    menu->add(&posbar);     // bpm and position indicator

    menu->add(&clock_source_selector);  // midi clock source (internal or from PC USB)

    menu->add(new SeparatorMenuItem((char*)"Project"));

    menu->add(&project_save);       // save project settings button
    menu->add(&project_selector);   // save project selector button

    // project loading options (whether to load or hold matrix settings, clock, sequence, behaviour options)
    project_multi_recall_options = new ObjectMultiToggleControl("Recall options", true);
    MultiToggleItemClass<Project> *load_matrix = new MultiToggleItemClass<Project> (
        (char*)"MIDI Mappings",
        &project,
        &Project::setLoadMatrixMappings,
        &Project::isLoadMatrixMappings    
    );
    #ifdef ENABLE_CLOCKS
        MultiToggleItemClass<Project> *load_clock = new MultiToggleItemClass<Project> (
            (char*)"Clock Settings",
            &project,
            &Project::setLoadClockSettings,
            &Project::isLoadClockSettings    
        );
    #endif
    #ifdef ENABLE_SEQUENCER
        MultiToggleItemClass<Project> *load_sequence = new MultiToggleItemClass<Project> (
            (char*)"Sequence Settings",
            &project,
            &Project::setLoadSequencerSettings,
            &Project::isLoadSequencerSettings    
        );
    #endif
    MultiToggleItemClass<Project> *load_behaviour_settings = new MultiToggleItemClass<Project> {
        (char*)"Behaviour Options",
        &project,
        &Project::setLoadBehaviourOptions,
        &Project::isLoadBehaviourOptions
    };
    project_multi_recall_options->addItem(load_matrix);
    #ifdef ENABLE_CLOCKS
        project_multi_recall_options->addItem(load_clock);
    #endif
    #ifdef ENABLE_SEQUENCER
        project_multi_recall_options->addItem(load_sequence);
    #endif
    project_multi_recall_options->addItem(load_behaviour_settings);
    //menu->add(&project_load_matrix_mappings);
    menu->add(project_multi_recall_options);

    // options for whether to auto-advance looper/sequencer/beatstep
    project_multi_autoadvance = new ObjectMultiToggleControl("Auto-advance", true);
    #ifdef ENABLE_SEQUENCER
        MultiToggleItemClass<Project> *auto_advance_sequencer = new MultiToggleItemClass<Project> (
            (char*)"Sequence",
            &project,
            &Project::set_auto_advance_sequencer,
            &Project::is_auto_advance_sequencer
        );
        project_multi_autoadvance->addItem(auto_advance_sequencer);
    #endif
    #ifdef ENABLE_LOOPER
        MultiToggleItemClass<Project> *auto_advance_looper = new MultiToggleItemClass<Project> (
            (char*)"Looper",
            &project,
            &Project::set_auto_advance_looper,
            &Project::is_auto_advance_looper
        );
        project_multi_autoadvance->addItem(auto_advance_looper);
    #endif
    #if defined(ENABLE_BEATSTEP) && defined(ENABLE_BEATSTEP_SYSEX)
        project_multi_autoadvance->addItem(new MultiToggleItemClass<DeviceBehaviour_Beatstep> (
            (char*)"Beatstep",
            behaviour_beatstep,
            &DeviceBehaviour_Beatstep::set_auto_advance_pattern,
            &DeviceBehaviour_Beatstep::is_auto_advance_pattern
        ));
    #endif
    menu->add(project_multi_autoadvance);

    menu->add_page("MIDI");
    menu->add(new SeparatorMenuItem((char*)"MIDI"));
    menu->add(new ObjectActionItem<MIDIMatrixManager>("{PANIC}", midi_matrix_manager, &MIDIMatrixManager::stop_all_notes));
    menu->add(&midi_matrix_selector);
    
    /*Serial.println(F("...starting behaviour_manager#make_menu_items..."));
    behaviour_manager->create_all_behaviour_menu_items(menu);
    Serial.println(F("...finished behaviour_manager#make_menu_items..."));*/

    // sequencer
    #ifdef ENABLE_SEQUENCER
        menu->add_page("Sequencer");
        //menu->add(&project_auto_advance_sequencer);
        menu->add(new SeparatorMenuItem((char*)"Sequencer"));
        menu->add(&sequencer_status);
    #endif

    // looper stuff
    #ifdef ENABLE_LOOPER
        menu->add_page("Looper");
        menu->add(mpk49_loop_track.make_menu_items());
        #ifdef ENABLE_DRUM_LOOPER
            menu->add(&drum_looper_status);
            menu->add(&drum_loop_quantizer_setting);
        #endif
    #endif

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

    // enable encoder and separate buttons
    pinMode(PIN_BUTTON_A, INPUT_PULLUP);
    pinMode(PIN_BUTTON_B, INPUT_PULLUP);
    pinMode(PIN_BUTTON_C, INPUT_PULLUP);

    Serial.println(F("Exiting setup_menu"));
    Serial_flush();

    /*menu->add(&test_item_1);
    menu->add(&test_item_2);
    menu->add(&test_item_3);*/
}

#endif