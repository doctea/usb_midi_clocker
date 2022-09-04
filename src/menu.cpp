#include "Config.h"
#include "storage.h"

#ifdef ENABLE_SCREEN

#include "mymenu.h"

#include "submenuitem.h"

#include "menu_looper.h"
#include "menu_sequencer.h"
#include "menu_bpm.h"
#include "menu_clock_source.h"
#include "menu_midi_matrix.h"

#include "menuitems_object_multitoggle.h"

#include "menu_usb.h"
#include "menu_behaviours.h"

#include "behaviour_beatstep.h"
#include "behaviour_keystep.h"
#include "behaviour_mpk49.h"
#include "behaviour_subclocker.h"
#include "behaviour_craftsynth.h"
#include "behaviour_neutron.h"

#include "midi_out_wrapper.h"
#include "midi_outs.h"
#include "midi_pc_usb.h"

#include "behaviour_subclocker.h"

#include "clock.h"

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

ObjectNumberControl<Project,int> project_selector = ObjectNumberControl<Project,int>("Project number", &project, &Project::setProjectNumber, &Project::getProjectNumber, nullptr);
//ObjectToggleControl<Project> project_load_matrix_mappings = ObjectToggleControl<Project>("Load project MIDI matrix settings", &project, &Project::setLoadMatrixMappings, &Project::isLoadMatrixMappings, nullptr);

ObjectMultiToggleControl project_multi_options = ObjectMultiToggleControl("Recall options", true);

ObjectMultiToggleControl project_multi_autoadvance = ObjectMultiToggleControl("Auto-advance", true);

/*#ifdef ENABLE_SEQUENCER
    ObjectToggleControl<Project> project_auto_advance_sequencer  = ObjectToggleControl<Project>("Sequencer auto-advance", &project, &Project::set_auto_advance_sequencer, &Project::is_auto_advance_sequencer, nullptr);
#endif
#ifdef ENABLE_LOOPER
    ObjectToggleControl<Project> project_auto_advance_looper     = ObjectToggleControl<Project>("Looper auto-advance",    &project, &Project::set_auto_advance_looper, &Project::is_auto_advance_looper, nullptr);
#endif*/
ActionItem project_save = ActionItem("Save settings", &save_project_settings);

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

#ifdef ENABLE_USB
    USBDevicesPanel usbdevices_panel = USBDevicesPanel();
#endif

BehavioursPanel behaviours_panel = BehavioursPanel();

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

#ifdef ENABLE_SUBCLOCKER
    ObjectNumberControl<DeviceBehaviour_Subclocker,int> subclocker_divisor_control = ObjectNumberControl<DeviceBehaviour_Subclocker,int>(
        "Subclocker div", 
        behaviour_subclocker, 
        &DeviceBehaviour_Subclocker::set_divisor, 
        &DeviceBehaviour_Subclocker::get_divisor, 
        nullptr, // change callback on_subclocker_divisor_changed
        1,  //min
        48  //max
    );
    ObjectNumberControl<DeviceBehaviour_Subclocker,int> subclocker_delay_ticks_control = ObjectNumberControl<DeviceBehaviour_Subclocker,int>(
        "Subclocker delay",
        behaviour_subclocker,
        &DeviceBehaviour_Subclocker::set_delay_ticks,
        &DeviceBehaviour_Subclocker::get_delay_ticks,
        nullptr
    );
    ObjectActionItem<DeviceBehaviour_Subclocker> subclocker_restart_action = ObjectActionItem<DeviceBehaviour_Subclocker>(
        "Restart Subclocker on bar",
        behaviour_subclocker,
        &DeviceBehaviour_Subclocker::set_restart_on_bar,
        &DeviceBehaviour_Subclocker::is_set_restart_on_bar,
        "Restarting.."
    );
#endif

/*MenuItem test_item_1 = MenuItem("test 1");
MenuItem test_item_2 = MenuItem("test 2");
MenuItem test_item_3 = MenuItem("test 3");*/

//DisplayTranslator_STeensy steensy = DisplayTranslator_STeensy();
//DisplayTranslator_STeensy_Big steensy = DisplayTranslator_STeensy_Big();
DisplayTranslator_Configured steensy = DisplayTranslator_Configured();

void setup_menu() {

    Serial.println("Instantiating DisplayTranslator_STeensy..");
    tft = &steensy; //DisplayTranslator_STeensy();
    delay(50);
    Serial.println("Finished DisplayTranslator_SS_OLED constructor");
    Serial.flush();
    Serial.println("Creating Menu object..");
    Serial.flush();
    menu = new Menu(tft);
    Serial.println("Created Menu object..");
    Serial.flush();

    menu->add_pinned(&top_loop_marker_panel); 
    menu->add(&posbar);

    menu->add(&clock_source_selector);

    menu->add(&project_save);
    menu->add(&project_selector);

    // project loading options (whether to load or hold matrix settings, clock, sequence)
    MultiToggleItemClass<Project> *load_matrix = new MultiToggleItemClass<Project> (
        (char*)"Load MIDI Mappings",
        &project,
        &Project::setLoadMatrixMappings,
        &Project::isLoadMatrixMappings    
    );
    MultiToggleItemClass<Project> *load_clock = new MultiToggleItemClass<Project> (
        (char*)"Load Clock Settings",
        &project,
        &Project::setLoadClockSettings,
        &Project::isLoadClockSettings    
    );
    MultiToggleItemClass<Project> *load_sequence = new MultiToggleItemClass<Project> (
        (char*)"Load Sequence Settings",
        &project,
        &Project::setLoadSequencerSettings,
        &Project::isLoadSequencerSettings    
    );
    project_multi_options.addItem(load_matrix);
    project_multi_options.addItem(load_clock);
    project_multi_options.addItem(load_sequence);
    //menu->add(&project_load_matrix_mappings);
    menu->add(&project_multi_options);

    // options for whether to auto-advance looper/sequencer/beatstep
    MultiToggleItemClass<Project> *auto_advance_sequencer = new MultiToggleItemClass<Project> (
        (char*)"Sequence",
        &project,
        &Project::set_auto_advance_sequencer,
        &Project::is_auto_advance_sequencer
    );
    MultiToggleItemClass<Project> *auto_advance_looper = new MultiToggleItemClass<Project> (
        (char*)"Looper",
        &project,
        &Project::set_auto_advance_looper,
        &Project::is_auto_advance_looper
    );
    project_multi_autoadvance.addItem(auto_advance_sequencer);
    project_multi_autoadvance.addItem(auto_advance_looper);
    #if defined(ENABLE_BEATSTEP) && defined(ENABLE_BEATSTEP_SYSEX)
        menu->add(&beatstep_auto_advance);
        project_multi_autoadvance.addItem(new MultiToggleItemClass<DeviceBehaviour_Beatstep> (
            (char*)"Beatstep advance",
            &behaviour_beatstep,
            &DeviceBehaviour_Beatstep::set_auto_advance_pattern(),
            &DeviceBehaviour_Beatstep::is_auto_advance_pattern()
        ));
    #endif
    menu->add(&project_multi_autoadvance);

    menu->add(&midi_matrix_selector);

    #ifdef ENABLE_BEATSTEP
        menu->add(&beatstep_notes);
    #endif

    #ifdef ENABLE_BASS_TRANSPOSE
        MIDIOutputWrapper_Behaviour *neutron_wrapper = (MIDIOutputWrapper_Behaviour *)midi_matrix_manager->get_target_for_handle((char*)"S3 : Neutron : ch 4");
        ObjectNumberControl<MIDIOutputWrapper,int> *neutron_transpose_control = new ObjectNumberControl<MIDIOutputWrapper,int>(
            "Neutron octave",
            neutron_wrapper, 
            &MIDIOutputWrapper::setForceOctave, 
            &MIDIOutputWrapper::getForceOctave, 
            nullptr,
            -1,
            8
        );
        //DeviceBehaviour_Neutron *behaviour_neutron = static_cast<DeviceBehaviour_Neutron *>(neutron_wrapper->output);
        HarmonyStatus *neutron_harmony = new HarmonyStatus("Neutron output", 
            &neutron_wrapper->last_transposed_note, 
            &neutron_wrapper->current_transposed_note, 
            &behaviour_neutron.last_drone_note
        );
        
        //TODO: see commented-out section in DeviceBehaviour_Neutron
        ObjectToggleControl<DeviceBehaviour_Neutron> *neutron_drone_bass = new ObjectToggleControl<DeviceBehaviour_Neutron> (
            "Neutron bass drone",
            &behaviour_neutron,
            &DeviceBehaviour_Neutron::set_drone,
            &DeviceBehaviour_Neutron::is_drone,
            nullptr
        );
        menu->add(neutron_drone_bass);
        
        menu->add(neutron_transpose_control);  // beatstep transposed to neutron control
        menu->add(neutron_harmony);
    #endif

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

    #ifdef ENABLE_SUBCLOCKER
        subclocker_divisor_control.go_back_on_select = subclocker_delay_ticks_control.go_back_on_select = true; 
        subclocker_restart_action.target_object = 
            subclocker_divisor_control.target_object = 
                subclocker_delay_ticks_control.target_object = 
                    behaviour_subclocker;   // because behaviour_subclocker pointer won't be set before now..?
        menu->add(&subclocker_divisor_control);
        menu->add(&subclocker_delay_ticks_control);
        menu->add(&subclocker_restart_action);
    #endif

    menu->add(&usbdevices_panel);
    menu->add(&behaviours_panel);

    ObjectToggleControl<Menu> *debug_times_control = new ObjectToggleControl<Menu>("Debug: Menu item times", menu, &Menu::setDebugTimes, &Menu::isDebugTimes, nullptr);
    menu->add(debug_times_control);

    pinMode(PIN_BUTTON_A, INPUT_PULLUP);
    pinMode(PIN_BUTTON_B, INPUT_PULLUP);
    pinMode(PIN_BUTTON_C, INPUT_PULLUP);

    Serial.println("Exiting setup_menu");
    Serial.flush();

    /*menu->add(&test_item_1);
    menu->add(&test_item_2);
    menu->add(&test_item_3);*/
}

#endif