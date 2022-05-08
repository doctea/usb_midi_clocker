#include "Config.h"
#include "storage.h"

#ifdef ENABLE_SCREEN
#include "mymenu.h"

#include "menu_looper.h"
#include "menu_sequencer.h"
#include "midi_beatstep.h"

#include "midi_out_wrapper.h"
#include "midi_outs.h"

void bass_transpose_changed(int last_value, int new_value) {
    Serial.printf("bass_transpose_changed(%i, %i)\n", last_value, new_value);
    if (last_value!=new_value) {
        midi_out_bass_wrapper.stop_all_notes();
    }
}

Menu menu = Menu();

PositionIndicator posbar = PositionIndicator();
//LooperStatus mpk49_looper = LooperStatus();
HarmonyStatus beatstep_notes =          HarmonyStatus("Beatstep harmony",   &last_beatstep_note,          &current_beatstep_note);
SequencerStatus sequencer_status =      SequencerStatus();
LooperStatus    mpk49_looper_status =   LooperStatus();
LooperQuantizeChanger quantizer_setting = LooperQuantizeChanger("Loop quant", &mpk49_loop_track);   // todo: make this part of the LooperStatus object
HarmonyStatus looper_harmony_status =   HarmonyStatus("Loop harmony",      &mpk49_loop_track.last_note, &mpk49_loop_track.current_note); // todo: make this part of the LooperStatus object
TransposeControl transpose_control =    TransposeControl("Loop transpose", &mpk49_loop_track); // todo: make this part of the LooperStatus object
NumberControl bass_transpose_control =  NumberControl("Bass octave", &bass_transpose_octave, bass_transpose_octave, 1, 4, &bass_transpose_changed);
USBDevicesPanel usbdevices_panel = USBDevicesPanel();

//MenuItem test_item_1 = MenuItem("test 1");
//MenuItem test_item_2 = MenuItem("test 2");
//MenuItem test_item_3 = MenuItem("test 3");

void setup_menu() {
    menu.add(&posbar);
    //menu.add(&mpk49_looper);
    menu.add(&beatstep_notes);
    menu.add(&sequencer_status);
    menu.add(&mpk49_looper_status);
    menu.add(&quantizer_setting);       // todo: make this part of the LooperStatus object
    menu.add(&looper_harmony_status);   // todo: make this part of the LooperStatus object
    menu.add(&transpose_control);
    menu.add(&bass_transpose_control);
    menu.add(&usbdevices_panel);

    //todo: move this to menu
    pinMode(PIN_BUTTON_A, INPUT_PULLUP);
    pinMode(PIN_BUTTON_B, INPUT_PULLUP);
    pinMode(PIN_BUTTON_C, INPUT_PULLUP);

    //menu.add(&test_item_1);
    //menu.add(&test_item_2);
    //menu.add(&test_item_3);
}

#endif