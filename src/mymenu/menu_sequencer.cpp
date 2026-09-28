#include "Config.h"

#include "mymenu.h"
#include "mymenu/menu_sequencer.h"

#if (defined(ENABLE_CLOCKS) || defined(ENABLE_SEQUENCER)) && defined(ENABLE_SCREEN)
    #include "mymenu/menu_gatedisplay.h"
    #include "mymenu/menu_sequencer_display.h"
    SequencerStatus sequencer_status = SequencerStatus("Pattern");
    TriggerSequencerDisplay trigger_sequencer_display = TriggerSequencerDisplay("Trigger Sequencer");
    ClockSequencerDisplay   clock_sequencer_display   = ClockSequencerDisplay("Clock Sequencer");

    #ifdef ENABLE_SD
        #include "menuitems_pageviewer.h"
        extern PageFileViewerMenuItem *sequence_fileviewer;
    #endif

    void setup_menu_sequencer() {
        // sequencer
        menu->add_page("Sequencer", C_WHITE, true, "Project");
        //menu->add(&project_auto_advance_sequencer);
        menu->add(new SeparatorMenuItem("Sequencer"));
        menu->add(&sequencer_status);

        SubMenuItemBar *save_load_bar = new SubMenuItemBar("Sequence load/save", false, true);
        save_load_bar->add(new LambdaNumberControl<int>(
            "Slot", 
            [=](int slot_number) -> void { project->select_scene_number(slot_number); },
            [=]() -> int { return sequencer_status.get_selected_slot(); },
            nullptr, 
            0, 
            NUM_SEQUENCES-1
        ));
        save_load_bar->add(new LambdaActionConfirmItem("Save", [=] () -> void { sequencer_status.save_to_slot_number(sequencer_status.get_selected_slot()); }));
        save_load_bar->add(new LambdaActionConfirmItem("Load", [=] () -> void { sequencer_status.load_slot_number(sequencer_status.get_selected_slot()); }));
        menu->add(save_load_bar);

        menu->add(&clock_sequencer_display);

        menu->add(new ObjectActionConfirmItem<VirtualBehaviour_SequencerGates>("Clear sequencer pattern", behaviour_sequencer_gates, &VirtualBehaviour_SequencerGates::sequencer_clear_pattern));
        menu->add(&trigger_sequencer_display);
        
        menu->add(new ActionItem("[debug] Reset cache", apcdisplay_initialise_last_sent, false));
        //menu->add(new ActionItem("[debug] Clear display", apcmini_clear_display, false));

        #ifdef ENABLE_SD
            sequence_fileviewer = new PageFileViewerMenuItem("Sequence");
            menu->add(sequence_fileviewer);
        #endif
    }
#endif