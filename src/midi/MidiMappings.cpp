#include "Config.h"
#include "ConfigMidi.h"
#include "midi/MidiMappings.h"
#include "midi/midi_outs.h"
#include "midi/midi_out_wrapper.h"

void setup_midi_serial_devices() {
    // todo: move to DeviceBehaviourSerialBase..
    for (int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
        midi_out_serial[i]->begin(MIDI_CHANNEL_OMNI);
        midi_out_serial[i]->turnThruOff();
    }
}
