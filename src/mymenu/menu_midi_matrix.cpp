#include "Config.h"

#include "behaviours/behaviour_cvoutput.h"
#include "mymenu/menuitems_notedisplay.h"
#include "mymenu/menu_midi_matrix.h"

MidiMatrixSelectorControl *midi_matrix_selector = nullptr;

void setup_menu_midi() {
    menu->add_page("MIDI", C_WHITE, true, "Settings");
    menu->remember_opened_page(-1, true);
    menu->add(new SeparatorMenuItem("MIDI"));

    // Matrix manager can be intentionally deferred until after USB init.
    // Avoid dereferencing it during early menu construction.
    if (midi_matrix_manager == nullptr) {
        menu->add(new LambdaActionItem("MIDI Matrix (loading)", [=]() -> void {
            menu_set_last_message("Matrix manager not ready yet", YELLOW);
        }));
        return;
    }

    SubMenuItemBar *midi_matrix_bar = new SubMenuItemBar("Panic", false, false);
    midi_matrix_bar->add(new LambdaActionItem("PANIC", [=]() -> void { midi_matrix_manager->stop_all_notes(); } )); 
    midi_matrix_bar->add(new LambdaActionConfirmItem("{HARD}", [=]() -> void { midi_matrix_manager->stop_all_notes_force(); } ));
    midi_matrix_bar->add(new LambdaActionConfirmItem("reset", [=]() -> void { midi_matrix_manager->reset_matrix(); } ));
    menu->add(midi_matrix_bar);

    if (midi_matrix_selector == nullptr)
        midi_matrix_selector = new MidiMatrixSelectorControl("MIDI Matrix");
    if (midi_matrix_selector != nullptr)
        menu->add(midi_matrix_selector);
    else
        menu->add(new LambdaActionItem("MIDI Matrix (alloc fail)", [=]() -> void {
            menu_set_last_message("Matrix UI alloc failed", RED);
        }));

    menu->add(new ToggleControl<bool>("Debug", &midi_matrix_manager->debug));

    // debuggery stuff ...
    //behaviour_cvoutput_2->debug = true;
    /*
    menu->add(new NoteDisplay("CV Output 1 notes", &behaviour_cvoutput_1->note_tracker));
    menu->add(new NoteHarmonyDisplay(
        (const char*)"CV Output 1 harmony", 
        &midi_matrix_manager->global_scale_type, 
        &midi_matrix_manager->global_scale_root, 
        &behaviour_cvoutput_1->note_tracker,
        &midi_matrix_manager->global_quantise_on
    ));
    menu->add(new HarmonyStatus("CV Output 1 harmony (oldskool)", &behaviour_cvoutput_1->last_transposed_note, &behaviour_cvoutput_1->current_transposed_note));
    menu->add(new NoteDisplay("CV Output 2 notes", &behaviour_cvoutput_2->note_tracker));
    menu->add(new NoteHarmonyDisplay(
        (const char*)"CV Output 2 harmony", 
        &midi_matrix_manager->global_scale_type, 
        &midi_matrix_manager->global_scale_root, 
        &behaviour_cvoutput_2->note_tracker,
        &midi_matrix_manager->global_quantise_on
    ));
    menu->add(new HarmonyStatus("CV Output 2 harmony (oldskool)", &behaviour_cvoutput_2->last_transposed_note, &behaviour_cvoutput_2->current_transposed_note));
    */

    // TODO: this stuff actually now belongs in Conductor menu items..
    // TOOD: but there is still logic in the midi_matrix_manager for requantising everything
    // TODO: so we need to preserve that logic but make it fire on a callback when conductor notifies it
    // menu->add_page("Quantiser");
    // menu->remember_opened_page();
    
    // LambdaScaleMenuItemBar *global_quantise_bar = new LambdaScaleMenuItemBar(
    //     "Global Scale", 
    //     [](scale_index_t t)   { conductor->set_scale_type(t); },
    //     []() -> scale_index_t { return conductor->get_scale_type(); },
    //     [](int8_t r)          { conductor->set_scale_root(r); },
    //     []() -> int8_t        { return conductor->get_scale_root(); },
    //     false, true, true
    // );
    // global_quantise_bar->add(new LambdaToggleControl("Quantise",
    //     [=](bool v) -> void { conductor->set_global_quantise_on(v); },
    //     [=]() -> bool { return conductor->is_global_quantise_on(); }
    // ));
    // menu->add(global_quantise_bar);

    // LambdaChordSubMenuItemBar *global_chord_bar = new LambdaChordSubMenuItemBar(
    //     "Global Chord", 
    //     [=](int8_t degree) -> void { conductor->set_chord_degree(degree); },
    //     [=]() -> int8_t { return conductor->get_chord_degree(); },
    //     [=](CHORD::Type chord_type) -> void { conductor->set_chord_type(chord_type); }, 
    //     [=]() -> CHORD::Type { return conductor->get_chord_type(); },
    //     [=](int8_t inversion) -> void { conductor->set_chord_inversion(inversion); },
    //     [=]() -> int8_t { return conductor->get_chord_inversion(); },
    //     false, true, true
    // );
    // global_chord_bar->add(new LambdaToggleControl("Quantise",
    //     [=](bool v) -> void { conductor->set_global_quantise_chord_on(v); },
    //     [=]() -> bool { return conductor->is_global_quantise_chord_on(); }
    // ));
    // menu->add(global_chord_bar);
}