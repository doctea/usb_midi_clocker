
#include "Config.h"
#include "ConfigMidi.h"

#include "midi/midi_outs.h"

#include "behaviours/behaviour_neutron.h"

DeviceBehaviour_Neutron behaviour_neutron = DeviceBehaviour_Neutron(); //(midi::MidiInterface<midi::SerialMIDI<HardwareSerial>>*)nullptr, &ENABLE_BASS); //midi_out_serial[2]);

#ifdef ENABLE_SCREEN

    #include "menu.h"
    #include "menuitems_object.h"

    LinkedList<MenuItem *> DeviceBehaviour_Neutron::make_menu_items() {
        LinkedList<MenuItem *> menu = ClockedBehaviour::make_menu_items();
        #ifdef ENABLE_BASS_TRANSPOSE
            MIDIOutputWrapper_Behaviour *neutron_wrapper = (MIDIOutputWrapper_Behaviour *)midi_matrix_manager->get_target_for_handle((char*)"S3 : Neutron : ch 4");
            ObjectNumberControl<MIDIOutputWrapper,int> *neutron_transpose_control = new ObjectNumberControl<MIDIOutputWrapper,int>(
                "Neutron octave",
                neutron_wrapper, 
                &MIDIOutputWrapper::setForceOctave, 
                &MIDIOutputWrapper::getForceOctave, 
                nullptr,
                -1,
                8
            );
            //DeviceBehaviour_Neutron *behaviour_neutron = static_cast<DeviceBehaviour_Neutron *>(neutron_wrapper->output);
            HarmonyStatus *neutron_harmony = new HarmonyStatus("Neutron output", 
                &neutron_wrapper->last_transposed_note, 
                &neutron_wrapper->current_transposed_note, 
                &behaviour_neutron.last_drone_note
            );
            
            //TODO: see commented-out section in DeviceBehaviour_Neutron
            ObjectToggleControl<DeviceBehaviour_Neutron> *neutron_drone_bass = new ObjectToggleControl<DeviceBehaviour_Neutron> (
                "Neutron bass drone",
                &behaviour_neutron,
                &DeviceBehaviour_Neutron::set_drone,
                &DeviceBehaviour_Neutron::is_drone,
                nullptr
            );
            
            menu.add(neutron_drone_bass);
            menu.add(neutron_transpose_control);  // beatstep transposed to neutron control
            menu.add(neutron_harmony);
        #endif
        return menu;
    }
#endif