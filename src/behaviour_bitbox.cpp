
#include "Config.h"
#include "ConfigMidi.h"

#include "midi_outs.h"

#include "behaviour_bitbox.h"

DeviceBehaviour_Bitbox behaviour_bitbox = DeviceBehaviour_Bitbox((midi::MidiInterface<midi::SerialMIDI<HardwareSerial>>*)nullptr, &ENABLE_BITBOX);

