#include "Config.h"

#ifdef ENABLE_SCREEN

    #include "mymenu/menu_project.h"

    #include "mymenu.h"

    #include "submenuitem_bar.h"
    #include "menuitems_object_multitoggle.h"
    #include "menuitems_lambda.h"

    #include "project.h"

    // needed because we show beatstep autoadvance options on project screen
    #include "behaviours/behaviour_beatstep.h"

    #ifdef ENABLE_SD
        #include "menuitems_pageviewer.h"
        extern PageFileViewerMenuItem *project_fileviewer;
    #endif

    // make these global so that we can toggle it from input_keyboard
    // ObjectMultiToggleControl *project_multi_recall_options = nullptr;
    ObjectMultiToggleControl *project_multi_autoadvance = nullptr;

    void setup_menu_project() {
        
        menu->add_page("Project", C_WHITE, true, "Project");

        // Show the currently loaded project number
        menu->add(
            new LambdaCallbackMenuItem(
                "Project number",
                [=]() -> const char* { 
                    static char buf[36];
                    snprintf(buf, sizeof(buf), "Current: %d", project->getProjectNumber());
                    return buf;
                },
                false
            )
        );

        SubMenuItemBar *project_bar = new SubMenuItemBar("Project load/save", false, true);
        project_bar->add(new LambdaNumberControl<int>(
            "Selected", 
            [=](int project_number) -> void { project->selectProjectNumber(project_number); }, 
            [=]() -> int { return project->getSelectedProjectNumber(); }, 
            nullptr, 
            0, 
            1024,
            true
        ));
        project_bar->add(new LambdaActionConfirmItem("Load", [=]() -> void { project->loadProjectNumber(project->getSelectedProjectNumber()); }));
        project_bar->add(new LambdaActionConfirmItem("Save", [=]() -> void { project->save_project_settings(project->getSelectedProjectNumber());}));
        menu->add(project_bar);

        // ----

        // ActionConfirmItem *project_save = new ActionConfirmItem("Save settings", &save_project_settings);
        // LambdaNumberControl<int> *project_selector = new LambdaNumberControl<int>(
        //     "Project number", 
        //     [=](int project_number) -> void { project->loadProjectNumber(project_number); },
        //     [=]() -> int { return project->getProjectNumber(); },
        //     nullptr, 
        //     0, 
        //     100
        // );

        // menu->add(project_save);       // save project settings button
        // menu->add(project_selector);   // save project selector button

        // // project loading options (whether to load or hold matrix settings, clock, sequence, behaviour options)
        // project_multi_recall_options = new ObjectMultiToggleControl("Recall options", true);
        // MultiToggleItemClass<Project> *load_matrix = new MultiToggleItemClass<Project> (
        //     "MIDI Mappings",
        //     project,
        //     &Project::setLoadMatrixMappings,
        //     &Project::isLoadMatrixMappings
        // );
        // #ifdef ENABLE_CLOCKS
        //     MultiToggleItemClass<Project> *load_clock = new MultiToggleItemClass<Project> (
        //         "Clock Settings",
        //         project,
        //         &Project::setLoadClockSettings,
        //         &Project::isLoadClockSettings    
        //     );
        // #endif
        // #ifdef ENABLE_SEQUENCER
        //     MultiToggleItemClass<Project> *load_scene = new MultiToggleItemClass<Project> (
        //         "Sequence Settings",
        //         project,
        //         &Project::setLoadSequencerSettings,
        //         &Project::isLoadSequencerSettings    
        //     );
        // #endif
        // MultiToggleItemClass<Project> *load_behaviour_settings = new MultiToggleItemClass<Project> {
        //     "Behaviour Options",
        //     project,
        //     &Project::setLoadBehaviourOptions,
        //     &Project::isLoadBehaviourOptions
        // };
        // #ifdef ENABLE_PARAMETERS
        //     MultiToggleItemClass<Project> *load_parameter_input_settings = new MultiToggleItemClass<Project> {
        //         "Parameter Input Options",
        //         project,
        //         &Project::setLoadParameterInputOptions,
        //         &Project::isLoadParameterInputOptions
        //     };
        // #endif

        // project_multi_recall_options->addItem(load_matrix);
        // #ifdef ENABLE_CLOCKS
        //     project_multi_recall_options->addItem(load_clock);
        // #endif
        // #ifdef ENABLE_SEQUENCER
        //     project_multi_recall_options->addItem(load_scene);
        // #endif
        // project_multi_recall_options->addItem(load_behaviour_settings);
        // #ifdef ENABLE_PARAMETERS
        //     project_multi_recall_options->addItem(load_parameter_input_settings);
        // #endif
        // //menu->add(&project_load_matrix_mappings);
        // menu->add(project_multi_recall_options);

        // options for whether to auto-advance looper/sequencer/beatstep
        project_multi_autoadvance = new ObjectMultiToggleControl("Auto-advance", true);
        #ifdef ENABLE_SEQUENCER
            MultiToggleItemClass<Project> *auto_advance_scene = new MultiToggleItemClass<Project> (
                "Sequence",
                project,
                &Project::set_auto_advance_scene,
                &Project::is_auto_advance_scene
            );
            project_multi_autoadvance->addItem(auto_advance_scene);
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
                &DeviceBehaviour_Beatstep::set_auto_advance_pattern,                &DeviceBehaviour_Beatstep::is_auto_advance_pattern
            ));
            #ifdef ENABLE_BEATSTEP_2
                project_multi_autoadvance->addItem(new MultiToggleItemClass<DeviceBehaviour_Beatstep> (
                    "Beatstep 2",
                    behaviour_beatstep_2,
                    &DeviceBehaviour_Beatstep::set_auto_advance_pattern,                    &DeviceBehaviour_Beatstep::is_auto_advance_pattern
                ));
            #endif
        #endif
        // #ifdef ENABLE_PROGRESSION
        //     project_multi_autoadvance->addItem(new MultiToggleItemLambda (
        //         "Prog.Pls",
        //         [=] (bool v) -> void { arranger->get_playback_mode() == LOOP_PLAYLIST; },
        //         [=] () -> bool { return behaviour_progression->advance_progression_playlist; }
        //     ));
        //     project_multi_autoadvance->addItem(new MultiToggleItemLambda (
        //         "Prog.Bar",
        //         [=] (bool v) -> void { behaviour_progression->advance_progression_bar = v; },
        //         [=] () -> bool { return behaviour_progression->advance_progression_bar; }
        //     ));
        // #endif
        menu->add(project_multi_autoadvance);

        #ifdef ENABLE_SD
            project_fileviewer = new PageFileViewerMenuItem("Project");
            menu->add(project_fileviewer);
        #endif
    }

#endif