
#include "Config.h"
#include "ConfigMidi.h"

#include "midi_outs.h"

#include "behaviour_neutron.h"

DeviceBehaviour_Neutron behaviour_neutron = DeviceBehaviour_Neutron(); //(midi::MidiInterface<midi::SerialMIDI<HardwareSerial>>*)nullptr, &ENABLE_BASS); //midi_out_serial[2]);
