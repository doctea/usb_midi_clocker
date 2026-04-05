#pragma once

// settings_root.h — defines SettingsRoot, the top-level saveloadlib node.
//
// Deliberately NOT included by storage.h because:
//   - storage.h is pulled in by midi_looper.h and many low-level headers
//   - project.h, behaviour_manager.h, and midi_mapper_matrix_manager.h are
//     all higher-level and must not be dragged into low-level include chains
//
// Include this header in:
//   - storage.cpp  (for setup_saveloadlib() / SettingsRoot definition)
//   - main.cpp     (if you need to call setup_saveloadlib() with full type)
//   - Any file that needs to iterate the full settings tree

#include "storage.h"
#include "project.h"
#include "behaviours/behaviour_manager.h"
#include "midi/midi_mapper_matrix_manager.h"

class SettingsRoot : public SHStorage<16, 16> {
    public:
        SettingsRoot() {
            this->set_path_segment("root");
        }

        virtual void setup_saveable_settings() override {
            // Project-scope settings (project scalars + behaviour project options)
            register_child(project);

            // Behaviour tree — each behaviour is a child of behaviour_manager
            register_child(behaviour_manager);

            // MIDI matrix mappings — connections (SL_SCOPE_ROUTING) + scale settings (SL_SCOPE_PROJECT)
            register_child(midi_matrix_manager);

            // Scene / pattern-scope settings (clock multipliers, sequence data)
            register_child(&storage::current_state);
        }
};
