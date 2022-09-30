
#include "Config.h"
#include "ConfigMidi.h"

#include "midi/midi_outs.h"

#include "behaviours/behaviour_bitbox.h"

DeviceBehaviour_Bitbox behaviour_bitbox = DeviceBehaviour_Bitbox(); //(midi::MidiInterface<midi::SerialMIDI<HardwareSerial>>*)nullptr, &ENABLE_BITBOX);

