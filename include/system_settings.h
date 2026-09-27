#pragma once

#include "storage.h"

class SystemSettings : public SHDynamic<0, 2> {
    bool release_outputs_on_midi_stop = true;
    bool release_outputs_on_external_clock_loss = false;

    public:
        SystemSettings() {
            set_path_segment("system");
        }

        virtual void setup_saveable_settings() override {
            register_setting(new VarSetting<bool>(
                "release_outputs_on_midi_stop", "MIDI Configuration", &release_outputs_on_midi_stop
            ), SL_SCOPE_SYSTEM, false);
            register_setting(new VarSetting<bool>(
                "release_outputs_on_external_clock_loss", "MIDI Configuration", &release_outputs_on_external_clock_loss
            ), SL_SCOPE_SYSTEM, false);
        }

        void setReleaseOutputsOnMidiStop(bool value) {
            release_outputs_on_midi_stop = value;
        }
        bool isReleaseOutputsOnMidiStop() {
            return release_outputs_on_midi_stop;
        }
        void setReleaseOutputsOnExternalClockLoss(bool value) {
            release_outputs_on_external_clock_loss = value;
        }
        bool isReleaseOutputsOnExternalClockLoss() {
            return release_outputs_on_external_clock_loss;
        }
};

extern SystemSettings system_settings;