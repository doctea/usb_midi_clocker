#include "Config.h"
#include "storage.h"

#ifdef ENABLE_SCREEN

#include "mymenu.h"

#include "submenuitem.h"

#include "menu_looper.h"
#include "menu_sequencer.h"
#include "menu_bpm.h"
#include "menu_midi_mapper.h"
#include "menu_clock_source.h"

#include "midi_lestrum.h"

#include "behaviour_beatstep.h"
#include "behaviour_keystep.h"
#include "behaviour_mpk49.h"
#include "behaviour_subclocker.h"
#include "behaviour_craftsynth.h"

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
#ifdef ENABLE_SEQUENCER
    ObjectToggleControl<Project> project_auto_advance_sequencer  = ObjectToggleControl<Project>("Sequencer auto-advance", &project, &Project::set_auto_advance_sequencer, &Project::is_auto_advance_sequencer, nullptr);
#endif
#ifdef ENABLE_LOOPER
    ObjectToggleControl<Project> project_auto_advance_looper     = ObjectToggleControl<Project>("Looper auto-advance",    &project, &Project::set_auto_advance_looper, &Project::is_auto_advance_looper, nullptr);
#endif
ActionItem project_save = ActionItem("Save settings", &save_project_settings);

BPMPositionIndicator posbar = BPMPositionIndicator();
//LooperStatus mpk49_looper = LooperStatus();
#ifdef ENABLE_BEATSTEP
    HarmonyStatus beatstep_notes =          HarmonyStatus("Beatstep harmony",   &behaviour_beatstep->last_note,          &behaviour_beatstep->current_note);
    #ifdef ENABLE_BASS_TRANSPOSE
        //NumberControl bass_transpose_control =  NumberControl("Bass octave", &bass_transpose_octave, bass_transpose_octave, 1, 4, &bass_transpose_changed);
        ObjectNumberControl<DeviceBehaviour_Beatstep,int> bass_transpose_control = ObjectNumberControl<DeviceBehaviour_Beatstep,int>(
            "Bass octave",
            behaviour_beatstep, 
            &DeviceBehaviour_Beatstep::setTransposeOctave, 
            &DeviceBehaviour_Beatstep::getTransposeOctave, 
            nullptr,
            0,
            8
        );
        HarmonyStatus bass_harmony = HarmonyStatus("Bass harmony", &behaviour_beatstep->last_transposed_note, &behaviour_beatstep->current_transposed_note);
    #endif
    #ifdef ENABLE_BEATSTEP_SYSEX
        ObjectToggleControl<DeviceBehaviour_Beatstep> beatstep_auto_advance = ObjectToggleControl<DeviceBehaviour_Beatstep> (
            "Beatstep auto-advance",
            behaviour_beatstep,
            &DeviceBehaviour_Beatstep::set_auto_advance_pattern,
            &DeviceBehaviour_Beatstep::is_auto_advance_pattern,
            nullptr
        );
    #endif
    //MidiOutputSelectorControl beatstep_output_selector = MidiOutputSelectorControl("Beatstep Output");
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


#ifdef ENABLE_KEYSTEP
    //MidiOutputSelectorControl keystep_output_selector = MidiOutputSelectorControl("KeyStep MIDI Output");
#endif

#ifdef ENABLE_MPK49
    //MidiOutputSelectorControl mpk49_output_selector = MidiOutputSelectorControl("MPK49 MIDI Output");
#endif

#ifdef ENABLE_USB
    USBDevicesPanel usbdevices_panel = USBDevicesPanel();
    /*MidiOutputSelectorControl pc_usb_input_1_selector = MidiOutputSelectorControl("PC USB 1 MIDI Output");
    MidiOutputSelectorControl pc_usb_input_2_selector = MidiOutputSelectorControl("PC USB 2 MIDI Output");
    MidiOutputSelectorControl pc_usb_input_3_selector = MidiOutputSelectorControl("PC USB 3 MIDI Output");
    MidiOutputSelectorControl pc_usb_input_4_selector = MidiOutputSelectorControl("PC USB 4 MIDI Output");*/
#endif

#ifdef ENABLE_LESTRUM
    //MidiOutputSelectorControl lestrum_pads_output_selector = MidiOutputSelectorControl("LeStrum pads Output");
    //MidiOutputSelectorControl lestrum_arp_output_selector  = MidiOutputSelectorControl("LeStrum arp Output");
#endif

/*#ifdef ENABLE_CRAFTSYNTH  // only need this if we want to listen to craftsynth midi *input*
    MidiOutputSelectorControl craftsynth_output_selector = MidiOutputSelectorControl("CraftSynth Output");
#endif*/
#if defined(ENABLE_CRAFTSYNTH_USB) && defined(ENABLE_CRAFTSYNTH_CLOCKTOGGLE)
    ObjectToggleControl<ClockedBehaviour> craftsynth_clock_toggle = ObjectToggleControl<ClockedBehaviour> (
        "CraftSynth clock enable",
        behaviour_craftsynth,
        &ClockedBehaviour::setClockEnabled,
        &ClockedBehaviour::isClockEnabled,
        nullptr
    );
#endif

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

    //ObjectActionItem<DeviceBehaviour_Subclocker> subclocker_restart_action = ObjectActionItem<DeviceBehaviour_Subclocker> ()

#endif


/*void mpk49_loop_track_setOutputWrapper(MIDIOutputWrapper *wrapper) {
    mpk49_loop_track.setOutputWrapper(wrapper);
}*/


/*MenuItem test_item_1 = MenuItem("test 1");
MenuItem test_item_2 = MenuItem("test 2");
MenuItem test_item_3 = MenuItem("test 3");*/

//DisplayTranslator_STeensy steensy = DisplayTranslator_STeensy();
DisplayTranslator_STeensy_Big steensy = DisplayTranslator_STeensy_Big();

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

    /*#ifdef ENABLE_LOOPER
        looper_output_selector.configure(mpk49_loop_track.output, mpk49_loop_track_setOutputWrapper);
    #endif
    #ifdef ENABLE_BEATSTEP    
        beatstep_output_selector.configure(beatstep_output, beatstep_setOutputWrapper);
    #endif
    #ifdef ENABLE_KEYSTEP
        keystep_output_selector.configure(keystep_output, keystep_setOutputWrapper);
    #endif
    #ifdef ENABLE_MPK49
        mpk49_output_selector.configure(mpk49_output, mpk49_setOutputWrapper);
    #endif
    #ifdef ENABLE_LESTRUM
        lestrum_pads_output_selector.configure(lestrum_pads_output, lestrum_pads_setOutputWrapper);
        lestrum_arp_output_selector.configure(lestrum_arp_output, lestrum_arp_setOutputWrapper);
    #endif
    //#ifdef ENABLE_CRAFTSYNTH
    //    craftsynth_output_selector.configure(craftsynth_output, craftsynth_setOutputWrapper);
    //#endif
    #ifdef ENABLE_USB
        pc_usb_input_1_selector.configure(pc_usb_outputs[0], pc_usb_1_setOutputWrapper);
        pc_usb_input_2_selector.configure(pc_usb_outputs[1], pc_usb_2_setOutputWrapper);
        pc_usb_input_3_selector.configure(pc_usb_outputs[2], pc_usb_3_setOutputWrapper);
        pc_usb_input_4_selector.configure(pc_usb_outputs[3], pc_usb_4_setOutputWrapper);
    #endif*/
    
    menu->add_pinned(&top_loop_marker_panel); 
    menu->add(&posbar);

    menu->add(&clock_source_selector);

    //project_selector.go_back_on_select = true;
    menu->add(&project_save);
    menu->add(&project_selector);

    #ifdef ENABLE_BEATSTEP
        menu->add(&beatstep_notes);
        #ifdef ENABLE_BASS_TRANSPOSE
            menu->add(&bass_transpose_control);  // beatstep transposed to neutron control
            menu->add(&bass_harmony);
        #endif
        #ifdef ENABLE_BEATSTEP_SYSEX
            menu->add(&beatstep_auto_advance);
        #endif
        //menu->add(&beatstep_output_selector);
    #endif

    // sequencer
    #ifdef ENABLE_SEQUENCER
        menu->add(&project_auto_advance_sequencer);
        menu->add(&sequencer_status);
    #endif

    // looper stuff
    #ifdef ENABLE_LOOPER
        //looper_submenu.set_tft(tft);
        menu->add(&project_auto_advance_looper);
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

    /*#ifdef ENABLE_BEATSTEP
        menu->add(&beatstep_output_selector);
    #endif*/
    #ifdef ENABLE_KEYSTEP
        //menu->add(&keystep_output_selector);
    #endif
    #ifdef ENABLE_MPK49
        //menu->add(&mpk49_output_selector);
    #endif

    #ifdef ENABLE_LESTRUM
        //menu->add(&lestrum_pads_output_selector);
        //menu->add(&lestrum_arp_output_selector);
    #endif

    #if defined(ENABLE_CRAFTSYNTH_USB) && defined(ENABLE_CRAFTSYNTH_CLOCKTOGGLE)
        menu->add(&craftsynth_clock_toggle);
    #endif

    #ifdef ENABLE_USB
        /*menu->add(&pc_usb_input_1_selector);
        menu->add(&pc_usb_input_2_selector);
        menu->add(&pc_usb_input_3_selector);
        menu->add(&pc_usb_input_4_selector);*/
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