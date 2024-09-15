#include "Config.h"
#include "ConfigMidi.h"

#if defined(ENABLE_BITBOX) && defined(ENABLE_BITBOX_DEDICATED)


    #include "midi/midi_outs.h"

    #include "behaviours/behaviour_bitbox.h"

    DeviceBehaviour_Bitbox *behaviour_bitbox = new DeviceBehaviour_Bitbox(); //(midi::MidiInterface<midi::SerialMIDI<HardwareSerial>>*)nullptr, &ENABLE_BITBOX);
#else
    #include "behaviours/behaviour_base_serial.h"
    #include "behaviours/behaviour_simplewrapper.h"
    Behaviour_SimpleWrapper<DeviceBehaviourSerialBase,DividedClockedBehaviour> *behaviour_bitbox = nullptr;
#endif