#pragma once

#include "Config.h"

// MIDI MUSO CV12 4 voice mono mode (handle voice allocation ourselves)

#ifdef ENABLE_MIDIMUSO_4MV

//#include "behaviours/behaviour_base.h";
//class DeviceBehaviourUltimateBase;

#include "behaviours/behaviour_base.h"
#include "behaviours/behaviour_base_serial.h"
#include "behaviours/behaviour_simplewrapper.h"

#include "behaviour_midibass.h"

//extern Behaviour_SimpleWrapper<DividedClockedBehaviour,DeviceBehaviourSerialBase> *behaviour_midimuso_4mv;

class Behaviour_MIDIMuso_4MV : public DeviceBehaviourSerialBase, public MIDIBassBehaviour, public virtual DividedClockedBehaviour {
    public:

    using DividedClockedBehaviour::on_tick;
    using DividedClockedBehaviour::send_clock;

    Behaviour_MIDIMuso_4MV () : DeviceBehaviourSerialBase () {
        /*for (int i = 0 ; i < MIDI_MAX_NOTE ; i++) {
            playing_notes[i] = NOTE_OFF;
        }*/
        this->TUNING_OFFSET = -3;   // because MIDI MUSO CV12's tuning is based on 1V=A, not 1V=C
    }

    virtual bool transmits_midi_notes() override {
        return true;
    }

    virtual const char *get_label() override {
        return "MIDIMuso 4MV";
    }

    static const int8_t max_voice_count = 4;
    int voices[max_voice_count] = { NOTE_OFF, NOTE_OFF, NOTE_OFF, NOTE_OFF };   // ideally int8_t, but int is compatible with HarmonyStatus menuitem
    int last_voices[max_voice_count] = { NOTE_OFF, NOTE_OFF, NOTE_OFF, NOTE_OFF };
    target_id_t voice_target_id[max_voice_count] = { -1, -1, -1, -1 };

    bool allow_voice_for_auto[max_voice_count] = { true, true, true, true };

    void on_bar(int bar) override {
        MIDIBassBehaviour::on_bar(bar);
        DividedClockedBehaviour::on_bar(bar);
    }

    int8_t find_slot_for(int8_t note) {
        int8_t note_slot = -1;
        for (int i = 0 ; i < max_voice_count ; i++) {
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
        //this->debug = true;
        if (channel==5) {
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
        //this->debug = false;
    }

    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
        if (!is_valid_note(note)) return;
        //this->debug = true;
        if (channel==5) {
            // do auto-assigning of notes schtick
            int note_slot = this->find_slot_for(note);
            if (note_slot>=0) {
                this->sendNoteOff(note, velocity, note_slot+1);
            } /*else {
                Serial.printf("MV4 auto: didn't find note %i at all?!!\n", note);
            }*/
        } else {
            // assign to given channel/pass through
            if (voices[channel-1]==note) {
                voices[channel-1] = NOTE_OFF;
                last_voices[channel-1] = note;
            }
            //DeviceBehaviourSerialBase::sendNoteOff(note, velocity, channel);
            MIDIBassBehaviour::sendNoteOff(note, velocity, channel);
        }
        //this->debug = false;
    }

    virtual void setup_saveable_parameters() override {
        if (this->saveable_parameters==nullptr)
            DeviceBehaviourUltimateBase::setup_saveable_parameters();

        MIDIBassBehaviour::setup_saveable_parameters();

        this->saveable_parameters->add(new LSaveableParameter<bool>("Output 1", "Allowed by Auto", &this->allow_voice_for_auto[0], [=](bool v) -> void { this->allow_voice_for_auto[0] = v; }, [=]() -> bool { return this->allow_voice_for_auto[0]; }));
        this->saveable_parameters->add(new LSaveableParameter<bool>("Output 2", "Allowed by Auto", &this->allow_voice_for_auto[1], [=](bool v) -> void { this->allow_voice_for_auto[1] = v; }, [=]() -> bool { return this->allow_voice_for_auto[1]; }));
        this->saveable_parameters->add(new LSaveableParameter<bool>("Output 3", "Allowed by Auto", &this->allow_voice_for_auto[2], [=](bool v) -> void { this->allow_voice_for_auto[2] = v; }, [=]() -> bool { return this->allow_voice_for_auto[2]; }));
        this->saveable_parameters->add(new LSaveableParameter<bool>("Output 4", "Allowed by Auto", &this->allow_voice_for_auto[3], [=](bool v) -> void { this->allow_voice_for_auto[3] = v; }, [=]() -> bool { return this->allow_voice_for_auto[3]; }));
    }


    #ifdef ENABLE_SCREEN
        FLASHMEM
        virtual LinkedList<MenuItem *> *make_menu_items() override;
    #endif

};

extern Behaviour_MIDIMuso_4MV *behaviour_midimuso_4mv;

#endif
