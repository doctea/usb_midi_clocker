
#include "Config.h"
#include "ConfigMidi.h"

#include "midi_outs.h"

#include "behaviour_neutron.h"

DeviceBehaviour_Neutron behaviour_neutron = DeviceBehaviour_Neutron((midi::MidiInterface<midi::SerialMIDI<HardwareSerial>>*)nullptr, &MIDI2); //midi_out_serial[2]);

