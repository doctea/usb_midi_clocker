#include "Config.h"
#include "storage.h"

#ifdef ENABLE_SCREEN

#include "mymenu.h"

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

#include "midi_out_wrapper.h"
#include "midi_outs.h"
#include "midi_pc_usb.h"

#include "behaviour_subclocker.h"

#include "clock.h"

#ifdef ENABLE_BASS_TRANSPOSE
    void bass_transpose_changed(int last_value, int new_value) {
        if (last_value!=new_value) {
            Serial.printf("bass_transpose_changed(%i, %i)\n", last_value, new_value);
            midi_out_bass_wrapper.stop_all_notes();
        }
    }
#endif

//DisplayTranslator *tft;
DisplayTranslator_STeensy *tft;
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

LoopMarkerPanel top_loop_marker_panel = LoopMarkerPanel(LOOP_LENGTH, PPQN, BEATS_PER_BAR, BARS_PER_PHRASE);

ClockSourceSelectorControl clock_source_selector = ClockSourceSelectorControl("Clock source", clock_mode);

ObjectNumberControl<Project,int> project_selector = ObjectNumberControl<Project,int>("Project number", &project, &Project::setProjectNumber, &Project::getProjectNumber, nullptr);
ObjectToggleControl<Project> project_autoadvance  = ObjectToggleControl<Project>("Sequencer auto-advance", &project, &Project::set_auto_advance, &Project::is_auto_advance, nullptr);
ActionItem project_save = ActionItem("Save settings", &save_project_settings);

BPMPositionIndicator posbar = BPMPositionIndicator();
//LooperStatus mpk49_looper = LooperStatus();
#ifdef ENABLE_BEATSTEP
    HarmonyStatus beatstep_notes =          HarmonyStatus("Beatstep harmony",   &last_beatstep_note,          &current_beatstep_note);
    MidiOutputSelectorControl beatstep_output_selector = MidiOutputSelectorControl("BeatStep Output");
    #ifdef ENABLE_BASS_TRANSPOSE
        NumberControl bass_transpose_control =  NumberControl("Bass octave", &bass_transpose_octave, bass_transpose_octave, 1, 4, &bass_transpose_changed);
    #endif
#endif
#ifdef ENABLE_SEQUENCER
    SequencerStatus sequencer_status =      SequencerStatus();
#endif
#ifdef ENABLE_LOOPER
    LooperStatus    mpk49_looper_status =   LooperStatus();
    LooperQuantizeControl quantizer_setting = LooperQuantizeControl("Loop quant",   &mpk49_loop_track);   // todo: make this part of the LooperStatus object
    HarmonyStatus looper_harmony_status =   HarmonyStatus("Loop harmony",           &mpk49_loop_track.last_note, &mpk49_loop_track.current_note); // todo: make this part of the LooperStatus object
    LooperTransposeControl looper_transpose_control =    LooperTransposeControl("Loop transpose",      &mpk49_loop_track); // todo: make this part of the LooperStatus object
    MidiOutputSelectorControl looper_output_selector = MidiOutputSelectorControl("Looper MIDI Output"); 
#endif

#ifdef ENABLE_KEYSTEP
    MidiOutputSelectorControl keystep_output_selector = MidiOutputSelectorControl("KeyStep MIDI Output");
#endif

#ifdef ENABLE_MPK49
    MidiOutputSelectorControl mpk49_output_selector = MidiOutputSelectorControl("MPK49 MIDI Output");
#endif

#ifdef ENABLE_USB
    USBDevicesPanel usbdevices_panel = USBDevicesPanel();
    MidiOutputSelectorControl pc_usb_input_1_selector = MidiOutputSelectorControl("PC USB 1 MIDI Output");
    MidiOutputSelectorControl pc_usb_input_2_selector = MidiOutputSelectorControl("PC USB 2 MIDI Output");
#endif

#ifdef ENABLE_LESTRUM
    MidiOutputSelectorControl lestrum_pads_output_selector = MidiOutputSelectorControl("LeStrum pads Output");
    MidiOutputSelectorControl lestrum_arp_output_selector  = MidiOutputSelectorControl("LeStrum arp Output");
#endif

#ifdef ENABLE_SUBCLOCKER
    //NumberControl subclocker_divisor_control =      NumberControl("Subclocker div",     &subclocker_divisor,        subclocker_divisor,     1, 24, nullptr); //&on_subclocker_divisor_changed);
    ObjectNumberControl<DeviceBehaviour_Subclocker,int> subclocker_divisor_control = ObjectNumberControl<DeviceBehaviour_Subclocker,int>(
        "Subclocker div", 
        behaviour_subclocker, 
        &DeviceBehaviour_Subclocker::set_divisor, 
        &DeviceBehaviour_Subclocker::get_divisor, 
        nullptr, //on_subclocker_divisor_changed
        1,  //min
        48  //max
    );
    //NumberControl subclocker_delay_ticks_control =  NumberControl("Subclocker delay",   &subclocker_delay_ticks,    subclocker_delay_ticks, 0, PPQN*BEATS_PER_BAR, nullptr); //&on_subclocker_divisor_changed);
    ObjectNumberControl<DeviceBehaviour_Subclocker,int> subclocker_delay_ticks_control = ObjectNumberControl<DeviceBehaviour_Subclocker,int>(
        "Subclocker delay",
        behaviour_subclocker,
        &DeviceBehaviour_Subclocker::set_delay_ticks,
        &DeviceBehaviour_Subclocker::get_delay_ticks,
        nullptr
    );
#endif

/*MenuItem test_item_1 = MenuItem("test 1");
MenuItem test_item_2 = MenuItem("test 2");
MenuItem test_item_3 = MenuItem("test 3");*/

DisplayTranslator_STeensy steensy = DisplayTranslator_STeensy();

void mpk49_loop_track_setOutputWrapper(MIDIOutputWrapper *wrapper) {
    mpk49_loop_track.setOutputWrapper(wrapper);
}

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

    #ifdef ENABLE_LOOPER
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
    pc_usb_input_1_selector.configure(pc_usb_1_output, pc_usb_1_setOutputWrapper);
    pc_usb_input_2_selector.configure(pc_usb_2_output, pc_usb_2_setOutputWrapper);
    
    menu->add_pinned(&top_loop_marker_panel); 
    menu->add(&posbar);

    menu->add(&clock_source_selector);

    //project_selector.go_back_on_select = true;
    menu->add(&project_selector);
    menu->add(&project_autoadvance);
    menu->add(&project_save);

    #ifdef ENABLE_BEATSTEP
        menu->add(&beatstep_notes);
        menu->add(&bass_transpose_control);  // beatstep transposed to neutron control
    #endif

    // sequencer
    #ifdef ENABLE_SEQUENCER
        menu->add(&sequencer_status);
    #endif

    // looper stuff
    #ifdef ENABLE_LOOPER
        menu->add(&mpk49_looper_status);    
        menu->add(&quantizer_setting);       // todo: make this part of the LooperStatus object
        menu->add(&looper_harmony_status);   // todo: make this part of the LooperStatus object
        menu->add(&looper_output_selector);
        menu->add(&looper_transpose_control);
    #endif

    #ifdef ENABLE_BEATSTEP
        menu->add(&beatstep_output_selector);
    #endif
    #ifdef ENABLE_KEYSTEP
        menu->add(&keystep_output_selector);
    #endif
    #ifdef ENABLE_MPK49
        menu->add(&mpk49_output_selector);
    #endif

    #ifdef ENABLE_LESTRUM
        menu->add(&lestrum_pads_output_selector);
        menu->add(&lestrum_arp_output_selector);
    #endif

    menu->add(&pc_usb_input_1_selector);
    menu->add(&pc_usb_input_2_selector);

    #ifdef ENABLE_SUBCLOCKER
        subclocker_divisor_control.go_back_on_select = subclocker_delay_ticks_control.go_back_on_select = true; 
        subclocker_divisor_control.target_object     = subclocker_delay_ticks_control.target_object     = behaviour_subclocker;   // because behaviour_subclocker pointer won't be set yet..?
        menu->add(&subclocker_divisor_control);
        menu->add(&subclocker_delay_ticks_control);
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