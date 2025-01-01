#pragma once

#include "Config.h"

// MIDI MUSO CV12 4-voice monophonic mode (handle voice allocation ourselves - allow automatic and manual allocation)

#ifdef ENABLE_MIDIMUSO_4MV

#include "behaviours/behaviour_base.h"
#include "behaviours/behaviour_base_serial.h"
#include "behaviours/behaviour_simplewrapper.h"

#include "behaviours/behaviour_polyphonic.h"

#include "behaviour_midibass.h"

#include "behaviour_modwheelreceiver.h"

// todo: send program controls to switch modes
// todo: switch into 4A mode (extra gates, extra cv outs instead of velocity outs), or even 6 mode 

class Behaviour_MIDIMuso_4MV : virtual public DeviceBehaviourSerialBase, public MIDIBassBehaviour, virtual public ModwheelReceiver, virtual public DividedClockedBehaviour, virtual public PolyphonicBehaviour<MIDIBassBehaviour> {
    public:

    //using MIDIBassBehaviour::sendNoteOn;
    //using MIDIBassBehaviour::sendNoteOff;

    using DividedClockedBehaviour::on_tick;
    using DividedClockedBehaviour::send_clock;

    using DeviceBehaviourSerialBase::sendProxiedControlChange;

    const int8_t CHANNEL_ROUND_ROBIN = 5;

    Behaviour_MIDIMuso_4MV () : DeviceBehaviourSerialBase () {
        DeviceBehaviourUltimateBase::TUNING_OFFSET = -3;   // because MIDI MUSO CV12's tuning is based on 1V=A, not 1V=C
    }

    virtual bool transmits_midi_notes() override {
        return true;
    }

    virtual const char *get_label() override {
        return "MIDIMuso 4MV";
    }

    /*static const int8_t max_voice_count = 4;
    int voices[max_voice_count] = { NOTE_OFF, NOTE_OFF, NOTE_OFF, NOTE_OFF };   // ideally int8_t, but int is compatible with HarmonyStatus menuitem
    int last_voices[max_voice_count] = { NOTE_OFF, NOTE_OFF, NOTE_OFF, NOTE_OFF };
    target_id_t voice_target_id[max_voice_count] = { -1, -1, -1, -1 };

    bool allow_voice_for_auto[max_voice_count] = { true, true, true, true };*/

    void on_bar(int bar) override {
        MIDIBassBehaviour::on_bar(bar);
        DividedClockedBehaviour::on_bar(bar);
    }

    /*int8_t find_slot_for(int8_t note) {
        int8_t note_slot = -1;
        for (int_fast8_t i = 0 ; i < max_voice_count ; i++) {
            if (!allow_voice_for_auto[i])
                continue;
            if (voices[i]==note) {
                note_slot = i;
                break;
            }
        }
        return note_slot;
    }

    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
        if (!is_valid_note(note)) return;
        if (channel==CHANNEL_ROUND_ROBIN) {
            // do auto-assigning of notes schtick
            int note_slot = this->find_slot_for(-1);
            if (note_slot>=0) {
                this->sendNoteOn(note, velocity, note_slot+1);
            }
        } else {
            // assign to given channel/pass through
            if (voices[channel-1]!=note)
                this->sendNoteOff(note, 0, channel);
            voices[channel-1] = note;
            //DeviceBehaviourSerialBase::sendNoteOn(note, velocity, channel);
            MIDIBassBehaviour::sendNoteOn(note, velocity, channel);
        }
    }

    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
        if (!is_valid_note(note)) return;
        if (channel==CHANNEL_ROUND_ROBIN) {
            // do auto-assigning of notes schtick
            int note_slot = this->find_slot_for(note);
            if (note_slot>=0) {
                this->sendNoteOff(note, velocity, note_slot+1);
            } 
        } else {
            // assign to given channel/pass through
            if (voices[channel-1]==note) {
                voices[channel-1] = NOTE_OFF;
                last_voices[channel-1] = note;
            }
            //DeviceBehaviourSerialBase::sendNoteOff(note, velocity, channel);
            MIDIBassBehaviour::sendNoteOff(note, velocity, channel);
        }
    }*/

    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
        PolyphonicBehaviour::sendNoteOn(note, velocity, channel);
    }

    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
        PolyphonicBehaviour::sendNoteOff(note, velocity, channel);
    }

    virtual void sendControlChange(byte cc_number, byte value, byte channel = 0) override {
        //if (this->debug) Serial.printf(F("%s#sendControlChange(cc=%i, value=%i, channel=%i)\n"), this->get_label(), cc_number, value, channel);
        // if we receive a value from another device, then update the proxy parameter, which will handle the actual sending
        // if modwheel should handle this event, handle it and return early
        if (ModwheelReceiver::process(cc_number, value, channel)) 
            return;

        DeviceBehaviourUltimateBase::sendControlChange(cc_number, value, channel);
    }

    virtual void setup_saveable_parameters() override {
        if (this->saveable_parameters==nullptr)
            DeviceBehaviourUltimateBase::setup_saveable_parameters();

        MIDIBassBehaviour::setup_saveable_parameters();

        DividedClockedBehaviour::setup_saveable_parameters();

        PolyphonicBehaviour::setup_saveable_parameters();
    }


    //FLASHMEM 
    bool already_initialised_parameters = false;
    virtual LinkedList<FloatParameter*> *initialise_parameters() override {
        Serial_printf(F("DeviceBehaviour_CraftSynth#initialise_parameters()...")); Serial_flush();
        if (already_initialised_parameters)
            return this->parameters;

        already_initialised_parameters = true;

        
        //Serial.println(F("\tcalling DeviceBehaviourUSBBase::initialise_parameters()")); 
        DeviceBehaviourSerialBase::initialise_parameters();
        
        MIDIBassBehaviour::initialise_parameters();
        //Serial.println(F("\tcalling ClockedBehaviour::initialise_parameters()"));
        DividedClockedBehaviour::initialise_parameters();

        ModwheelReceiver::initialise_parameters();

        PolyphonicBehaviour::initialise_parameters();

        //Serial.println(F("\tAdding parameters..."));
        //parameters->clear();
        // todo: read these from a configuration file
        //this->add_parameters();
        //parameters->add(new MIDICCParameter<>("Mod Wheel",     this,    (byte)1,    (byte)1));    // this should be handled by ModwheelReceiver
        // todo: enable these two extra CC outputs when muso is in 4B or 6 mode - four extra CC outputs when in 4A mode!
        //parameters->add(new MIDICCParameter<>("7 vol",         this,   (byte)7,   (byte)1));
        //parameters->add(new MIDICCParameter<>("11 expr",       this,   (byte)11,   (byte)1));
        //parameters->add(new MIDICCParameter<>("73 attack",     this,   (byte)7,   (byte)1));
        //parameters->add(new MIDICCParameter<>("72 release",    this,   (byte)11,   (byte)1));
        parameters->add(new MIDICCParameter<>("71 res/aft",    this,    (byte)71,   (byte)1));
        parameters->add(new MIDICCParameter<>("74 cut off",    this,    (byte)74,   (byte)1));
        
        Serial_printf(F("DeviceBehaviour_CraftSynth#initialise_parameters() returning %p\n"), parameters); Serial_flush();
        return parameters;
    }

    #ifdef ENABLE_SCREEN
        FLASHMEM
        virtual LinkedList<MenuItem *> *make_menu_items() override;
    #endif

};

extern Behaviour_MIDIMuso_4MV *behaviour_midimuso_4mv;

#endif
