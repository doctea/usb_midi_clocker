#pragma once

#include "Config.h"

#include "behaviours/behaviour_base.h"

#ifdef ENABLE_SCREEN
    #include "menuitems_lambda.h"
#endif

//template<class ConcreteClass=DeviceBehaviourUltimateBase>
class PolyphonicBehaviour : virtual public DeviceBehaviourUltimateBase {
    public:

    /*DeviceBehaviourUltimateBase *concrete_class = nullptr;
    virtual void setConcreteClass(DeviceBehaviourUltimateBase *concrete_class) {
        this->concrete_class = concrete_class;
    }*/

    virtual bool transmits_midi_notes() override {
        return true;
    }

    static const int8_t max_voice_count = 4;
    int voices[max_voice_count] = { NOTE_OFF, NOTE_OFF, NOTE_OFF, NOTE_OFF };   // ideally int8_t, but int is compatible with HarmonyStatus menuitem
    int last_voices[max_voice_count] = { NOTE_OFF, NOTE_OFF, NOTE_OFF, NOTE_OFF };
    target_id_t voice_target_id[max_voice_count] = { -1, -1, -1, -1 };

    bool allow_voice_for_auto[max_voice_count] = { true, true, true, true };

    const int8_t CHANNEL_ROUND_ROBIN = max_voice_count + 1;

    int8_t find_slot_for(int8_t note) {
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
            if (NOTE_OFF==this->find_slot_for(note) && note_slot>=0) { // find empty slot to use
                this->sendNoteOn(note, velocity, note_slot+1);
                this->current_channel = channel;
            }
        } else {
            // assign to given channel/pass through
            if (voices[channel-1]!=note) {
                this->sendNoteOff(note, 0, channel);
                voices[channel-1] = note;
                //DeviceBehaviourSerialBase::sendNoteOn(note, velocity, channel);
                //ConcreteClass::sendNoteOn(note, velocity, channel);
                DeviceBehaviourUltimateBase::sendNoteOn(note, velocity, channel);
                //this->concrete_class->sendNoteOn(note, velocity, channel);
            }
        }
    }

    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
        if (!is_valid_note(note)) return;
        if (channel==CHANNEL_ROUND_ROBIN) {
            // do auto-assigning of notes schtick
            int note_slot = this->find_slot_for(note);
            while (note_slot>=0) {
                this->sendNoteOff(note, velocity, note_slot+1);
                note_slot = this->find_slot_for(note);
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
            //ConcreteClass::sendNoteOff(note, velocity, channel);
            DeviceBehaviourUltimateBase::sendNoteOff(note, velocity, channel);
            //this->concrete_class->sendNoteOff(note, velocity, channel);
        }
    }

    virtual void setup_saveable_parameters() override {
        if (this->saveable_parameters==nullptr)
            DeviceBehaviourUltimateBase::setup_saveable_parameters();

        /*
        MIDIBassBehaviour::setup_saveable_parameters();
        DividedClockedBehaviour::setup_saveable_parameters();
        */

        //ModwheelReceiver::setup_saveable_parameters();

        this->saveable_parameters->add(new LSaveableParameter<bool>("Output 1", "Allowed by Auto", &this->allow_voice_for_auto[0], [=](bool v) -> void { this->allow_voice_for_auto[0] = v; }, [=]() -> bool { return this->allow_voice_for_auto[0]; }));
        this->saveable_parameters->add(new LSaveableParameter<bool>("Output 2", "Allowed by Auto", &this->allow_voice_for_auto[1], [=](bool v) -> void { this->allow_voice_for_auto[1] = v; }, [=]() -> bool { return this->allow_voice_for_auto[1]; }));
        this->saveable_parameters->add(new LSaveableParameter<bool>("Output 3", "Allowed by Auto", &this->allow_voice_for_auto[2], [=](bool v) -> void { this->allow_voice_for_auto[2] = v; }, [=]() -> bool { return this->allow_voice_for_auto[2]; }));
        this->saveable_parameters->add(new LSaveableParameter<bool>("Output 4", "Allowed by Auto", &this->allow_voice_for_auto[3], [=](bool v) -> void { this->allow_voice_for_auto[3] = v; }, [=]() -> bool { return this->allow_voice_for_auto[3]; }));
    }

    #ifdef ENABLE_SCREEN
        FLASHMEM
        //virtual LinkedList<MenuItem *> *make_menu_items() override;

        virtual LinkedList<MenuItem *> *make_menu_items() override {
            LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();

            //return menuitems;

            // todo: remove the sort-of useless default HarmonyStatus from the menuitems...?

            SubMenuItemBar *allow_voice_toggles = new SubMenuItemBar("Allowed by Auto");

            for (int i = 0 ; i < max_voice_count ; i++) {
                char label[MENU_C_MAX];
                snprintf(label, MENU_C_MAX, "Output %i", i+1);

                Serial_printf("%s#make_menu_items: Creating controls for %s\n", this->get_label(), label); Serial_flush();

                HarmonyStatus *status = new HarmonyStatus(label, &last_voices[i], &voices[i]);
                menuitems->add(status);

                allow_voice_toggles->add(new LambdaToggleControl(label, 
                    [=](bool v) -> void { this->allow_voice_for_auto[i] = v;    },
                    [=]()       -> bool { return this->allow_voice_for_auto[i]; }
                ));
            }

            menuitems->add(allow_voice_toggles);

            /*
            DividedClockedBehaviour::make_menu_items();
            MIDIBassBehaviour::make_menu_items();
            */

            return menuitems;
        }
    #endif
};
