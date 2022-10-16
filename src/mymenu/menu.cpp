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

ClockSourceSelectorControl clock_source_selector = ClockSourceSelectorControl("Clock source", clock_mode);

ObjectNumberControl<Project,int> project_selector = ObjectNumberControl<Project,int>("Project number", &project, &Project::setProjectNumber, &Project::getProjectNumber, nullptr, 0, 100);
//ObjectToggleControl<Project> project_load_matrix_mappings = ObjectToggleControl<Project>("Load project MIDI matrix settings", &project, &Project::setLoadMatrixMappings, &Project::isLoadMatrixMappings, nullptr);

/*#ifdef ENABLE_SEQUENCER
    ObjectToggleControl<Project> project_auto_advance_sequencer  = ObjectToggleControl<Project>("Sequencer auto-advance", &project, &Project::set_auto_advance_sequencer, &Project::is_auto_advance_sequencer, nullptr);
#endif
#ifdef ENABLE_LOOPER
    ObjectToggleControl<Project> project_auto_advance_looper     = ObjectToggleControl<Project>("Looper auto-advance",    &project, &Project::set_auto_advance_looper, &Project::is_auto_advance_looper, nullptr);
#endif*/
ActionConfirmItem project_save = ActionConfirmItem("Save settings", &save_project_settings);

BPMPositionIndicator posbar = BPMPositionIndicator();

#ifdef ENABLE_BEATSTEP
    HarmonyStatus beatstep_notes = HarmonyStatus("Beatstep harmony",   &behaviour_beatstep->last_note,          &behaviour_beatstep->current_note);
    #ifdef ENABLE_BEATSTEP_SYSEX
        ObjectToggleControl<DeviceBehaviour_Beatstep> beatstep_auto_advance = ObjectToggleControl<DeviceBehaviour_Beatstep> (
            "Beatstep auto-advance",
            behaviour_beatstep,
            &DeviceBehaviour_Beatstep::set_auto_advance_pattern,
            &DeviceBehaviour_Beatstep::is_auto_advance_pattern,
            nullptr
        );
    #endif
#endif
#ifdef ENABLE_SEQUENCER
    SequencerStatus sequencer_status =      SequencerStatus("Sequencer");
#endif
#ifdef ENABLE_LOOPER
    //SubMenuItem     looper_submenu = SubMenuItem("Looper Submenu");
    LooperStatus            mpk49_looper_status =       LooperStatus("Looper",                  &mpk49_loop_track);
    LooperQuantizeControl   quantizer_setting =         LooperQuantizeControl("Loop quant",     &mpk49_loop_track);   
    LooperTransposeControl  looper_transpose_control =  LooperTransposeControl("Loop transpose",&mpk49_loop_track);
    //MidiOutputSelectorControl looper_output_selector =  MidiOutputSelectorControl("Looper MIDI Output"); 
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

FLASHMEM void setup_menu() {

    Serial.println(F("Instantiating DisplayTranslator_STeensy.."));
    tft = &steensy; //DisplayTranslator_STeensy();
    delay(50);
    Serial.println(F("Finished DisplayTranslator_SS_OLED constructor"));
    Serial.flush();
    Serial.println(F("Creating Menu object.."));
    Serial.flush();
    menu = new Menu(tft);
    Serial.println(F("Created Menu object.."));
    Serial.flush();

    menu->add_pinned(&top_loop_marker_panel); 
    menu->add(&posbar);

    menu->add(&clock_source_selector);

    menu->add(&project_save);
    menu->add(&project_selector);

    // project loading options (whether to load or hold matrix settings, clock, sequence, behaviour options)
    ObjectMultiToggleControl *project_multi_load_options = new ObjectMultiToggleControl("Recall options", true);
    MultiToggleItemClass<Project> *load_matrix = new MultiToggleItemClass<Project> (
        (char*)"MIDI Mappings",
        &project,
        &Project::setLoadMatrixMappings,
        &Project::isLoadMatrixMappings    
    );
    MultiToggleItemClass<Project> *load_clock = new MultiToggleItemClass<Project> (
        (char*)"Clock Settings",
        &project,
        &Project::setLoadClockSettings,
        &Project::isLoadClockSettings    
    );
    MultiToggleItemClass<Project> *load_sequence = new MultiToggleItemClass<Project> (
        (char*)"Sequence Settings",
        &project,
        &Project::setLoadSequencerSettings,
        &Project::isLoadSequencerSettings    
    );
    MultiToggleItemClass<Project> *load_behaviour_settings = new MultiToggleItemClass<Project> {
        (char*)"Behaviour Options",
        &project,
        &Project::setLoadBehaviourOptions,
        &Project::isLoadBehaviourOptions
    };
    project_multi_load_options->addItem(load_matrix);
    project_multi_load_options->addItem(load_clock);
    project_multi_load_options->addItem(load_sequence);
    project_multi_load_options->addItem(load_behaviour_settings);
    //menu->add(&project_load_matrix_mappings);
    menu->add(project_multi_load_options);

    // options for whether to auto-advance looper/sequencer/beatstep
    ObjectMultiToggleControl *project_multi_autoadvance = new ObjectMultiToggleControl("Auto-advance", true);
    MultiToggleItemClass<Project> *auto_advance_sequencer = new MultiToggleItemClass<Project> (
        (char*)"Sequence",
        &project,
        &Project::set_auto_advance_sequencer,
        &Project::is_auto_advance_sequencer
    );
    project_multi_autoadvance->addItem(auto_advance_sequencer);

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
        menu->add(&beatstep_auto_advance);
        project_multi_autoadvance->addItem(new MultiToggleItemClass<DeviceBehaviour_Beatstep> (
            (char*)"Beatstep advance",
            behaviour_beatstep,
            &DeviceBehaviour_Beatstep::set_auto_advance_pattern,
            &DeviceBehaviour_Beatstep::is_auto_advance_pattern
        ));
    #endif
    menu->add(project_multi_autoadvance);

    menu->add(&midi_matrix_selector);

    #ifdef ENABLE_BEATSTEP
        menu->add(&beatstep_notes);
    #endif

    Serial.println(F("...starting behaviour_manager#make_menu_items..."));
    behaviour_manager->create_behaviour_menu_items(menu);
    Serial.println(F("...finished behaviour_manager#make_menu_items..."));

    // sequencer
    #ifdef ENABLE_SEQUENCER
        //menu->add(&project_auto_advance_sequencer);
        menu->add(&sequencer_status);
    #endif

    // looper stuff
    #ifdef ENABLE_LOOPER
        //looper_submenu.set_tft(tft);
        //menu->add(&project_auto_advance_looper);
        menu->add(&mpk49_looper_status); 
        menu->add(&quantizer_setting);       // todo: make this part of the LooperStatus object..? (maybe not as it allows interaction)
        //menu->add(&looper_output_selector);
        menu->add(&looper_transpose_control);
        //menu->add(&looper_submenu);
    #endif
    #ifdef ENABLE_DRUM_LOOPER
        menu->add(&drum_looper_status);
        menu->add(&drum_loop_quantizer_setting);
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

    /*#ifdef ENABLE_SUBCLOCKER
        subclocker_divisor_control.go_back_on_select = subclocker_delay_ticks_control.go_back_on_select = true; 
        subclocker_restart_action.target_object = 
            subclocker_divisor_control.target_object = 
                subclocker_delay_ticks_control.target_object = 
                    behaviour_subclocker;   // because behaviour_subclocker pointer won't be set before now..?
        menu->add(&subclocker_divisor_control);
        menu->add(&subclocker_delay_ticks_control);
        menu->add(&subclocker_restart_action);
    #endif

    #ifdef ENABLE_BEATSTEP_DIVISOR
        beatstep_divisor_control.go_back_on_select = beatstep_delay_ticks_control.go_back_on_select = true; 
        beatstep_restart_action.target_object = 
            beatstep_divisor_control.target_object = 
                beatstep_delay_ticks_control.target_object = 
                    behaviour_beatstep;   // because behaviour_beatstep pointer won't be set before now..?
        menu->add(&beatstep_divisor_control);
        menu->add(&beatstep_delay_ticks_control);
        menu->add(&beatstep_restart_action);
    #endif*/


    pinMode(PIN_BUTTON_A, INPUT_PULLUP);
    pinMode(PIN_BUTTON_B, INPUT_PULLUP);
    pinMode(PIN_BUTTON_C, INPUT_PULLUP);

    Serial.println(F("Exiting setup_menu"));
    Serial.flush();

    /*menu->add(&test_item_1);
    menu->add(&test_item_2);
    menu->add(&test_item_3);*/
}

#endif