#include "Config.h"
#include "ConfigMidi.h"
#include "midi/MidiMappings.h"
#include "midi/midi_outs.h"
#include "midi/midi_out_wrapper.h"

#include "clock.h"

FLASHMEM void setup_midi_serial_devices() {
    // todo: move to DeviceBehaviourSerialBase..
    for (unsigned int i = 0 ; i < NUM_MIDI_OUTS ; i++) {
        midi_out_serial[i]->begin(MIDI_CHANNEL_OMNI);
        midi_out_serial[i]->turnThruOff();
    }

    #ifdef ENABLE_CLOCK_INPUT_MIDI_DIN
        midi_out_serial[0]->setHandleStart(din_midi_handle_start);
        midi_out_serial[0]->setHandleStop(din_midi_handle_stop);
        midi_out_serial[0]->setHandleClock(din_midi_handle_clock);
        midi_out_serial[0]->setHandleContinue(din_midi_handle_continue);        
    #endif
}
